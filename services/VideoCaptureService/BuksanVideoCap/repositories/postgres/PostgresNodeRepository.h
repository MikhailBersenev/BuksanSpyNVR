#ifndef REPOSITORIES_POSTGRES_POSTGRESNODEREPOSITORY_H
#define REPOSITORIES_POSTGRES_POSTGRESNODEREPOSITORY_H

#include "db/IConnectionPool.h"
#include "repositories/interfaces/INodeRepository.h"
#include <memory>

namespace buksan {

class PostgresNodeRepository final : public INodeRepository {
public:
    explicit PostgresNodeRepository(std::shared_ptr<IConnectionPool> pool);

    std::optional<Node> findById(const std::string& nodeId) override;
    std::vector<Node> listAll() override;

private:
    std::shared_ptr<IConnectionPool> pool_;
};

} // namespace buksan

#endif // REPOSITORIES_POSTGRES_POSTGRESNODEREPOSITORY_H
