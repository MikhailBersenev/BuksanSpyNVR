#ifndef MODELS_RECORDING_H
#define MODELS_RECORDING_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace buksan {

struct Recording {
    std::int64_t recordId{0};
    std::int64_t userId{0};
    std::int64_t unixTime{0};
    std::string mediaFile;
    std::optional<std::int64_t> alertId;
    std::int64_t deviceId{0};
    std::string timeValue;
    std::string dateValue;
    std::optional<std::int64_t> mandatoryMark;
};

struct CreateRecordingCommand {
    std::int64_t userId{0};
    std::int64_t unixTime{0};
    std::string mediaFile;
    std::optional<std::int64_t> alertId;
    std::int64_t deviceId{0};
    std::string timeValue;
    std::string dateValue;
    std::optional<std::int64_t> mandatoryMark;
};

struct RecordingQuery {
    std::int64_t cameraId{0};
    std::int64_t fromUnix{0};
    std::int64_t toUnix{0};
};

} // namespace buksan

#endif // MODELS_RECORDING_H
