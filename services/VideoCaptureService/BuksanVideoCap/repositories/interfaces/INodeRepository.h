#ifndef REPOSITORIES_INTERFACES_INODEREPOSITORY_H
#define REPOSITORIES_INTERFACES_INODEREPOSITORY_H

#include "models/Node.h"
#include <optional>
#include <string>
#include <vector>

namespace buksan {

class INodeRepository {
public:
    virtual ~INodeRepository() = default;

    virtual std::optional<Node> findById(const std::string& nodeId) = 0;
    virtual std::vector<Node> listAll() = 0;
};

} // namespace buksan

#endif // REPOSITORIES_INTERFACES_INODEREPOSITORY_H
