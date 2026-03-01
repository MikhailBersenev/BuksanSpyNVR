#ifndef UTILS_INMEMORYMETADATASYNCQUEUE_H
#define UTILS_INMEMORYMETADATASYNCQUEUE_H

#include "repositories/interfaces/IMetadataSyncQueue.h"
#include <deque>
#include <mutex>

namespace buksan {

class InMemoryMetadataSyncQueue final : public IMetadataSyncQueue {
public:
    void enqueue(CreateRecordingCommand command) override;
    bool tryDequeue(CreateRecordingCommand& command) override;
    std::size_t size() const override;

private:
    mutable std::mutex mutex_;
    std::deque<CreateRecordingCommand> queue_;
};

} // namespace buksan

#endif // UTILS_INMEMORYMETADATASYNCQUEUE_H
