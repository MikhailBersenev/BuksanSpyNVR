#include "HttpServer.h"
#define CROW_RETURNS_OK_ON_HTTP_OPTIONS_REQUEST
#include <crow.h>
#include <crow/middlewares/cors.h>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace buksan {

namespace {
using json = nlohmann::json;

crow::response jsonResponse(int code, const json& j) {
    crow::response res(code);
    res.set_header("Content-Type", "application/json");
    res.body = j.dump();
    return res;
}

crow::response errorResponse(int code, const std::string& message) {
    return jsonResponse(code, json{{"error", message}});
}
} // namespace

struct HttpServerImpl {
    using App = crow::App<crow::CORSHandler>;
    App app;
};

HttpServer::HttpServer(CameraManager& manager, uint16_t port)
    : manager_(manager)
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
    ([] {
        return jsonResponse(200, json{{"status", "ok"}});
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
    ([this] {
        try {
            auto list = manager_.listCameras();
            json arr = json::array();
            for (const auto& p : list) {
                arr.push_back(json{{"id", p.first}, {"status", p.second}});
            }
            return jsonResponse(200, arr);
        } catch (const std::exception& e) {
            return errorResponse(500, e.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/cameras/<string>")
    .methods("GET"_method)
    ([this](const std::string& id) {
        try {
            if (!manager_.cameraExists(id)) {
                return errorResponse(404, "camera not found");
            }
            std::string status = manager_.getStatus(id);
            return jsonResponse(200, json{{"id", id}, {"status", status}});
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
}

void HttpServer::run() {
    impl_->app.port(port_).multithreaded().run();
}

} // namespace buksan
