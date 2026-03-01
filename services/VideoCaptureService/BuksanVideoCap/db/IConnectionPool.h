#ifndef DB_ICONNECTIONPOOL_H
#define DB_ICONNECTIONPOOL_H

#include <memory>

namespace pqxx {
class connection;
}

namespace buksan {

class IConnectionPool {
public:
    virtual ~IConnectionPool() = default;

    virtual std::shared_ptr<pqxx::connection> acquire() = 0;
    virtual void release(std::shared_ptr<pqxx::connection> connection) = 0;
};

class PooledConnection {
public:
    PooledConnection(std::shared_ptr<IConnectionPool> pool, std::shared_ptr<pqxx::connection> connection);
    ~PooledConnection();

    PooledConnection(PooledConnection&& other) noexcept;
    PooledConnection& operator=(PooledConnection&& other) noexcept;

    PooledConnection(const PooledConnection&) = delete;
    PooledConnection& operator=(const PooledConnection&) = delete;

    pqxx::connection& get() const;

private:
    std::shared_ptr<IConnectionPool> pool_;
    std::shared_ptr<pqxx::connection> connection_;
};

} // namespace buksan

#endif // DB_ICONNECTIONPOOL_H
