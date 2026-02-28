#ifndef CORE_CAMERAMANAGER_H
#define CORE_CAMERAMANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace buksan {

class CameraSession;

struct CameraEntry {
    std::string id;
    std::string rtsp_url;
    std::string storage_path;
    int segment_duration{300};
    std::shared_ptr<CameraSession> session;
};

class CameraManager {
public:
    CameraManager() = default;

    bool addCamera(const std::string& id,
                  const std::string& rtsp_url,
                  const std::string& storage_path,
                  int segment_duration);
    bool removeCamera(const std::string& id);
    bool startRecording(const std::string& id);
    bool stopRecording(const std::string& id);
    std::string getStatus(const std::string& id) const;
    bool cameraExists(const std::string& id) const;
    void stopAll();

    std::vector<std::pair<std::string, std::string>> listCameras() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CameraEntry> cameras_;
};

} // namespace buksan

#endif // CORE_CAMERAMANAGER_H
