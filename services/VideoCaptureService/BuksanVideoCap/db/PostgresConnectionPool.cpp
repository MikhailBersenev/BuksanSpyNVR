#include "db/PostgresConnectionPool.h"
#include <pqxx/pqxx>
#include <stdexcept>
#include <utility>

namespace buksan {

PostgresConnectionPool::PostgresConnectionPool(std::string connectionString, std::size_t poolSize)
    : connectionString_(std::move(connectionString))
    , poolSize_(poolSize) {
    if (poolSize_ == 0) {
        throw std::invalid_argument("Postgres connection pool size must be greater than zero");
    }
}

std::shared_ptr<pqxx::connection> PostgresConnectionPool::acquire() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (!available_.empty()) {
                auto connection = available_.front();
                available_.pop();
                return connection;
            }
            if (activeConnections_ < poolSize_) {
                ++activeConnections_;
                lock.unlock();
                auto connection = std::make_shared<pqxx::connection>(connectionString_);
                if (!connection->is_open()) {
                    std::lock_guard<std::mutex> rollbackLock(mutex_);
                    --activeConnections_;
                    cv_.notify_one();
                    throw std::runtime_error("Cannot open PostgreSQL connection");
                }
                return connection;
            }
            cv_.wait(lock, [this] { return !available_.empty() || activeConnections_ < poolSize_; });
        }
    }
}

void PostgresConnectionPool::release(std::shared_ptr<pqxx::connection> connection) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!connection || !connection->is_open()) {
            if (activeConnections_ > 0) {
                --activeConnections_;
            }
        } else {
            available_.push(std::move(connection));
        }
    }
    cv_.notify_one();
}

} // namespace buksan
