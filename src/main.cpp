#include <iostream>
#include "io/image_handler.hpp"
#include "processors/threshold_processor.hpp"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

int main()
{
    // ── Load image ────────────────────────────────────────────
    Image img = loadImage("D:/sbme/third year/Second Term/Computer Vision/Tasks/Image-Thresholding-And-Segmentation/resources/images/1.jpg");

    // ── Run all 4 methods ─────────────────────────────────────
    ThresholdProcessor::optimalThreshold(img);
    ThresholdProcessor::otsuThreshold(img);
    ThresholdProcessor::spectralThreshold(img, true);
    ThresholdProcessor::spectralThreshold(img, false, 3);
    ThresholdProcessor::localThreshold(img, 31, 5.0);

    // ── Display results ───────────────────────────────────────
    cv::imshow("Original", img.mat);
    cv::imshow("Optimal", img.get("optimal_binary"));
    cv::imshow("Otsu", img.get("otsu_binary"));
    cv::imshow("Spectral Auto", img.get("spectral_auto_labeled"));
    cv::imshow("Spectral Manual", img.get("spectral_manual_labeled"));
    cv::imshow("Local", img.get("local_binary"));

    cv::waitKey(0);   // wait until any key is pressed
    cv::destroyAllWindows();

    return 0;
}