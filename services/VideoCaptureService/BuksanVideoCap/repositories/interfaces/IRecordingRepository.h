#ifndef REPOSITORIES_INTERFACES_IRECORDINGREPOSITORY_H
#define REPOSITORIES_INTERFACES_IRECORDINGREPOSITORY_H

#include "models/Recording.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace buksan {

class IRecordingRepository {
public:
    virtual ~IRecordingRepository() = default;

    virtual std::vector<Recording> findByCameraAndRange(const RecordingQuery& query) = 0;
    virtual std::optional<Recording> findById(std::int64_t recordingId) = 0;
    virtual std::int64_t create(const CreateRecordingCommand& command) = 0;
};

} // namespace buksan

#endif // REPOSITORIES_INTERFACES_IRECORDINGREPOSITORY_H
