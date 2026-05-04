#include "UI/segmentation/segmentation_controller.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <QPixmap>
#include <QImage>
#include <QString>

// ── Image loading ─────────────────────────────────────────────────────────────

bool SegmentationController::loadImage(const QString& path)
{
    image_.mat = cv::imread(path.toStdString());
    if (image_.mat.empty()) return false;
    image_.clearCache();
    seeds_.clear();
    return true;
}

QPixmap SegmentationController::originalPixmap() const
{
    if (image_.mat.empty()) return {};
    return matToPixmap(image_.mat);
}

// ── k-Means ───────────────────────────────────────────────────────────────────

QPixmap SegmentationController::runKMeans(int                    k,
                                          KMeansProcessor::Space space,
                                          float                  xyWeight)
{
    if (!hasImage()) return {};
    KMeansProcessor::kMeansScratch(image_, k, space, xyWeight);
    return matToPixmap(image_.get("kmeans_scratch"));
}

// ── Region Growing ────────────────────────────────────────────────────────────

void SegmentationController::addSeed(int x, int y)
{
    seeds_.push_back({x, y});
}

void SegmentationController::clearSeeds()
{
    seeds_.clear();
}

QPixmap SegmentationController::runRegionGrowing(float threshold)
{
    if (!hasImage() || seeds_.empty()) return {};
    RegionGrowingProcessor::regionGrowingScratch(image_, seeds_, threshold);
    return matToPixmap(image_.get("region_growing_scratch"));
}

// ── Mean Shift ────────────────────────────────────────────────────────────

QPixmap SegmentationController::runMeanShift(int spatialBandwidth, int colorBandwidth, bool useLUV)
{
    if (!hasImage()) return {};
    SegmentationProcessor::meanShiftScratch(image_, spatialBandwidth, colorBandwidth, useLUV);
    return matToPixmap(image_.get("mean_shift_scratch"));
}

// ── Agglomerative ─────────────────────────────────────────────────────────

QPixmap SegmentationController::runAgglomerative(int targetClusters, bool useLUV)
{
    if (!hasImage()) return {};
    AgglomerativeProcessor::agglomerativeScratch(image_, targetClusters, useLUV);
    return matToPixmap(image_.get("agglomerative_scratch"));
}

// ── Shared ────────────────────────────────────────────────────────────────────

QPixmap SegmentationController::matToPixmap(const cv::Mat& mat)
{
    if (mat.empty()) return {};
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    QImage qimg(rgb.data,
                rgb.cols,
                rgb.rows,
                static_cast<int>(rgb.step),
                QImage::Format_RGB888);
    return QPixmap::fromImage(qimg.copy()); // .copy() detaches from cv::Mat memory
}