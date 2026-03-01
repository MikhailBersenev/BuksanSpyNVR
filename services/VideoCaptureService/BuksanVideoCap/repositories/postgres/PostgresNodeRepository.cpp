#include "repositories/postgres/PostgresNodeRepository.h"
#include <pqxx/pqxx>
#include <stdexcept>

namespace buksan {

namespace {

Node mapNode(const pqxx::row& row) {
    Node node;
    node.nodeId = row["node_id"].c_str();
    node.caption = row["caption"].c_str();
    node.status = row["status"].c_str();
    return node;
}

} // namespace

PostgresNodeRepository::PostgresNodeRepository(std::shared_ptr<IConnectionPool> pool)
    : pool_(std::move(pool)) {
    if (!pool_) {
        throw std::invalid_argument("PostgresNodeRepository requires a connection pool");
    }
}

std::optional<Node> PostgresNodeRepository::findById(const std::string& nodeId) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "SELECT node_id, caption, status FROM nodes WHERE node_id = $1",
        nodeId);

    if (result.empty()) {
        return std::nullopt;
    }
    return mapNode(result.front());
}

std::vector<Node> PostgresNodeRepository::listAll() {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec("SELECT node_id, caption, status FROM nodes ORDER BY node_id ASC");

    std::vector<Node> nodes;
    nodes.reserve(result.size());
    for (const auto& row : result) {
        nodes.push_back(mapNode(row));
    }
    return nodes;
}

} // namespace buksan
