#ifndef API_HTTPSERVER_H
#define API_HTTPSERVER_H

#include "../core/CameraManager.h"
#include <cstdint>
#include <memory>

namespace buksan {

struct HttpServerImpl;

class HttpServer {
public:
    explicit HttpServer(CameraManager& manager, uint16_t port = 8080);
    ~HttpServer();

    void run();
    void setupRoutes();

private:
    CameraManager& manager_;
    uint16_t port_;
    std::unique_ptr<HttpServerImpl> impl_;
};

} // namespace buksan

#endif // API_HTTPSERVER_H
