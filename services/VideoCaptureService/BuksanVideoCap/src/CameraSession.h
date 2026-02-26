#ifndef CAMERASESSION_H
#define CAMERASESSION_H

#include "ConfigLoader.h"
#include <atomic>
#include <memory>
#include <thread>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace buksan {

class Recorder;
class Analytics;

class CameraSession {
public:
    explicit CameraSession(const CameraConfig& config,
                          const std::string& storage_path);

    ~CameraSession();

    void start();
    void stop();
    bool running() const { return running_.load(); }

private:
    void run();
    bool connect();
    void disconnect();

    CameraConfig config_;
    std::string storage_path_;
    std::unique_ptr<Recorder> recorder_;
    std::unique_ptr<Analytics> analytics_;
    cv::VideoCapture capture_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace buksan

#endif // CAMERASESSION_H
