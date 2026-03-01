#ifndef SERVICES_CAMERASERVICE_H
#define SERVICES_CAMERASERVICE_H

#include "repositories/interfaces/ICameraRepository.h"
#include <memory>
#include <optional>
#include <vector>

namespace buksan {

class CameraService {
public:
    explicit CameraService(std::unique_ptr<ICameraRepository> cameraRepository);

    std::optional<Camera> findById(std::int64_t cameraId);
    std::vector<Camera> listAll();
    std::vector<std::int64_t> registerFromConfig(const std::vector<RegisterCameraCommand>& cameras);

private:
    std::unique_ptr<ICameraRepository> cameraRepository_;
};

} // namespace buksan

#endif // SERVICES_CAMERASERVICE_H
