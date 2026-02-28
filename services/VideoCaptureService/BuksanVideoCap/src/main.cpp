#include "ConfigLoader.h"
#include "StorageManager.h"
#include "core/CameraManager.h"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
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

} // namespace

int main(int argc, char* argv[]) {
    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp", 1);

    std::string config_path;
    bool run_api = true;
    uint16_t api_port = 8080;

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

#ifdef BUKSAN_BUILD_API
    if (run_api) {
        std::cout << "API: http://0.0.0.0:" << api_port << "/api/v1" << std::endl;
        buksan::HttpServer server(manager, api_port);
        server.run();
        return 0;
    }
#endif

    while (!shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    return 0;
}
