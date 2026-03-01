#include "HttpServer.h"
#define CROW_RETURNS_OK_ON_HTTP_OPTIONS_REQUEST
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace buksan {

namespace {
using json = nlohmann::json;
constexpr std::size_t kStreamChunkSize = 64 * 1024;

crow::response jsonResponse(int code, const json& j) {
    crow::response res(code);
    res.set_header("Content-Type", "application/json");
    res.body = j.dump();
    return res;
}

crow::response errorResponse(int code, const std::string& message) {
    return jsonResponse(code, json{{"error", message}});
}

json toJson(const Recording& recording) {
    json payload{
        {"record_id", recording.recordId},
        {"user", recording.userId},
        {"unixtime", recording.unixTime},
        {"mediafile", recording.mediaFile},
        {"device", recording.deviceId},
        {"time", recording.timeValue},
        {"date", recording.dateValue},
    };
    if (recording.alertId.has_value()) {
        payload["alert"] = recording.alertId.value();
    }
    if (recording.mandatoryMark.has_value()) {
        payload["mandatory_mark"] = recording.mandatoryMark.value();
    }
    return payload;
}

json toJson(const Camera& camera) {
    json payload{
        {"device_id", camera.deviceId},
        {"type", camera.type},
        {"add_date", camera.addDate},
        {"caption", camera.caption},
        {"rtsp_url", camera.rtspUrl},
        {"status", camera.status},
    };
    if (camera.assignedNodeId.has_value()) {
        payload["assigned_node_id"] = camera.assignedNodeId.value();
    }
    return payload;
}

json toJson(const Node& node) {
    return json{
        {"node_id", node.nodeId},
        {"caption", node.caption},
        {"status", node.status},
    };
}

struct ByteRange {
    std::uint64_t start{0};
    std::uint64_t end{0};
};

std::optional<ByteRange> parseRangeHeader(const std::string& headerValue, std::uint64_t fileSize) {
    try {
        if (headerValue.rfind("bytes=", 0) != 0) {
            return std::nullopt;
        }

        const std::string value = headerValue.substr(6);
        const auto dash = value.find('-');
        if (dash == std::string::npos) {
            return std::nullopt;
        }

        const std::string startPart = value.substr(0, dash);
        const std::string endPart = value.substr(dash + 1);

        ByteRange range;
        if (startPart.empty()) {
            if (endPart.empty()) {
                return std::nullopt;
            }
            const std::uint64_t suffixLength = std::stoull(endPart);
            if (suffixLength == 0) {
                return std::nullopt;
            }
            if (suffixLength >= fileSize) {
                range.start = 0;
            } else {
                range.start = fileSize - suffixLength;
            }
            range.end = fileSize - 1;
            return range;
        }

        range.start = std::stoull(startPart);
        if (range.start >= fileSize) {
            return std::nullopt;
        }

        if (endPart.empty()) {
            range.end = fileSize - 1;
        } else {
            range.end = std::stoull(endPart);
            if (range.end >= fileSize) {
                range.end = fileSize - 1;
            }
        }

        if (range.start > range.end) {
            return std::nullopt;
        }
        return range;
    } catch (...) {
        return std::nullopt;
    }
}

void streamFile(const std::string& path, std::uint64_t startOffset, std::uint64_t endOffset, crow::response& res) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        res = errorResponse(404, "media file is missing");
        res.end();
        return;
    }

    input.seekg(static_cast<std::streamoff>(startOffset), std::ios::beg);
    std::uint64_t remaining = endOffset - startOffset + 1;
    std::vector<char> buffer(kStreamChunkSize);

    while (remaining > 0 && input.good()) {
        const std::size_t toRead = static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
        input.read(buffer.data(), static_cast<std::streamsize>(toRead));
        const std::streamsize readBytes = input.gcount();
        if (readBytes <= 0) {
            break;
        }
        res.write(std::string(buffer.data(), static_cast<std::size_t>(readBytes)));
        remaining -= static_cast<std::uint64_t>(readBytes);
    }

    res.end();
}
} // namespace

struct HttpServerImpl {
    using App = crow::App<crow::CORSHandler>;
    App app;
};

