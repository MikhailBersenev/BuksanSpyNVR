#ifndef REPOSITORIES_INTERFACES_IMETADATASYNCQUEUE_H
#define REPOSITORIES_INTERFACES_IMETADATASYNCQUEUE_H

#include "models/Recording.h"
#include <cstddef>

namespace buksan {

class IMetadataSyncQueue {
public:
    virtual ~IMetadataSyncQueue() = default;

    virtual void enqueue(CreateRecordingCommand command) = 0;
    virtual bool tryDequeue(CreateRecordingCommand& command) = 0;
    virtual std::size_t size() const = 0;
};

} // namespace buksan

#endif // REPOSITORIES_INTERFACES_IMETADATASYNCQUEUE_H
