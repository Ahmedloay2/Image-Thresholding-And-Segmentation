#pragma once
#include "model/image.hpp"

namespace SegmentationProcessor {

    // OpenCV built-in Mean Shift segmentation
    void meanShiftOpenCV(Image& img, int spatialBandwidth, int colorBandwidth, bool useLUV = false);

    // From-scratch implementation of Mean Shift segmentation
    void meanShiftScratch(Image& img, int spatialBandwidth, int colorBandwidth, bool useLUV = false);

}
