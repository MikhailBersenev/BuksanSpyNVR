#ifndef CONFIGLOADER_H
#define CONFIGLOADER_H

#include <string>
#include <vector>

namespace buksan {

struct CameraConfig {
    std::string id;
    std::string rtsp_url;
    bool record{true};
    bool analytics{false};
};

struct AppConfig {
    std::string storage_path;
    std::vector<CameraConfig> cameras;
};

class ConfigLoader {
public:
    explicit ConfigLoader(const std::string& path);

    const AppConfig& config() const { return config_; }
    bool loaded() const { return loaded_; }
    std::string error() const { return error_; }

private:
    AppConfig config_;
    bool loaded_{false};
    std::string error_;
};

} // namespace buksan

#endif // CONFIGLOADER_H
