#include "services/RecordingService.h"
#include <stdexcept>
#include <utility>

namespace buksan {

RecordingService::RecordingService(std::unique_ptr<IRecordingRepository> recordingRepository,
                                   std::shared_ptr<IMetadataSyncQueue> metadataQueue)
    : recordingRepository_(std::move(recordingRepository))
    , metadataQueue_(std::move(metadataQueue)) {
    if (!recordingRepository_ || !metadataQueue_) {
        throw std::invalid_argument("RecordingService requires repository and metadata queue");
    }
}

std::vector<Recording> RecordingService::findByCameraAndRange(const RecordingQuery& query) {
    return recordingRepository_->findByCameraAndRange(query);
}

std::optional<Recording> RecordingService::findById(std::int64_t recordingId) {
    return recordingRepository_->findById(recordingId);
}

RegisterRecordingResult RecordingService::registerSegment(const CreateRecordingCommand& command) {
    try {
        const std::int64_t recordId = recordingRepository_->create(command);
        return {.persisted = true, .recordId = recordId};
    } catch (...) {
        metadataQueue_->enqueue(command);
        return {.persisted = false, .recordId = std::nullopt};
    }
}

std::size_t RecordingService::flushPendingMetadata(std::size_t maxBatchSize) {
    std::size_t flushedCount = 0;
    for (std::size_t i = 0; i < maxBatchSize; ++i) {
        CreateRecordingCommand command;
        if (!metadataQueue_->tryDequeue(command)) {
            break;
        }

        try {
            recordingRepository_->create(command);
            ++flushedCount;
        } catch (...) {
            metadataQueue_->enqueue(std::move(command));
            break;
        }
    }
    return flushedCount;
}

std::size_t RecordingService::pendingQueueSize() const {
    return metadataQueue_->size();
}

} // namespace buksan
