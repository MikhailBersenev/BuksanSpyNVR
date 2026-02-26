#ifndef STORAGEMANAGER_H
#define STORAGEMANAGER_H

#include <string>

namespace buksan {

class StorageManager {
public:
    explicit StorageManager(const std::string& base_path);

    bool ensureDirectory() const;
    std::string basePath() const { return base_path_; }

private:
    std::string base_path_;
};

} // namespace buksan

#endif // STORAGEMANAGER_H
