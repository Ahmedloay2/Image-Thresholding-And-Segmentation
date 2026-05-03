/**
 * @file threshold_controller.cpp
 * @brief Controller implementation — bridges UI and ThresholdProcessor.
 */

#include "UI/threshold/threshold_controller.hpp"
#include "processors/threshold_processor.hpp"
#include "io/image_handler.hpp"

#include <QImage>
#include <QString>
#include <opencv2/imgproc.hpp>

// ── Image loading ─────────────────────────────────────────────────────────────

bool ThresholdController::loadImage(const QString& path)
{
    try {
        Image loaded = ::loadImage(path.toStdString());
        image_ = std::move(loaded);
        return true;
    }
    catch (...) {
        image_.reset();
        return false;
    }
}

bool ThresholdController::hasImage() const noexcept
{
    return image_.has_value();
}

QPixmap ThresholdController::originalPixmap() const
{
    if (!image_) return {};
    return matToPixmap(image_->mat);
}

QString ThresholdController::imageInfo() const
{
    if (!image_) return {};
    return QString("%1 × %2  ·  BGR")
        .arg(image_->mat.cols)
        .arg(image_->mat.rows);
}

// ── Thresholding methods ──────────────────────────────────────────────────────

QPixmap ThresholdController::runOptimal()
{
    if (!image_) return {};
    // Clear previous optimal result so it recomputes
    // (Image cache is additive — rerunning overwrites the same key)
    ThresholdProcessor::optimalThreshold(*image_);
    return matToPixmap(image_->get("optimal_binary"));
}

QPixmap ThresholdController::runOtsu()
{
    if (!image_) return {};
    ThresholdProcessor::otsuThreshold(*image_);
    return matToPixmap(image_->get("otsu_binary"));
}

QPixmap ThresholdController::runSpectralAuto()
{
    if (!image_) return {};
    ThresholdProcessor::spectralThreshold(*image_, true);
    return matToPixmap(image_->get("spectral_auto_labeled"));
}

QPixmap ThresholdController::runSpectralManual(int K)
{
    if (!image_) return {};
    ThresholdProcessor::spectralThreshold(*image_, false, K);
    return matToPixmap(image_->get("spectral_manual_labeled"));
}

QPixmap ThresholdController::runLocal(int windowSize, double C)
{
    if (!image_) return {};
    // Ensure windowSize is odd — required for symmetric local window
    if (windowSize % 2 == 0) windowSize++;
    ThresholdProcessor::localThreshold(*image_, windowSize, C);
    return matToPixmap(image_->get("local_binary"));
}

// ── Private helpers ───────────────────────────────────────────────────────────

QPixmap ThresholdController::matToPixmap(const cv::Mat& mat)
{
    if (mat.empty()) return {};

    QImage qimg;

    if (mat.type() == CV_8UC1)
    {
        // Grayscale / binary — convert to RGB for Qt display
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
        qimg = QImage(rgb.data,
                      rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy(); // .copy() detaches from Mat memory
    }
    else if (mat.type() == CV_8UC3)
    {
        // BGR — Qt expects RGB
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        qimg = QImage(rgb.data,
                      rgb.cols, rgb.rows,
                      static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    }

    return QPixmap::fromImage(qimg);
}

void ThresholdController::clearThresholdCache()
{
    if (image_) image_->clearCache();
}
