#ifndef SERVICES_METADATASYNCWORKER_H
#define SERVICES_METADATASYNCWORKER_H

#include "services/RecordingService.h"
#include <atomic>
#include <chrono>
#include <cstddef>
#include <thread>

namespace buksan {

class MetadataSyncWorker {
public:
    MetadataSyncWorker(RecordingService& recordingService,
                       std::chrono::milliseconds retryInterval,
                       std::size_t maxBatchSize);
    ~MetadataSyncWorker();

    void start();
    void stop();

private:
    void runLoop();

    RecordingService& recordingService_;
    std::chrono::milliseconds retryInterval_;
    std::size_t maxBatchSize_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;
};

} // namespace buksan

#endif // SERVICES_METADATASYNCWORKER_H
