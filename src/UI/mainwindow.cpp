#include "UI/mainwindow.hpp"
#include "ui/ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPainter>
#include <QCoreApplication>

// ── Constructor ───────────────────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);

    // Intercept mouse clicks on the original image label for seed planting
    ui_->lblOriginalImage->installEventFilter(this);
    ui_->lblOriginalImage->setMouseTracking(true);

    // Build segmentation params and inject them into the empty tabSegmentation
    buildSegmentationPanel();

    // Restore margins
    ui_->rootLayout->setContentsMargins(0, 0, 0, 0);
    ui_->threshLayout->setContentsMargins(0, 0, 0, 0);
    ui_->leftLayout->setContentsMargins(16, 16, 16, 16);
    ui_->rightLayout->setContentsMargins(16, 16, 16, 16);
    ui_->toolbarLayout->setContentsMargins(16, 8, 16, 8);
    ui_->paramsLayout->setContentsMargins(16, 12, 16, 12);
    ui_->rowWindowSizeLayout->setContentsMargins(0, 0, 0, 0);
    ui_->rowCLayout->setContentsMargins(0, 0, 0, 0);
    ui_->rowKLayout->setContentsMargins(0, 0, 0, 0);

    // Shared toolbar connections
    connect(ui_->btnLoadImage, &QPushButton::clicked, this, &MainWindow::onLoadImage);
    connect(ui_->btnRun,       &QPushButton::clicked, this, &MainWindow::onRunClicked);
    connect(ui_->tabWidget,    &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);

    // Threshold control connections
    connect(ui_->cmbMethod, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onMethodChanged);
    connect(ui_->sliderWindowSize, &QSlider::valueChanged, this, &MainWindow::onWindowSizeChanged);
    connect(ui_->sliderC,          &QSlider::valueChanged, this, &MainWindow::onCChanged);
    connect(ui_->sliderK,          &QSlider::valueChanged, this, &MainWindow::onKSpectralChanged);

    // Initial state
    updateParamsVisibility(METHOD_OPTIMAL);
    ui_->tabWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    delete ui_;
}

// ── Build segmentation param panel ───────────────────────────────────────────

