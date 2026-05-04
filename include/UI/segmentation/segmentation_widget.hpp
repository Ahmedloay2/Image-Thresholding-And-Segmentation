#pragma once

/**
 * @file segmentation_widget.hpp
 * @brief Self-contained widget for k-Means and Region Growing segmentation.
 */

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QRadioButton>
#include <QPushButton>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QResizeEvent>
#include <vector>

#include "UI/segmentation/segmentation_controller.hpp"
#include "UI/segmentation/clickable_label.hpp"

class SegmentationWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SegmentationWidget(QWidget* parent = nullptr);

protected:
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onLoadImage();
    void onExportResult();
    void onAlgorithmChanged(int id);
    void onKChanged(int value);
    void onXYWeightChanged(int value);
    void onThresholdChanged(int value);
    void onMSSpatialChanged(int value);
    void onMSColorChanged(int value);
    void onClearSeeds();
    void onPixelClicked(int x, int y);
    void onRun();

private:
    void displayPixmap(QLabel* label, const QPixmap& pm);
    void updateSeedLabel();
    void drawSeedOverlay();

    // ── controller ────────────────────────────────────────────────────────────
    SegmentationController controller_;
    QPixmap                currentResult_;

    // ── toolbar ───────────────────────────────────────────────────────────────
    QPushButton* loadBtn_   = nullptr;
    QPushButton* exportBtn_ = nullptr;

    // ── image panels ──────────────────────────────────────────────────────────
    ClickableLabel* originalLabel_ = nullptr;
    QLabel*         resultLabel_   = nullptr;

    // ── algorithm selector ────────────────────────────────────────────────────
    QButtonGroup* algoGroup_ = nullptr;
    QRadioButton* kmRadio_   = nullptr;
    QRadioButton* rgRadio_   = nullptr;
    QRadioButton* msRadio_   = nullptr;
    QRadioButton* aggRadio_  = nullptr;

    // ── stacked param panels ──────────────────────────────────────────────────
    QStackedWidget* paramStack_ = nullptr;

    // k-Means controls
    QSpinBox*     kSpin_          = nullptr;
    QRadioButton* rgbRadio_       = nullptr;
    QRadioButton* rgbXyRadio_     = nullptr;
    QSlider*      xyWeightSlider_ = nullptr;
    QLabel*       xyWeightLabel_  = nullptr;
    QWidget*      xyWeightRow_    = nullptr;

    // Region Growing controls
    QSlider*     thresholdSlider_ = nullptr;
    QLabel*      thresholdLabel_  = nullptr;
    QLabel*      seedCountLabel_  = nullptr;
    QPushButton* clearSeedsBtn_   = nullptr;

    // Mean Shift controls
    QSlider*      msSpatialSlider_ = nullptr;
    QLabel*       msSpatialLabel_  = nullptr;
    QSlider*      msColorSlider_   = nullptr;
    QLabel*       msColorLabel_    = nullptr;
    QRadioButton* msRgbRadio_      = nullptr;
    QRadioButton* msLuvRadio_      = nullptr;

    // Agglomerative controls
    QSpinBox*     aggClustersSpin_ = nullptr;
    QRadioButton* aggRgbRadio_     = nullptr;
    QRadioButton* aggLuvRadio_     = nullptr;

    // ── run button ────────────────────────────────────────────────────────────
    QPushButton* runBtn_ = nullptr;

    // ── state ─────────────────────────────────────────────────────────────────
    QPixmap             baseOriginalPixmap_;
    std::vector<QPoint> seedPoints_;
};