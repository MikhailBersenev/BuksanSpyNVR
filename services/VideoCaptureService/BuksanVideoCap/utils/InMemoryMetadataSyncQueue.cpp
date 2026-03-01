#include "utils/InMemoryMetadataSyncQueue.h"

namespace buksan {

void InMemoryMetadataSyncQueue::enqueue(CreateRecordingCommand command) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push_back(std::move(command));
}

bool InMemoryMetadataSyncQueue::tryDequeue(CreateRecordingCommand& command) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return false;
    }
    command = std::move(queue_.front());
    queue_.pop_front();
    return true;
}

std::size_t InMemoryMetadataSyncQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

} // namespace buksan
