#ifndef SERVICES_RECORDINGSERVICE_H
#define SERVICES_RECORDINGSERVICE_H

#include "repositories/interfaces/IMetadataSyncQueue.h"
#include "repositories/interfaces/IRecordingRepository.h"
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace buksan {

struct RegisterRecordingResult {
    bool persisted{false};
    std::optional<std::int64_t> recordId;
};

class RecordingService {
public:
    RecordingService(std::unique_ptr<IRecordingRepository> recordingRepository,
                     std::shared_ptr<IMetadataSyncQueue> metadataQueue);

    std::vector<Recording> findByCameraAndRange(const RecordingQuery& query);
    std::optional<Recording> findById(std::int64_t recordingId);
    RegisterRecordingResult registerSegment(const CreateRecordingCommand& command);
    std::size_t flushPendingMetadata(std::size_t maxBatchSize);
    std::size_t pendingQueueSize() const;

private:
    std::unique_ptr<IRecordingRepository> recordingRepository_;
    std::shared_ptr<IMetadataSyncQueue> metadataQueue_;
};

} // namespace buksan

#endif // SERVICES_RECORDINGSERVICE_H
