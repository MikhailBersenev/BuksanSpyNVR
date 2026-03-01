#include "services/MetadataSyncWorker.h"
#include <thread>

namespace buksan {

MetadataSyncWorker::MetadataSyncWorker(RecordingService& recordingService,
                                       std::chrono::milliseconds retryInterval,
                                       std::size_t maxBatchSize)
    : recordingService_(recordingService)
    , retryInterval_(retryInterval)
    , maxBatchSize_(maxBatchSize) {
}

MetadataSyncWorker::~MetadataSyncWorker() {
    stop();
}

void MetadataSyncWorker::start() {
    if (running_.exchange(true)) {
        return;
    }
    workerThread_ = std::thread(&MetadataSyncWorker::runLoop, this);
}

void MetadataSyncWorker::stop() {
    if (!running_.exchange(false)) {
        return;
    }
    if (workerThread_.joinable()) {
        workerThread_.join();
    }
}

void MetadataSyncWorker::runLoop() {
    while (running_.load()) {
        recordingService_.flushPendingMetadata(maxBatchSize_);
        std::this_thread::sleep_for(retryInterval_);
    }
}

} // namespace buksan
