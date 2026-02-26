#include "ConfigLoader.h"
#include <yaml-cpp/yaml.h>
#include <fstream>

namespace buksan {

ConfigLoader::ConfigLoader(const std::string& path) {
    try {
        YAML::Node root = YAML::LoadFile(path);
        if (!root) {
            error_ = "Empty or invalid config file";
            return;
        }
        if (auto sp = root["storage_path"]) {
            config_.storage_path = sp.as<std::string>();
        }
        if (auto cam = root["cameras"]) {
            for (const auto& c : cam) {
                CameraConfig cc;
                if (auto id = c["id"]) cc.id = id.as<std::string>();
                if (auto url = c["rtsp_url"]) cc.rtsp_url = url.as<std::string>();
                if (auto rec = c["record"]) cc.record = rec.as<bool>(true);
                if (auto an = c["analytics"]) cc.analytics = an.as<bool>(false);
                config_.cameras.push_back(std::move(cc));
            }
        }
        loaded_ = true;
    } catch (const YAML::Exception& e) {
        error_ = std::string("YAML: ") + e.what();
    } catch (const std::exception& e) {
        error_ = e.what();
    }
}

} // namespace buksan
