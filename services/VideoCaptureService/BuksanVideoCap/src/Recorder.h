#ifndef RECORDER_H
#define RECORDER_H

#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

namespace buksan {

class Recorder {
public:
    Recorder(const std::string& cameraId,
             const std::string& storagePath,
             int segmentDurationSeconds,
             double fps,
             cv::Size frameSize);

    ~Recorder();

    void writeFrame(const cv::Mat& frame);
    void stop();
    void startNewSegment();

    bool isRecording() const;

private:
    void closeSegment();
    void openNextSegment();
    std::string makeSegmentPath() const;

    std::string camera_id_;
    std::string storage_path_;
    int segment_duration_sec_;
    double fps_;
    cv::Size frame_size_;

    std::unique_ptr<cv::VideoWriter> writer_;
    std::chrono::steady_clock::time_point segment_start_;
    mutable std::mutex mutex_;
    std::atomic<bool> stopped_{false};
};

} // namespace buksan

#endif // RECORDER_H