HttpServer::HttpServer(CameraManager& manager,
                       RecordingService& recordingService,
                       CameraService& cameraService,
                       NodeService& nodeService,
                       uint16_t port)
    : manager_(manager)
    , recordingService_(recordingService)
    , cameraService_(cameraService)
    , nodeService_(nodeService)
    , port_(port)
    , impl_(std::make_unique<HttpServerImpl>())
{
    impl_->app.get_middleware<crow::CORSHandler>().global().origin("*").methods(crow::HTTPMethod::GET, crow::HTTPMethod::POST, crow::HTTPMethod::DELETE, crow::HTTPMethod::OPTIONS).headers("Content-Type");
    setupRoutes();
}

HttpServer::~HttpServer() = default;

void HttpServer::setupRoutes() {
    auto& app = impl_->app;

    CROW_ROUTE(app, "/api/v1/health")
    ([this] {
        return jsonResponse(200, json{{"status", "ok"}, {"pending_metadata_queue", recordingService_.pendingQueueSize()}});
    });

    CROW_ROUTE(app, "/api/v1/cameras")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        try {
            json body = json::parse(req.body);
            std::string id = body.value("id", "");
            std::string rtsp_url = body.value("rtsp_url", "");
            std::string storage_path = body.value("storage_path", "");
            int segment_duration = body.value("segment_duration", 300);
            if (id.empty() || rtsp_url.empty() || storage_path.empty()) {
                return errorResponse(400, "id, rtsp_url and storage_path are required");
            }
            if (!manager_.addCamera(id, rtsp_url, storage_path, segment_duration)) {
                return errorResponse(400, "duplicate id or invalid parameters");
            }
            return jsonResponse(201, json{{"id", id}});
        } catch (const json::exception& e) {
            return errorResponse(400, std::string("invalid JSON: ") + e.what());
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/cameras")
    .methods("GET"_method)
    ([this](const crow::request&) {
        try {
            const auto list = cameraService_.listAll();
            json arr = json::array();
            for (const auto& camera : list) {
                arr.push_back(toJson(camera));
            }
            return jsonResponse(200, arr);
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/cameras/<string>")
    .methods("GET"_method)
    ([this](const std::string& idAsString) {
        try {
            const std::int64_t id = std::stoll(idAsString);
            const auto camera = cameraService_.findById(id);
            if (!camera.has_value()) {
                return errorResponse(404, "camera not found");
            }
            return jsonResponse(200, toJson(camera.value()));
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/cameras/<string>/start")
    .methods("POST"_method)
    ([this](const std::string& id) {
        try {
            if (!manager_.cameraExists(id)) {
                return errorResponse(404, "camera not found");
            }
            if (!manager_.startRecording(id)) {
                return errorResponse(409, "already running");
            }
            return jsonResponse(200, json{{"id", id}, {"status", "running"}});
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/cameras/<string>/stop")
    .methods("POST"_method)
    ([this](const std::string& id) {
        try {
            if (!manager_.cameraExists(id)) {
                return errorResponse(404, "camera not found");
            }
            if (!manager_.stopRecording(id)) {
                return errorResponse(409, "already stopped");
            }
            return jsonResponse(200, json{{"id", id}, {"status", "stopped"}});
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/cameras/<string>")
    .methods("DELETE"_method)
    ([this](const std::string& id) {
        try {
            if (!manager_.cameraExists(id)) {
                return errorResponse(404, "camera not found");
            }
            if (!manager_.removeCamera(id)) {
                return errorResponse(409, "cannot delete while running");
            }
            return jsonResponse(200, json{{"id", id}, {"deleted", true}});
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/nodes")
    .methods("GET"_method)
    ([this] {
        try {
            const auto nodes = nodeService_.listAll();
            json arr = json::array();
            for (const auto& node : nodes) {
                arr.push_back(toJson(node));
            }
            return jsonResponse(200, arr);
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/recordings")
    .methods("GET"_method)
    ([this](const crow::request& req) {
        try {
            const char* cameraIdRaw = req.url_params.get("camera_id");
            const char* fromRaw = req.url_params.get("from");
            const char* toRaw = req.url_params.get("to");
            if (cameraIdRaw == nullptr || fromRaw == nullptr || toRaw == nullptr) {
                return errorResponse(400, "camera_id, from and to query params are required");
            }

            RecordingQuery query;
            query.cameraId = std::stoll(cameraIdRaw);
            query.fromUnix = std::stoll(fromRaw);
            query.toUnix = std::stoll(toRaw);
            if (query.fromUnix > query.toUnix) {
                return errorResponse(400, "from must be less than or equal to to");
            }

            const auto recordings = recordingService_.findByCameraAndRange(query);
            json arr = json::array();
            for (const auto& recording : recordings) {
                arr.push_back(toJson(recording));
            }
            return jsonResponse(200, arr);
        } catch (const std::invalid_argument&) {
            return errorResponse(400, "camera_id, from and to must be numeric");
        } catch (const std::out_of_range&) {
            return errorResponse(400, "camera_id, from or to is out of range");
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/recordings/<string>")
    .methods("GET"_method)
    ([this](const std::string& idAsString) {
        try {
            const std::int64_t recordId = std::stoll(idAsString);
            const auto recording = recordingService_.findById(recordId);
            if (!recording.has_value()) {
                return errorResponse(404, "recording not found");
            }
            return jsonResponse(200, toJson(recording.value()));
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/recordings")
    .methods("POST"_method)
    ([this](const crow::request& req) {
        try {
            const json payload = json::parse(req.body);
            CreateRecordingCommand command;
            command.userId = payload.at("user").get<std::int64_t>();
            command.unixTime = payload.at("unixtime").get<std::int64_t>();
            command.mediaFile = payload.at("mediafile").get<std::string>();
            command.deviceId = payload.at("device").get<std::int64_t>();
            command.timeValue = payload.at("time").get<std::string>();
            command.dateValue = payload.at("date").get<std::string>();
            if (payload.contains("alert") && !payload.at("alert").is_null()) {
                command.alertId = payload.at("alert").get<std::int64_t>();
            }
            if (payload.contains("mandatory_mark") && !payload.at("mandatory_mark").is_null()) {
                command.mandatoryMark = payload.at("mandatory_mark").get<std::int64_t>();
            }

            const auto result = recordingService_.registerSegment(command);
            if (result.persisted) {
                return jsonResponse(201, json{{"persisted", true}, {"record_id", result.recordId.value()}});
            }

            return jsonResponse(202, json{
                                     {"persisted", false},
                                     {"queued", true},
                                     {"pending_queue_size", recordingService_.pendingQueueSize()},
                                 });
        } catch (const json::exception& e) {
            return errorResponse(400, std::string("invalid json payload: ") + e.what());
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/recordings/<string>/stream")
    .methods("GET"_method)
    ([this](const crow::request& req, crow::response& res, const std::string& idAsString) {
        try {
            const std::int64_t recordId = std::stoll(idAsString);
            const auto recording = recordingService_.findById(recordId);
            if (!recording.has_value()) {
                res = errorResponse(404, "recording not found");
                res.end();
                return;
            }

            const std::string path = recording->mediaFile;
            if (!std::filesystem::exists(path)) {
                res = errorResponse(404, "media file is missing");
                res.end();
                return;
            }

            const std::uint64_t fileSize = std::filesystem::file_size(path);
            if (fileSize == 0) {
                res = errorResponse(404, "media file is empty");
                res.end();
                return;
            }

            std::uint64_t startOffset = 0;
            std::uint64_t endOffset = fileSize - 1;

            const std::string rangeHeader = req.get_header_value("Range");
            if (!rangeHeader.empty()) {
                const auto parsedRange = parseRangeHeader(rangeHeader, fileSize);
                if (!parsedRange.has_value()) {
                    res.code = 416;
                    res.set_header("Content-Range", "bytes */" + std::to_string(fileSize));
                    res.end();
                    return;
                }
                startOffset = parsedRange->start;
                endOffset = parsedRange->end;
                res.code = 206;
                res.set_header("Content-Range",
                               "bytes " + std::to_string(startOffset) + "-" + std::to_string(endOffset) + "/" +
                                   std::to_string(fileSize));
            } else {
                res.code = 200;
            }

            res.set_header("Content-Type", "video/mp4");
            res.set_header("Accept-Ranges", "bytes");
            res.set_header("Transfer-Encoding", "chunked");
            streamFile(path, startOffset, endOffset, res);
        } catch (const std::exception& e) {
            res = errorResponse(500, e.what());
            res.end();
        }
    });
}

void HttpServer::run() {
    impl_->app.port(port_).multithreaded().run();
}

} // namespace buksan