void MainWindow::buildSegmentationPanel()
{
    auto* root = new QVBoxLayout(ui_->tabSegmentation);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(8);

    // Algorithm selector
    kmRadio_  = new QRadioButton("k-Means");
    rgRadio_  = new QRadioButton("Region Growing");
    msRadio_  = new QRadioButton("Mean Shift");
    aggRadio_ = new QRadioButton("Agglomerative");
    kmRadio_->setChecked(true);

    segAlgoGroup_ = new QButtonGroup(this);
    segAlgoGroup_->addButton(kmRadio_, 0);
    segAlgoGroup_->addButton(rgRadio_, 1);
    segAlgoGroup_->addButton(msRadio_, 2);
    segAlgoGroup_->addButton(aggRadio_, 3);

    auto* algoBox    = new QGroupBox("Algorithm");
    auto* algoLayout = new QHBoxLayout;
    algoLayout->addWidget(kmRadio_);
    algoLayout->addWidget(rgRadio_);
    algoLayout->addWidget(msRadio_);
    algoLayout->addWidget(aggRadio_);
    algoLayout->addStretch();
    algoBox->setLayout(algoLayout);
    root->addWidget(algoBox);

    // Stacked param panels
    segParamStack_ = new QStackedWidget;

    // 0: k-Means panel
    auto* kmPanel  = new QWidget;
    auto* kmLayout = new QVBoxLayout(kmPanel);
    kmLayout->setContentsMargins(0, 0, 0, 0);

    auto* kRow = new QHBoxLayout;
    kSpin_     = new QSpinBox;
    kSpin_->setRange(1, 32);
    kSpin_->setValue(5);
    kRow->addWidget(new QLabel("k (clusters):"));
    kRow->addWidget(kSpin_);
    kRow->addStretch();

    auto* spaceBox    = new QGroupBox("Feature space");
    auto* spaceLayout = new QHBoxLayout;
    rgbRadio_   = new QRadioButton("RGB");
    rgbXyRadio_ = new QRadioButton("RGB + XY");
    rgbRadio_->setChecked(true);
    spaceLayout->addWidget(rgbRadio_);
    spaceLayout->addWidget(rgbXyRadio_);
    spaceLayout->addStretch();
    spaceBox->setLayout(spaceLayout);

    xyWeightRow_   = new QWidget;
    auto* xyLayout = new QHBoxLayout(xyWeightRow_);
    xyLayout->setContentsMargins(0, 0, 0, 0);
    xyWeightSlider_ = new QSlider(Qt::Horizontal);
    xyWeightSlider_->setRange(1, 200);
    xyWeightSlider_->setValue(50);
    xyWeightLabel_ = new QLabel("50");
    xyWeightLabel_->setMinimumWidth(30);
    xyLayout->addWidget(new QLabel("XY weight:"));
    xyLayout->addWidget(xyWeightSlider_);
    xyLayout->addWidget(xyWeightLabel_);
    xyWeightRow_->setVisible(false);

    kmLayout->addLayout(kRow);
    kmLayout->addWidget(spaceBox);
    kmLayout->addWidget(xyWeightRow_);

    // 1: Region Growing panel
    auto* rgPanel  = new QWidget;
    auto* rgLayout = new QVBoxLayout(rgPanel);
    rgLayout->setContentsMargins(0, 0, 0, 0);

    auto* thrRow = new QHBoxLayout;
    thresholdSlider_ = new QSlider(Qt::Horizontal);
    thresholdSlider_->setRange(1, 150);
    thresholdSlider_->setValue(30);
    thresholdLabel_ = new QLabel("30");
    thresholdLabel_->setMinimumWidth(30);
    thrRow->addWidget(new QLabel("Threshold:"));
    thrRow->addWidget(thresholdSlider_);
    thrRow->addWidget(thresholdLabel_);

    auto* seedRow   = new QHBoxLayout;
    seedCountLabel_ = new QLabel("Seeds planted: 0");
    clearSeedsBtn_  = new QPushButton("Clear Seeds");
    seedRow->addWidget(seedCountLabel_);
    seedRow->addStretch();
    seedRow->addWidget(clearSeedsBtn_);

    auto* hint = new QLabel("\xe2\x86\x92 Click the left image to plant seed points");
    hint->setStyleSheet("color: gray; font-style: italic;");

    rgLayout->addLayout(thrRow);
    rgLayout->addLayout(seedRow);
    rgLayout->addWidget(hint);

    // 2: Mean Shift panel
    auto* msPanel  = new QWidget;
    auto* msLayout = new QVBoxLayout(msPanel);
    msLayout->setContentsMargins(0, 0, 0, 0);

    auto* msSpRow = new QHBoxLayout;
    msSpatialSlider_ = new QSlider(Qt::Horizontal);
    msSpatialSlider_->setRange(1, 100);
    msSpatialSlider_->setValue(15);
    msSpatialLabel_ = new QLabel("15");
    msSpatialLabel_->setMinimumWidth(30);
    msSpRow->addWidget(new QLabel("Spatial BW:"));
    msSpRow->addWidget(msSpatialSlider_);
    msSpRow->addWidget(msSpatialLabel_);

    auto* msColRow = new QHBoxLayout;
    msColorSlider_ = new QSlider(Qt::Horizontal);
    msColorSlider_->setRange(1, 100);
    msColorSlider_->setValue(20);
    msColorLabel_ = new QLabel("20");
    msColorLabel_->setMinimumWidth(30);
    msColRow->addWidget(new QLabel("Color BW:"));
    msColRow->addWidget(msColorSlider_);
    msColRow->addWidget(msColorLabel_);

    auto* msFrameBox = new QGroupBox("Color Frame");
    auto* msFrameLayout = new QHBoxLayout;
    msRgbRadio_ = new QRadioButton("RGB");
    msLuvRadio_ = new QRadioButton("LUV");
    msRgbRadio_->setChecked(true);
    msFrameLayout->addWidget(msRgbRadio_);
    msFrameLayout->addWidget(msLuvRadio_);
    msFrameLayout->addStretch();
    msFrameBox->setLayout(msFrameLayout);

    msLayout->addLayout(msSpRow);
    msLayout->addLayout(msColRow);
    msLayout->addWidget(msFrameBox);

    // 3: Agglomerative panel
    auto* aggPanel  = new QWidget;
    auto* aggLayout = new QVBoxLayout(aggPanel);
    aggLayout->setContentsMargins(0, 0, 0, 0);

    auto* aggKRow = new QHBoxLayout;
    aggClustersSpin_ = new QSpinBox;
    aggClustersSpin_->setRange(1, 32);
    aggClustersSpin_->setValue(5);
    aggKRow->addWidget(new QLabel("k (clusters):"));
    aggKRow->addWidget(aggClustersSpin_);
    aggKRow->addStretch();

    auto* aggFrameBox = new QGroupBox("Color Frame");
    auto* aggFrameLayout = new QHBoxLayout;
    aggRgbRadio_ = new QRadioButton("RGB");
    aggLuvRadio_ = new QRadioButton("LUV");
    aggRgbRadio_->setChecked(true);
    aggFrameLayout->addWidget(aggRgbRadio_);
    aggFrameLayout->addWidget(aggLuvRadio_);
    aggFrameLayout->addStretch();
    aggFrameBox->setLayout(aggFrameLayout);

    aggLayout->addLayout(aggKRow);
    aggLayout->addWidget(aggFrameBox);

    segParamStack_->addWidget(kmPanel);  // 0
    segParamStack_->addWidget(rgPanel);  // 1
    segParamStack_->addWidget(msPanel);  // 2
    segParamStack_->addWidget(aggPanel); // 3
    root->addWidget(segParamStack_);
    root->addStretch();

    // Connections
    connect(segAlgoGroup_, &QButtonGroup::idClicked, this, [this](int id) {
        segParamStack_->setCurrentIndex(id);
        if (id != 1) { // Clear seeds if not in Region Growing
            segCtrl_.clearSeeds();
            seedPoints_.clear();
            updateSeedLabel();
            if (!basePixmap_.isNull())
                displayPixmap(ui_->lblOriginalImage, basePixmap_);
        }
    });

    connect(kSpin_,           QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MainWindow::onKMeansChanged);
    connect(xyWeightSlider_,  &QSlider::valueChanged, this, &MainWindow::onXYWeightChanged);
    connect(thresholdSlider_, &QSlider::valueChanged, this, &MainWindow::onThresholdChanged);
    connect(msSpatialSlider_, &QSlider::valueChanged, this, &MainWindow::onMSSpatialChanged);
    connect(msColorSlider_,   &QSlider::valueChanged, this, &MainWindow::onMSColorChanged);
    connect(clearSeedsBtn_,   &QPushButton::clicked,  this, &MainWindow::onClearSeeds);
    connect(rgbXyRadio_, &QRadioButton::toggled, xyWeightRow_, &QWidget::setVisible);
}

