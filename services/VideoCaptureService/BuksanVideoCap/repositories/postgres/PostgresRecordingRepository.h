#ifndef REPOSITORIES_POSTGRES_POSTGRESRECORDINGREPOSITORY_H
#define REPOSITORIES_POSTGRES_POSTGRESRECORDINGREPOSITORY_H

#include "db/IConnectionPool.h"
#include "repositories/interfaces/IRecordingRepository.h"
#include <memory>

namespace buksan {

class PostgresRecordingRepository final : public IRecordingRepository {
public:
    explicit PostgresRecordingRepository(std::shared_ptr<IConnectionPool> pool);

    std::vector<Recording> findByCameraAndRange(const RecordingQuery& query) override;
    std::optional<Recording> findById(std::int64_t recordingId) override;
    std::int64_t create(const CreateRecordingCommand& command) override;

private:
    std::shared_ptr<IConnectionPool> pool_;
};

} // namespace buksan

#endif // REPOSITORIES_POSTGRES_POSTGRESRECORDINGREPOSITORY_H
