#include "services/NodeService.h"
#include <stdexcept>
#include <utility>

namespace buksan {

NodeService::NodeService(std::unique_ptr<INodeRepository> nodeRepository)
    : nodeRepository_(std::move(nodeRepository)) {
    if (!nodeRepository_) {
        throw std::invalid_argument("NodeService requires repository");
    }
}

std::optional<Node> NodeService::findById(const std::string& nodeId) {
    return nodeRepository_->findById(nodeId);
}

std::vector<Node> NodeService::listAll() {
    return nodeRepository_->listAll();
}

} // namespace buksan
