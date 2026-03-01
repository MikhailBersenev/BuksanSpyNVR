#include "IConnectionPool.h"
#include <pqxx/pqxx>
#include <utility>

namespace buksan {

PooledConnection::PooledConnection(std::shared_ptr<IConnectionPool> pool, std::shared_ptr<pqxx::connection> connection)
    : pool_(std::move(pool))
    , connection_(std::move(connection)) {
}

PooledConnection::~PooledConnection() {
    if (pool_ && connection_) {
        pool_->release(connection_);
    }
}

PooledConnection::PooledConnection(PooledConnection&& other) noexcept
    : pool_(std::move(other.pool_))
    , connection_(std::move(other.connection_)) {
}

PooledConnection& PooledConnection::operator=(PooledConnection&& other) noexcept {
    if (this != &other) {
        if (pool_ && connection_) {
            pool_->release(connection_);
        }
        pool_ = std::move(other.pool_);
        connection_ = std::move(other.connection_);
    }
    return *this;
}

pqxx::connection& PooledConnection::get() const {
    return *connection_;
}

} // namespace buksan
