#pragma once
#include "model/image.hpp"

namespace ThresholdProcessor {

    void optimalThreshold(Image& img);
    void otsuThreshold(Image& img);

    void spectralThreshold(Image& img, bool autoMode, int K = 0);

    void localThreshold(Image& img, int windowSize, double C);
}