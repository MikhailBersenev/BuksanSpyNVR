#ifndef REPOSITORIES_INTERFACES_ICAMERAREPOSITORY_H
#define REPOSITORIES_INTERFACES_ICAMERAREPOSITORY_H

#include "models/Camera.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace buksan {

class ICameraRepository {
public:
    virtual ~ICameraRepository() = default;

    virtual std::optional<Camera> findById(std::int64_t cameraId) = 0;
    virtual std::optional<Camera> findByRtspUrl(const std::string& rtspUrl) = 0;
    virtual std::vector<Camera> listAll() = 0;
    virtual std::int64_t create(const RegisterCameraCommand& command) = 0;
};

} // namespace buksan

#endif // REPOSITORIES_INTERFACES_ICAMERAREPOSITORY_H