// ── eventFilter — seed planting via clicks on the original image ──────────────

bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui_->lblOriginalImage &&
        event->type() == QEvent::MouseButtonPress &&
        ui_->tabWidget->currentIndex() == 1 &&
        segAlgoGroup_->checkedId() == 1 &&
        imageLoaded_)
    {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton) {
            const QSize sz    = ui_->lblOriginalImage->size();
            const int   imgW  = basePixmap_.width();
            const int   imgH  = basePixmap_.height();
            if (imgW <= 0 || imgH <= 0) return false;

            const float scaleW = static_cast<float>(sz.width())  / imgW;
            const float scaleH = static_cast<float>(sz.height()) / imgH;
            const float scale  = std::min(scaleW, scaleH);
            const int   pmW    = static_cast<int>(imgW * scale);
            const int   pmH    = static_cast<int>(imgH * scale);
            const int   offX   = (sz.width()  - pmW) / 2;
            const int   offY   = (sz.height() - pmH) / 2;
            const int   lx     = me->pos().x() - offX;
            const int   ly     = me->pos().y() - offY;

            if (lx >= 0 && ly >= 0 && lx < pmW && ly < pmH)
                onImageClicked(static_cast<int>(lx / scale),
                               static_cast<int>(ly / scale));
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// ── Shared slots ──────────────────────────────────────────────────────────────

void MainWindow::onLoadImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Image", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif *.webp *.pgm *.ppm);;All Files (*)");
    if (path.isEmpty()) return;

    const bool thOk  = threshCtrl_.loadImage(path);
    const bool segOk = segCtrl_.loadImage(path);

    if (!thOk && !segOk) {
        ui_->lblOriginalImage->setText("Failed to load image.");
        return;
    }

    imageLoaded_ = true;
    basePixmap_  = threshCtrl_.originalPixmap();

    segCtrl_.clearSeeds();
    seedPoints_.clear();
    updateSeedLabel();

    displayPixmap(ui_->lblOriginalImage, basePixmap_);
    ui_->lblImageInfo->setText(threshCtrl_.imageInfo());
    ui_->lblResultImage->clear();
    ui_->lblResultImage->setText("Press  \xe2\x96\xb6 Run  to process");
}

