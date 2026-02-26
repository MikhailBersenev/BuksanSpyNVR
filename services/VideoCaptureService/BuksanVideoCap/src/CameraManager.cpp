#include "CameraManager.h"
#include "CameraSession.h"

namespace buksan {

CameraManager::CameraManager(const AppConfig& config) : config_(config) {
    for (const auto& cam : config_.cameras) {
        if (cam.rtsp_url.empty()) continue;
        sessions_.push_back(std::make_shared<CameraSession>(cam, config_.storage_path));
    }
}

void CameraManager::startAll() {
    for (auto& s : sessions_) {
        s->start();
    }
}

void CameraManager::stopAll() {
    for (auto& s : sessions_) {
        s->stop();
    }
}

} // namespace buksan
