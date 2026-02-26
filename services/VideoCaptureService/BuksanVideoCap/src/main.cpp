#include "ConfigLoader.h"
#include "StorageManager.h"
#include "CameraManager.h"
#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

namespace {

std::atomic<bool> shutdown_requested{false};

void signalHandler(int) {
    shutdown_requested.store(true);
}

} // namespace

int main(int argc, char* argv[]) {
    (void)argc;
    const std::string config_path = (argv[1] && argv[1][0]) ? argv[1] : "config.yaml";

    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;tcp", 1);

    std::signal(SIGINT, signalHandler);

    buksan::ConfigLoader loader(config_path);
    if (!loader.loaded()) {
        std::cout << "Config error: " << loader.error() << std::endl;
        return 1;
    }

    const auto& config = loader.config();
    if (config.storage_path.empty()) {
        std::cout << "Config error: storage_path is required" << std::endl;
        return 1;
    }

    buksan::StorageManager storage(config.storage_path);
    if (!storage.ensureDirectory()) {
        std::cout << "Failed to create storage directory: " << config.storage_path << std::endl;
        return 1;
    }

    buksan::CameraManager manager(config);
    manager.startAll();

    while (!shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    manager.stopAll();
    return 0;
}