void MainWindow::onRunClicked()
{
    if (!imageLoaded_) return;
    if (ui_->tabWidget->currentIndex() == 0)
        runThreshold();
    else
        runSegmentation();
}

void MainWindow::onTabChanged(int index)
{
    // Hide threshold-specific "Method" selection and vertical separator when in segmentation
    ui_->lblMethodTitle->setVisible(index == 0);
    ui_->cmbMethod->setVisible(index == 0);
    ui_->separator1->setVisible(index == 0);

    // Also hide threshold params so they don't leak into segmentation
    if (index == 1) {
        updateParamsVisibility(-1); // Hides all threshold params
    } else {
        updateParamsVisibility(ui_->cmbMethod->currentIndex());
    }

    if (!basePixmap_.isNull())
        displayPixmap(ui_->lblOriginalImage, basePixmap_);
    ui_->lblResultImage->clear();
    ui_->lblResultImage->setText("Press  \xe2\x96\xb6 Run  to process");
}

// ── Threshold slots ───────────────────────────────────────────────────────────

void MainWindow::onMethodChanged(int index)      { updateParamsVisibility(index); }

void MainWindow::onWindowSizeChanged(int value)
{
    if (value % 2 == 0) { ui_->sliderWindowSize->setValue(value + 1); return; }
    ui_->lblWindowSizeVal->setText(QString::number(value));
}

void MainWindow::onCChanged(int value)           { ui_->lblCVal->setText(QString::number(value)); }
void MainWindow::onKSpectralChanged(int value)   { ui_->lblKVal->setText(QString::number(value)); }

// ── Segmentation slots ────────────────────────────────────────────────────────

void MainWindow::onKMeansChanged(int /*v*/)      {}
void MainWindow::onXYWeightChanged(int value)    { xyWeightLabel_->setText(QString::number(value)); }
void MainWindow::onThresholdChanged(int value)   { thresholdLabel_->setText(QString::number(value)); }
void MainWindow::onMSSpatialChanged(int value)   { msSpatialLabel_->setText(QString::number(value)); }
void MainWindow::onMSColorChanged(int value)     { msColorLabel_->setText(QString::number(value)); }

void MainWindow::onClearSeeds()
{
    segCtrl_.clearSeeds();
    seedPoints_.clear();
    updateSeedLabel();
    if (!basePixmap_.isNull())
        displayPixmap(ui_->lblOriginalImage, basePixmap_);
}

void MainWindow::onImageClicked(int x, int y)
{
    segCtrl_.addSeed(x, y);
    seedPoints_.push_back({x, y});
    updateSeedLabel();
    drawSeedOverlay();
}

// ── Run helpers ───────────────────────────────────────────────────────────────

