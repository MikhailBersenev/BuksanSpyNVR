#ifndef REPOSITORIES_POSTGRES_POSTGRESCAMERAREPOSITORY_H
#define REPOSITORIES_POSTGRES_POSTGRESCAMERAREPOSITORY_H

#include "db/IConnectionPool.h"
#include "repositories/interfaces/ICameraRepository.h"
#include <memory>

namespace buksan {

class PostgresCameraRepository final : public ICameraRepository {
public:
    explicit PostgresCameraRepository(std::shared_ptr<IConnectionPool> pool);

    std::optional<Camera> findById(std::int64_t cameraId) override;
    std::optional<Camera> findByRtspUrl(const std::string& rtspUrl) override;
    std::vector<Camera> listAll() override;
    std::int64_t create(const RegisterCameraCommand& command) override;

private:
    std::shared_ptr<IConnectionPool> pool_;
};

} // namespace buksan

#endif // REPOSITORIES_POSTGRES_POSTGRESCAMERAREPOSITORY_H
