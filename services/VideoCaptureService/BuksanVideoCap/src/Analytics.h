#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <opencv2/core.hpp>

namespace buksan {

class Analytics {
public:
    Analytics() = default;

    void processFrame(const cv::Mat& frame);
};

} // namespace buksan

#endif // ANALYTICS_H
