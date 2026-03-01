#ifndef MODELS_CAMERA_H
#define MODELS_CAMERA_H

#include <cstdint>
#include <optional>
#include <string>

namespace buksan {

struct Camera {
    std::int64_t deviceId{0};
    std::int64_t type{0};
    std::string addDate;
    std::string caption;
    std::string rtspUrl;
    std::optional<std::string> assignedNodeId;
    std::string status;
};

struct RegisterCameraCommand {
    std::int64_t type{0};
    std::string caption;
    std::string rtspUrl;
    std::optional<std::string> assignedNodeId;
    std::string status{"active"};
};

} // namespace buksan

#endif // MODELS_CAMERA_H
