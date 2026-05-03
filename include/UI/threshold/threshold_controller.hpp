#pragma once

/**
 * @file threshold_controller.hpp
 * @brief Controller layer between UI and ThresholdProcessor.
 *
 * SRP: This class owns the Image object and is the only layer
 *      that calls ThresholdProcessor. MainWindow never touches
 *      ThresholdProcessor directly.
 *
 * It also handles the conversion from cv::Mat → QPixmap so the
 * UI layer deals only in Qt types.
 */

#include "model/image.hpp"
#include <QPixmap>
#include <QString>
#include <optional>

class ThresholdController
{
public:
    // ── Image loading ─────────────────────────────────────────────────────────

    /// Load image from disk. Returns false if path is invalid.
    bool loadImage(const QString& path);

    /// True after a valid image has been loaded.
    [[nodiscard]] bool hasImage() const noexcept;

    /// Original image as QPixmap for display.
    [[nodiscard]] QPixmap originalPixmap() const;

    /// Image dimensions as a display string e.g. "1920 × 1080 · BGR"
    [[nodiscard]] QString imageInfo() const;

    // ── Thresholding methods ──────────────────────────────────────────────────

    QPixmap runOptimal();
    QPixmap runOtsu();
    QPixmap runSpectralAuto();
    QPixmap runSpectralManual(int K);
    QPixmap runLocal(int windowSize, double C);

private:
    std::optional<Image> image_;

    /// Convert a CV_8UC1 or CV_8UC3 Mat to QPixmap for display.
    static QPixmap matToPixmap(const cv::Mat& mat);

    /// Invalidate cached results when parameters change.
    void clearThresholdCache();
};
