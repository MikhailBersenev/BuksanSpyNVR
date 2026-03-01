#include "services/CameraService.h"
#include <stdexcept>
#include <utility>

namespace buksan {

CameraService::CameraService(std::unique_ptr<ICameraRepository> cameraRepository)
    : cameraRepository_(std::move(cameraRepository)) {
    if (!cameraRepository_) {
        throw std::invalid_argument("CameraService requires repository");
    }
}

std::optional<Camera> CameraService::findById(std::int64_t cameraId) {
    return cameraRepository_->findById(cameraId);
}

std::vector<Camera> CameraService::listAll() {
    return cameraRepository_->listAll();
}

std::vector<std::int64_t> CameraService::registerFromConfig(const std::vector<RegisterCameraCommand>& cameras) {
    std::vector<std::int64_t> deviceIds;
    deviceIds.reserve(cameras.size());

    for (const auto& camera : cameras) {
        if (camera.rtspUrl.empty()) {
            continue;
        }

        auto existing = cameraRepository_->findByRtspUrl(camera.rtspUrl);
        if (existing.has_value()) {
            deviceIds.push_back(existing->deviceId);
            continue;
        }

        deviceIds.push_back(cameraRepository_->create(camera));
    }

    return deviceIds;
}

} // namespace buksan
