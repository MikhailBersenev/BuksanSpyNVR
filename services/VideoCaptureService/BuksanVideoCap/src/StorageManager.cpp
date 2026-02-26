#include "StorageManager.h"
#include <sys/stat.h>
#include <errno.h>

namespace buksan {

StorageManager::StorageManager(const std::string& base_path) : base_path_(base_path) {}

bool StorageManager::ensureDirectory() const {
    if (base_path_.empty()) return false;
    struct stat st;
    if (stat(base_path_.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    if (mkdir(base_path_.c_str(), 0755) == 0) return true;
    return (errno == EEXIST);
}

} // namespace buksan
