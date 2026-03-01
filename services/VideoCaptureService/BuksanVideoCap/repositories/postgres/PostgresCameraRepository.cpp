#include "repositories/postgres/PostgresCameraRepository.h"
#include <pqxx/pqxx>
#include <stdexcept>

namespace buksan {

namespace {

Camera mapCamera(const pqxx::row& row) {
    Camera camera;
    camera.deviceId = row["deviceid"].as<std::int64_t>();
    camera.type = row["type"].as<std::int64_t>();
    camera.addDate = row["adddate"].c_str();
    camera.caption = row["caption"].c_str();
    camera.rtspUrl = row["rtsp_url"].c_str();
    if (!row["assigned_node_id"].is_null()) {
        camera.assignedNodeId = row["assigned_node_id"].c_str();
    }
    camera.status = row["status"].c_str();
    return camera;
}

} // namespace

PostgresCameraRepository::PostgresCameraRepository(std::shared_ptr<IConnectionPool> pool)
    : pool_(std::move(pool)) {
    if (!pool_) {
        throw std::invalid_argument("PostgresCameraRepository requires a connection pool");
    }
}

std::optional<Camera> PostgresCameraRepository::findById(std::int64_t cameraId) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "SELECT deviceid, type, adddate, caption, rtsp_url, assigned_node_id, status "
        "FROM devices WHERE deviceid = $1",
        cameraId);

    if (result.empty()) {
        return std::nullopt;
    }
    return mapCamera(result.front());
}

std::optional<Camera> PostgresCameraRepository::findByRtspUrl(const std::string& rtspUrl) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "SELECT deviceid, type, adddate, caption, rtsp_url, assigned_node_id, status "
        "FROM devices WHERE rtsp_url = $1 "
        "ORDER BY deviceid ASC "
        "LIMIT 1",
        rtspUrl);

    if (result.empty()) {
        return std::nullopt;
    }
    return mapCamera(result.front());
}

std::vector<Camera> PostgresCameraRepository::listAll() {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec(
        "SELECT deviceid, type, adddate, caption, rtsp_url, assigned_node_id, status "
        "FROM devices ORDER BY deviceid ASC");

    std::vector<Camera> cameras;
    cameras.reserve(result.size());
    for (const auto& row : result) {
        cameras.push_back(mapCamera(row));
    }
    return cameras;
}

std::int64_t PostgresCameraRepository::create(const RegisterCameraCommand& command) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::work tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "INSERT INTO devices (type, adddate, caption, rtsp_url, assigned_node_id, status) "
        "VALUES ($1, CURRENT_DATE, $2, $3, $4, $5) "
        "RETURNING deviceid",
        command.type,
        command.caption,
        command.rtspUrl,
        command.assignedNodeId,
        command.status);

    tx.commit();
    return result.front()["deviceid"].as<std::int64_t>();
}

} // namespace buksan
