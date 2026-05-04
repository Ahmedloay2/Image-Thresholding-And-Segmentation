#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QRadioButton>
#include <QPushButton>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QMouseEvent>
#include <QEvent>
#include <vector>

#include "UI/threshold/threshold_controller.hpp"
#include "UI/segmentation/segmentation_controller.hpp"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onLoadImage();
    void onRunClicked();
    void onTabChanged(int index);

    // Threshold slots
    void onMethodChanged(int index);
    void onWindowSizeChanged(int value);
    void onCChanged(int value);
    void onKSpectralChanged(int value);

    // Segmentation slots
    void onKMeansChanged(int value);
    void onXYWeightChanged(int value);
    void onThresholdChanged(int value);
    void onMSSpatialChanged(int value);
    void onMSColorChanged(int value);
    void onClearSeeds();
    void onImageClicked(int x, int y);

private:
    void buildSegmentationPanel();
    void updateParamsVisibility(int methodIndex);
    void runThreshold();
    void runSegmentation();
    void displayPixmap(QLabel* label, const QPixmap& pixmap);
    void updateSeedLabel();
    void drawSeedOverlay();

    Ui::MainWindow* ui_;

    // Segmentation param widgets (built in code, injected into tabSegmentation)
    QStackedWidget* segParamStack_   = nullptr;
    QSpinBox*       kSpin_           = nullptr;
    QRadioButton*   rgbRadio_        = nullptr;
    QRadioButton*   rgbXyRadio_      = nullptr;
    QSlider*        xyWeightSlider_  = nullptr;
    QLabel*         xyWeightLabel_   = nullptr;
    QWidget*        xyWeightRow_     = nullptr;
    QSlider*        thresholdSlider_ = nullptr;
    QLabel*         thresholdLabel_  = nullptr;
    QLabel*         seedCountLabel_  = nullptr;
    QPushButton*    clearSeedsBtn_   = nullptr;
    QButtonGroup*   segAlgoGroup_    = nullptr;
    QRadioButton*   kmRadio_         = nullptr;
    QRadioButton*   rgRadio_         = nullptr;
    QRadioButton*   msRadio_         = nullptr;
    QRadioButton*   aggRadio_        = nullptr;

    // Mean Shift widgets
    QSlider*        msSpatialSlider_ = nullptr;
    QLabel*         msSpatialLabel_  = nullptr;
    QSlider*        msColorSlider_   = nullptr;
    QLabel*         msColorLabel_    = nullptr;
    QRadioButton*   msRgbRadio_      = nullptr;
    QRadioButton*   msLuvRadio_      = nullptr;

    // Agglomerative widgets
    QSpinBox*       aggClustersSpin_ = nullptr;
    QRadioButton*   aggRgbRadio_     = nullptr;
    QRadioButton*   aggLuvRadio_     = nullptr;

    // Controllers
    ThresholdController    threshCtrl_;
    SegmentationController segCtrl_;

    // State
    QPixmap             basePixmap_;
    std::vector<QPoint> seedPoints_;
    bool                imageLoaded_ = false;

    static constexpr int METHOD_OPTIMAL         = 0;
    static constexpr int METHOD_OTSU            = 1;
    static constexpr int METHOD_SPECTRAL_AUTO   = 2;
    static constexpr int METHOD_SPECTRAL_MANUAL = 3;
    static constexpr int METHOD_LOCAL           = 4;
};