void MainWindow::runThreshold()
{
    QPixmap result;
    switch (ui_->cmbMethod->currentIndex())
    {
    case METHOD_OPTIMAL:
        result = threshCtrl_.runOptimal(); break;
    case METHOD_OTSU:
        result = threshCtrl_.runOtsu(); break;
    case METHOD_SPECTRAL_AUTO:
        result = threshCtrl_.runSpectralAuto(); break;
    case METHOD_SPECTRAL_MANUAL:
        result = threshCtrl_.runSpectralManual(ui_->sliderK->value()); break;
    case METHOD_LOCAL:
        result = threshCtrl_.runLocal(ui_->sliderWindowSize->value(),
                                      static_cast<double>(ui_->sliderC->value())); break;
    default: break;
    }
    displayPixmap(ui_->lblResultImage, result);
}

void MainWindow::runSegmentation()
{
    QPixmap result;

    int algoId = segAlgoGroup_->checkedId();

    if (algoId == 0) { // k-Means
        const KMeansProcessor::Space space = rgbXyRadio_->isChecked()
            ? KMeansProcessor::Space::RGB_XY
            : KMeansProcessor::Space::RGB;
        result = segCtrl_.runKMeans(kSpin_->value(), space,
                                    static_cast<float>(xyWeightSlider_->value()));
    }
    else if (algoId == 1) { // Region Growing
        if (segCtrl_.seedCount() == 0) {
            QMessageBox::information(this, "No Seeds",
                "Click on the original image to plant at least one seed.");
            return;
        }
        result = segCtrl_.runRegionGrowing(static_cast<float>(thresholdSlider_->value()));
    }
    else if (algoId == 2) { // Mean Shift
        result = segCtrl_.runMeanShift(msSpatialSlider_->value(),
                                       msColorSlider_->value(),
                                       msLuvRadio_->isChecked());
    }
    else if (algoId == 3) { // Agglomerative
        result = segCtrl_.runAgglomerative(aggClustersSpin_->value(),
                                           aggLuvRadio_->isChecked());
    }

    if (result.isNull()) {
        QMessageBox::warning(this, "Error", "Segmentation failed.");
        return;
    }
    displayPixmap(ui_->lblResultImage, result);
}

// ── UI helpers ────────────────────────────────────────────────────────────────

void MainWindow::displayPixmap(QLabel* label, const QPixmap& pixmap)
{
    if (pixmap.isNull()) return;
    label->setPixmap(pixmap.scaled(label->size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation));
}

void MainWindow::updateParamsVisibility(int idx)
{
    bool isLocal   = (idx == METHOD_LOCAL);
    bool isSpecMan = (idx == METHOD_SPECTRAL_MANUAL);

    ui_->lblWindowSize->setVisible(isLocal);
    ui_->sliderWindowSize->setVisible(isLocal);
    ui_->lblWindowSizeVal->setVisible(isLocal);
    ui_->lblC->setVisible(isLocal);
    ui_->sliderC->setVisible(isLocal);
    ui_->lblCVal->setVisible(isLocal);
    ui_->lblK->setVisible(isSpecMan);
    ui_->sliderK->setVisible(isSpecMan);
    ui_->lblKVal->setVisible(isSpecMan);
}

void MainWindow::updateSeedLabel()
{
    if (seedCountLabel_)
        seedCountLabel_->setText(
            QString("Seeds planted: %1").arg(segCtrl_.seedCount()));
}

void MainWindow::drawSeedOverlay()
{
    if (basePixmap_.isNull()) return;
    QPixmap overlay = basePixmap_.copy();
    QPainter p(&overlay);
    p.setRenderHint(QPainter::Antialiasing);

    const QList<QColor> colors = {
        Qt::red, Qt::blue, Qt::green, Qt::yellow,
        Qt::cyan, Qt::magenta, Qt::white, Qt::darkRed
    };

    for (int i = 0; i < static_cast<int>(seedPoints_.size()); ++i) {
        const QPoint& pt  = seedPoints_[i];
        QColor        col = colors[i % colors.size()];
        p.setPen(QPen(Qt::white, 3));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pt, 8, 8);
        p.setPen(QPen(col, 1.5));
        p.setBrush(col);
        p.drawEllipse(pt, 5, 5);
        p.setPen(Qt::black);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(pt.x() - 3, pt.y() + 3, QString::number(i + 1));
    }
    p.end();
    displayPixmap(ui_->lblOriginalImage, overlay);
}