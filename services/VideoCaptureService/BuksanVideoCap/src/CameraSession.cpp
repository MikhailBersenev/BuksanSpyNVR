#include "CameraSession.h"
#include "Recorder.h"
#include "Analytics.h"
#include <chrono>
#include <iostream>
#include <thread>

namespace buksan {

namespace {
const int reconnect_delay_ms = 1000;
}

CameraSession::CameraSession(const CameraConfig& config,
                             const std::string& storage_path)
    : config_(config)
    , storage_path_(storage_path)
    , analytics_(std::make_unique<Analytics>())
{
}

CameraSession::~CameraSession() {
    stop();
}

void CameraSession::start() {
    if (running_.exchange(true)) return;
    thread_ = std::thread(&CameraSession::run, this);
}

void CameraSession::stop() {
    if (!running_.exchange(false)) return;
    if (thread_.joinable()) thread_.join();
    disconnect();
}

bool CameraSession::connect() {
    if (capture_.isOpened()) return true;
    if (!capture_.open(config_.rtsp_url, cv::CAP_FFMPEG)) {
        std::cout << "[" << config_.id << "] open failed, retry in " << (reconnect_delay_ms / 1000) << "s" << std::endl;
        return false;
    }
    capture_.set(cv::CAP_PROP_BUFFERSIZE, 1);
    std::cout << "[" << config_.id << "] connected" << std::endl;
    return true;
}

void CameraSession::disconnect() {
    if (recorder_) {
        recorder_->stop();
    }
    if (capture_.isOpened()) {
        capture_.release();
        capture_ = cv::VideoCapture();
    }
}

void CameraSession::run() {
    bool writer_started = false;
    double fps = 25.0;

    while (running_.load()) {
        if (!connect()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay_ms));
            continue;
        }

        cv::Mat frame;
        if (!capture_.read(frame)) {
            std::cout << "[" << config_.id << "] read failed, reconnecting" << std::endl;
            disconnect();
            writer_started = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(reconnect_delay_ms));
            continue;
        }

        if (!writer_started && config_.record && !frame.empty() && frame.cols > 0 && frame.rows > 0) {
            fps = capture_.get(cv::CAP_PROP_FPS);
            if (fps <= 0) fps = 25.0;
            if (recorder_) {
                try {
                    recorder_->startNewSegment();
                    writer_started = true;
                    std::cout << "[" << config_.id << "] recording resumed (new segment)" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "[" << config_.id << "] startNewSegment failed: " << e.what() << std::endl;
                }
            } else {
                try {
                    recorder_ = std::make_unique<Recorder>(config_.id, storage_path_, 300, fps, frame.size());
                    writer_started = true;
                    std::cout << "[" << config_.id << "] recording started" << std::endl;
                } catch (const std::exception& e) {
                    std::cout << "[" << config_.id << "] recorder failed: " << e.what() << std::endl;
                }
            }
        }

        if (config_.record && recorder_ && recorder_->isRecording()) {
            try {
                recorder_->writeFrame(frame);
            } catch (const std::exception& e) {
                std::cout << "Camera " << config_.id << ": writeFrame failed: " << e.what() << std::endl;
                writer_started = false;
            }
        }
        if (config_.analytics && analytics_) {
            analytics_->processFrame(frame);
        }
    }

    disconnect();
}

} // namespace buksan
