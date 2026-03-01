#ifndef API_HTTPSERVER_H
#define API_HTTPSERVER_H

#include "../core/CameraManager.h"
#include "services/CameraService.h"
#include "services/NodeService.h"
#include "services/RecordingService.h"
#include <cstdint>
#include <memory>

namespace buksan {

struct HttpServerImpl;

class HttpServer {
public:
    HttpServer(CameraManager& manager,
               RecordingService& recordingService,
               CameraService& cameraService,
               NodeService& nodeService,
               uint16_t port = 8080);
    ~HttpServer();

    void run();
    void setupRoutes();

private:
    CameraManager& manager_;
    RecordingService& recordingService_;
    CameraService& cameraService_;
    NodeService& nodeService_;
    uint16_t port_;
    std::unique_ptr<HttpServerImpl> impl_;
};

} // namespace buksan

#endif // API_HTTPSERVER_H
