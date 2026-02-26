#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H

#include "ConfigLoader.h"
#include <memory>
#include <vector>

namespace buksan {

class CameraSession;

class CameraManager {
public:
    explicit CameraManager(const AppConfig& config);

    void startAll();
    void stopAll();

private:
    AppConfig config_;
    std::vector<std::shared_ptr<CameraSession>> sessions_;
};

} // namespace buksan

#endif // CAMERAMANAGER_H
