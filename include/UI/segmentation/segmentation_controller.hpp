#pragma once

/**
 * @file segmentation_controller.hpp
 * @brief Controller that owns the Image and delegates to KMeans / RegionGrowing processors.
 *
 * SRP: MainWindow only handles UI events.
 *      This controller handles all segmentation logic.
 *      Processors are never called directly from MainWindow.
 */

#include "model/image.hpp"
#include "processors/kmeans_processor.hpp"
#include "processors/region_growing_processor.hpp"
#include "processors/segmentation_processor.hpp"
#include "processors/agglomerative_processor.hpp"

#include <opencv2/core.hpp>
#include <vector>
#include <QString>
#include <QPixmap>

class SegmentationController
{
public:
    // ── Image loading ─────────────────────────────────────────────────────────

    /// Load an image from disk. Returns true on success.
    bool loadImage(const QString& path);

    /// True if an image has been loaded.
    [[nodiscard]] bool hasImage() const noexcept { return !image_.mat.empty(); }

    /// Original image as a QPixmap for display.
    [[nodiscard]] QPixmap originalPixmap() const;

    // ── k-Means ───────────────────────────────────────────────────────────────

    /// Run k-Means++ segmentation. Returns result pixmap or null on failure.
    QPixmap runKMeans(int                    k,
                      KMeansProcessor::Space space,
                      float                  xyWeight);

    // ── Region Growing ────────────────────────────────────────────────────────

    /// Add a seed point (pixel coords in the original image).
    void addSeed(int x, int y);

    /// Remove all seeds.
    void clearSeeds();

    /// Current seed count.
    [[nodiscard]] int seedCount() const noexcept
    { return static_cast<int>(seeds_.size()); }

    /// Run Region Growing using planted seeds. Returns result pixmap or null.
    QPixmap runRegionGrowing(float threshold);

    // ── Mean Shift ────────────────────────────────────────────────────────────

    /// Run Mean Shift segmentation.
    QPixmap runMeanShift(int spatialBandwidth, int colorBandwidth, bool useLUV);

    // ── Agglomerative ─────────────────────────────────────────────────────────

    /// Run Agglomerative clustering segmentation.
    QPixmap runAgglomerative(int targetClusters, bool useLUV);

    // ── Shared ────────────────────────────────────────────────────────────────

    /// Convert an OpenCV BGR mat to a QPixmap.
    static QPixmap matToPixmap(const cv::Mat& mat);

private:
    Image                  image_;
    std::vector<cv::Point> seeds_;
};