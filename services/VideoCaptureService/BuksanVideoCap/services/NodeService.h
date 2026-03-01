#ifndef SERVICES_NODESERVICE_H
#define SERVICES_NODESERVICE_H

#include "repositories/interfaces/INodeRepository.h"
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace buksan {

class NodeService {
public:
    explicit NodeService(std::unique_ptr<INodeRepository> nodeRepository);

    std::optional<Node> findById(const std::string& nodeId);
    std::vector<Node> listAll();

private:
    std::unique_ptr<INodeRepository> nodeRepository_;
};

} // namespace buksan

#endif // SERVICES_NODESERVICE_H
