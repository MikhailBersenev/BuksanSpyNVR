#include "ConfigLoader.h"
#include "StorageManager.h"
#include "core/CameraManager.h"
#include "db/PostgresConnectionPool.h"
#include "repositories/postgres/PostgresCameraRepository.h"
#include "repositories/postgres/PostgresNodeRepository.h"
#include "repositories/postgres/PostgresRecordingRepository.h"
#include "services/CameraService.h"
#include "services/MetadataSyncWorker.h"
#include "services/NodeService.h"
#include "services/RecordingService.h"
#include "utils/InMemoryMetadataSyncQueue.h"
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#ifdef __linux__
#include <unistd.h>
#endif

#ifdef BUKSAN_BUILD_API
#include "api/HttpServer.h"
#endif

namespace {

std::atomic<bool> shutdown_requested{false};
buksan::CameraManager* g_manager = nullptr;

void signalHandler(int) {
    shutdown_requested.store(true);
    if (g_manager) {
        g_manager->stopAll();
    }
#ifdef BUKSAN_BUILD_API
    exit(0);
#endif
}

std::string findConfigPath(const std::string& fromArg) {
    if (!fromArg.empty()) return fromArg;
    std::ifstream f("config.yaml");
    if (f.good()) return "config.yaml";
#ifdef __linux__
    char buf[1024];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        std::string exe(buf);
        auto sep = exe.find_last_of('/');
        if (sep != std::string::npos) {
            std::string candidate = exe.substr(0, sep) + "/../config.yaml";
            std::ifstream c(candidate);
            if (c.good()) return candidate;
        }
    }
#endif
    return "config.yaml";
}

std::string readEnvOrDefault(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }
    return value;
}

int readEnvIntOrDefault(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return fallback;
    }
    try {
        const int parsed = std::stoi(value);
        return parsed > 0 ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}

} // namespace

int main(int argc, char* argv[]) {
    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp", 1);

    std::string config_path;
    bool run_api = true;
    uint16_t api_port = 8080;
    std::shared_ptr<buksan::PostgresConnectionPool> pool;
    std::unique_ptr<buksan::RecordingService> recordingService;
    std::unique_ptr<buksan::CameraService> cameraService;
    std::unique_ptr<buksan::NodeService> nodeService;
    std::shared_ptr<buksan::InMemoryMetadataSyncQueue> metadataQueue;
    std::unique_ptr<buksan::MetadataSyncWorker> metadataSyncWorker;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "--no-api") {
            run_api = false;
        } else if (arg == "--api" && i + 1 < argc) {
            try {
                api_port = static_cast<uint16_t>(std::stoul(argv[++i]));
            } catch (...) {}
        } else if (arg != "--config" && arg != "--api" && arg[0] != '-') {
            try {
                api_port = static_cast<uint16_t>(std::stoul(arg));
            } catch (...) {}
        }
    }

    config_path = findConfigPath(config_path);

    buksan::CameraManager manager;
    g_manager = &manager;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    buksan::ConfigLoader loader(config_path);
    if (!loader.loaded()) {
        std::cout << "Config error: " << loader.error() << " (file: " << config_path << ")" << std::endl;
        return 1;
    }
    int started = 0;
    {
        const auto& config = loader.config();
        if (!config.storage_path.empty()) {
            buksan::StorageManager storage(config.storage_path);
            storage.ensureDirectory();
            for (const auto& cam : config.cameras) {
                if (cam.rtsp_url.empty()) continue;
                if (manager.addCamera(cam.id, cam.rtsp_url, config.storage_path, 300)) {
                    manager.startRecording(cam.id);
                    ++started;
                }
            }
        }
    }
    if (started > 0) {
        std::cout << "Started " << started << " camera(s) from " << config_path << std::endl;
    }

    try {
        const std::string dbConnectionString = readEnvOrDefault(
            "BUKSAN_PG_DSN",
            "dbname=buksanspy user=postgres password=postgres host=127.0.0.1 port=5432");
        const int poolSize = readEnvIntOrDefault("BUKSAN_PG_POOL_SIZE", 8);
        const int retrySeconds = readEnvIntOrDefault("BUKSAN_METADATA_RETRY_SECONDS", 2);
        const int retryBatch = readEnvIntOrDefault("BUKSAN_METADATA_RETRY_BATCH", 64);

        pool = std::make_shared<buksan::PostgresConnectionPool>(dbConnectionString, static_cast<std::size_t>(poolSize));
        metadataQueue = std::make_shared<buksan::InMemoryMetadataSyncQueue>();

        auto recordingRepository = std::make_unique<buksan::PostgresRecordingRepository>(pool);
        auto cameraRepository = std::make_unique<buksan::PostgresCameraRepository>(pool);
        auto nodeRepository = std::make_unique<buksan::PostgresNodeRepository>(pool);

        std::shared_ptr<buksan::IMetadataSyncQueue> queueAbstraction = metadataQueue;
        recordingService = std::make_unique<buksan::RecordingService>(std::move(recordingRepository), std::move(queueAbstraction));
        cameraService = std::make_unique<buksan::CameraService>(std::move(cameraRepository));
        nodeService = std::make_unique<buksan::NodeService>(std::move(nodeRepository));

        std::vector<buksan::RegisterCameraCommand> cameraCommands;
        cameraCommands.reserve(loader.config().cameras.size());
        for (const auto& configCamera : loader.config().cameras) {
            if (configCamera.rtsp_url.empty()) {
                continue;
            }

            buksan::RegisterCameraCommand command;
            command.type = 0;
            command.caption = configCamera.id.empty() ? configCamera.rtsp_url : configCamera.id;
            command.rtspUrl = configCamera.rtsp_url;
            command.status = "active";
            cameraCommands.push_back(std::move(command));
        }
        cameraService->registerFromConfig(cameraCommands);

        metadataSyncWorker = std::make_unique<buksan::MetadataSyncWorker>(
            *recordingService,
            std::chrono::milliseconds(retrySeconds * 1000),
            static_cast<std::size_t>(retryBatch));
        metadataSyncWorker->start();
    } catch (const std::exception& e) {
        std::cerr << "Database wiring failed: " << e.what() << std::endl;
        return 1;
    }

#ifdef BUKSAN_BUILD_API
    if (run_api) {
        std::cout << "API: http://0.0.0.0:" << api_port << "/api/v1" << std::endl;
        buksan::HttpServer server(manager, *recordingService, *cameraService, *nodeService, api_port);
        server.run();
        return 0;
    }
#endif

    while (!shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (metadataSyncWorker) {
        metadataSyncWorker->stop();
    }
    return 0;
}
