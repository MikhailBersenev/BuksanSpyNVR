#ifndef DB_POSTGRESCONNECTIONPOOL_H
#define DB_POSTGRESCONNECTIONPOOL_H

#include "db/IConnectionPool.h"
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace pqxx {
class connection;
}

namespace buksan {

class PostgresConnectionPool final : public IConnectionPool {
public:
    PostgresConnectionPool(std::string connectionString, std::size_t poolSize);

    std::shared_ptr<pqxx::connection> acquire() override;
    void release(std::shared_ptr<pqxx::connection> connection) override;

private:
    std::string connectionString_;
    std::size_t poolSize_{0};
    std::queue<std::shared_ptr<pqxx::connection>> available_;
    std::size_t activeConnections_{0};
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace buksan

#endif // DB_POSTGRESCONNECTIONPOOL_H
