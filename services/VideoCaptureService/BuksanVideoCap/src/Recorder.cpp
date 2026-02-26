#include "Recorder.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace buksan {

namespace fs = std::filesystem;

Recorder::Recorder(const std::string& cameraId,
                   const std::string& storagePath,
                   int segmentDurationSeconds,
                   double fps,
                   cv::Size frameSize)
    : camera_id_(cameraId)
    , storage_path_(storagePath)
    , segment_duration_sec_(segmentDurationSeconds)
    , fps_(fps)
    , frame_size_(frameSize)
{
    if (segment_duration_sec_ <= 0) {
        throw std::runtime_error("Recorder: segmentDurationSeconds must be positive");
    }
    if (fps_ <= 0.0 || frame_size_.width <= 0 || frame_size_.height <= 0) {
        throw std::runtime_error("Recorder: invalid fps or frame size");
    }

    fs::path dir = fs::path(storage_path_) / camera_id_;
    if (!fs::exists(dir)) {
        if (!fs::create_directories(dir)) {
            throw std::runtime_error("Recorder: failed to create directory " + dir.string());
        }
    }

    segment_start_ = std::chrono::steady_clock::now();
    openNextSegment();
    if (!writer_ || !writer_->isOpened()) {
        throw std::runtime_error("Recorder: failed to open first segment");
    }
}

Recorder::~Recorder() {
    stop();
}

void Recorder::closeSegment() {
    if (writer_) {
        writer_->release();
        writer_.reset();
    }
}

void Recorder::openNextSegment() {
    std::string path = makeSegmentPath();
    writer_ = std::make_unique<cv::VideoWriter>();
    writer_->open(path, cv::VideoWriter::fourcc('X', '2', '6', '4'), fps_, frame_size_);
    if (!writer_->isOpened()) {
        throw std::runtime_error("Recorder: failed to open segment " + path);
    }
    segment_start_ = std::chrono::steady_clock::now();
}

std::string Recorder::makeSegmentPath() const {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&t);
    if (!local) {
        throw std::runtime_error("Recorder: localtime failed");
    }
    std::ostringstream os;
    os << std::put_time(local, "%Y-%m-%d_%H-%M-%S") << ".mkv";
    fs::path dir = fs::path(storage_path_) / camera_id_;
    return (dir / os.str()).string();
}

void Recorder::writeFrame(const cv::Mat& frame) {
    if (frame.empty()) return;

    std::lock_guard<std::mutex> lock(mutex_);
    if (stopped_.load()) return;
    if (!writer_ || !writer_->isOpened()) return;

    auto elapsed = std::chrono::steady_clock::now() - segment_start_;
    auto limit = std::chrono::seconds(segment_duration_sec_);
    if (elapsed >= limit) {
        closeSegment();
        if (stopped_.load()) return;
        openNextSegment();
    }

    if (writer_ && writer_->isOpened()) {
        writer_->write(frame);
    }
}

void Recorder::stop() {
    bool expected = false;
    if (!stopped_.compare_exchange_strong(expected, true)) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    closeSegment();
}

void Recorder::startNewSegment() {
    std::lock_guard<std::mutex> lock(mutex_);
    closeSegment();
    stopped_.store(false);
    openNextSegment();
}

bool Recorder::isRecording() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return !stopped_.load() && writer_ && writer_->isOpened();
}

} // namespace buksan
