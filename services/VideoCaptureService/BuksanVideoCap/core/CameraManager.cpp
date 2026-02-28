#include "CameraManager.h"
#include "../src/CameraSession.h"
#include "../src/ConfigLoader.h"
#include <algorithm>

namespace buksan {

bool CameraManager::addCamera(const std::string& id,
                             const std::string& rtsp_url,
                             const std::string& storage_path,
                             int segment_duration) {
    if (id.empty() || rtsp_url.empty() || storage_path.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    if (cameras_.find(id) != cameras_.end()) {
        return false;
    }
    CameraEntry e;
    e.id = id;
    e.rtsp_url = rtsp_url;
    e.storage_path = storage_path;
    e.segment_duration = segment_duration <= 0 ? 300 : segment_duration;
    cameras_[id] = std::move(e);
    return true;
}

bool CameraManager::removeCamera(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(id);
    if (it == cameras_.end()) {
        return false;
    }
    if (it->second.session && it->second.session->running()) {
        return false;
    }
    cameras_.erase(it);
    return true;
}

bool CameraManager::startRecording(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(id);
    if (it == cameras_.end()) {
        return false;
    }
    CameraEntry& e = it->second;
    if (e.session && e.session->running()) {
        return false;
    }
    CameraConfig config;
    config.id = e.id;
    config.rtsp_url = e.rtsp_url;
    config.record = true;
    config.analytics = false;
    e.session = std::make_shared<CameraSession>(config, e.storage_path, e.segment_duration);
    e.session->start();
    return true;
}

bool CameraManager::stopRecording(const std::string& id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(id);
    if (it == cameras_.end()) {
        return false;
    }
    CameraEntry& e = it->second;
    if (!e.session || !e.session->running()) {
        return false;
    }
    e.session->stop();
    e.session.reset();
    return true;
}

std::string CameraManager::getStatus(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = cameras_.find(id);
    if (it == cameras_.end()) {
        return "";
    }
    if (it->second.session && it->second.session->running()) {
        return "running";
    }
    return "stopped";
}

bool CameraManager::cameraExists(const std::string& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return cameras_.find(id) != cameras_.end();
}

void CameraManager::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& p : cameras_) {
        if (p.second.session && p.second.session->running()) {
            p.second.session->stop();
            p.second.session.reset();
        }
    }
}

std::vector<std::pair<std::string, std::string>> CameraManager::listCameras() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(cameras_.size());
    for (const auto& p : cameras_) {
        std::string status = (p.second.session && p.second.session->running()) ? "running" : "stopped";
        out.emplace_back(p.first, status);
    }
    return out;
}

} // namespace buksan
