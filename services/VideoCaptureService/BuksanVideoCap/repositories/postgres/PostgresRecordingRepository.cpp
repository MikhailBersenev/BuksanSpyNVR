#include "repositories/postgres/PostgresRecordingRepository.h"
#include <pqxx/pqxx>
#include <stdexcept>

namespace buksan {

namespace {

Recording mapRecording(const pqxx::row& row) {
    Recording recording;
    recording.recordId = row["recordid"].as<std::int64_t>();
    recording.userId = row["user_id"].as<std::int64_t>();
    recording.unixTime = row["unixtime"].as<std::int64_t>();
    recording.mediaFile = row["mediafile"].c_str();
    if (!row["alert_id"].is_null()) {
        recording.alertId = row["alert_id"].as<std::int64_t>();
    }
    recording.deviceId = row["device"].as<std::int64_t>();
    recording.timeValue = row["time"].c_str();
    recording.dateValue = row["date"].c_str();
    if (!row["mandatorymark"].is_null()) {
        recording.mandatoryMark = row["mandatorymark"].as<std::int64_t>();
    }
    return recording;
}

} // namespace

PostgresRecordingRepository::PostgresRecordingRepository(std::shared_ptr<IConnectionPool> pool)
    : pool_(std::move(pool)) {
    if (!pool_) {
        throw std::invalid_argument("PostgresRecordingRepository requires a connection pool");
    }
}

std::vector<Recording> PostgresRecordingRepository::findByCameraAndRange(const RecordingQuery& query) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "SELECT recordid, \"user\" AS user_id, unixtime, mediafile, alert AS alert_id, "
        "device, \"time\", \"date\", mandatorymark "
        "FROM recordings "
        "WHERE device = $1 AND unixtime BETWEEN $2 AND $3 "
        "ORDER BY unixtime ASC",
        query.cameraId,
        query.fromUnix,
        query.toUnix);

    std::vector<Recording> recordings;
    recordings.reserve(result.size());
    for (const auto& row : result) {
        recordings.push_back(mapRecording(row));
    }
    return recordings;
}

std::optional<Recording> PostgresRecordingRepository::findById(std::int64_t recordingId) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::read_transaction tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "SELECT recordid, \"user\" AS user_id, unixtime, mediafile, alert AS alert_id, "
        "device, \"time\", \"date\", mandatorymark "
        "FROM recordings WHERE recordid = $1",
        recordingId);

    if (result.empty()) {
        return std::nullopt;
    }
    return mapRecording(result.front());
}

std::int64_t PostgresRecordingRepository::create(const CreateRecordingCommand& command) {
    auto connection = pool_->acquire();
    PooledConnection lease(pool_, connection);
    pqxx::work tx(lease.get());

    const pqxx::result result = tx.exec_params(
        "INSERT INTO recordings (\"user\", unixtime, mediafile, alert, device, \"time\", \"date\", mandatorymark) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
        "RETURNING recordid",
        command.userId,
        command.unixTime,
        command.mediaFile,
        command.alertId,
        command.deviceId,
        command.timeValue,
        command.dateValue,
        command.mandatoryMark);

    tx.commit();
    return result.front()["recordid"].as<std::int64_t>();
}

} // namespace buksan
