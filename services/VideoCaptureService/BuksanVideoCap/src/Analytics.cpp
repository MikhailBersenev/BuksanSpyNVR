#include "Analytics.h"
#include <iostream>

namespace buksan {

void Analytics::processFrame(const cv::Mat& frame) {
    if (frame.empty()) return;
    (void)frame;
    std::cout << "motion check" << std::endl;
}

} // namespace buksan
