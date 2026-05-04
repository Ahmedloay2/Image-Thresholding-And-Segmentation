#include "UI/segmentation/segmentation_widget.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QSizePolicy>
#include <QFrame>
#include <QApplication>
#include <QResizeEvent>


// ── Constructor ───────────────────────────────────────────────────────────────

SegmentationWidget::SegmentationWidget(QWidget* parent)
    : QWidget(parent)
{
    // ── Toolbar ───────────────────────────────────────────────────────────────
    loadBtn_   = new QPushButton("Load Image");
    exportBtn_ = new QPushButton("Export Result");
    exportBtn_->setEnabled(false);

    auto* toolbar = new QHBoxLayout;
    toolbar->addWidget(loadBtn_);
    toolbar->addStretch();
    toolbar->addWidget(exportBtn_);

    // ── Image panels ──────────────────────────────────────────────────────────
    originalLabel_ = new ClickableLabel;
    originalLabel_->setAlignment(Qt::AlignCenter);
    originalLabel_->setMinimumSize(300, 250);
    originalLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    originalLabel_->setFrameShape(QFrame::StyledPanel);
    originalLabel_->setText("Original image");

    resultLabel_ = new QLabel;
    resultLabel_->setAlignment(Qt::AlignCenter);
    resultLabel_->setMinimumSize(300, 250);
    resultLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    resultLabel_->setFrameShape(QFrame::StyledPanel);
    resultLabel_->setText("Result will appear here");

    auto* imagesLayout = new QHBoxLayout;
    imagesLayout->addWidget(originalLabel_);
    imagesLayout->addWidget(resultLabel_);

    // ── Algorithm selector ────────────────────────────────────────────────────
    kmRadio_ = new QRadioButton("k-Means");
    rgRadio_ = new QRadioButton("Region Growing");
    kmRadio_->setChecked(true);

    algoGroup_ = new QButtonGroup(this);
    algoGroup_->addButton(kmRadio_, 0);
    algoGroup_->addButton(rgRadio_, 1);

    auto* algoBox    = new QGroupBox("Algorithm");
    auto* algoLayout = new QHBoxLayout;
    algoLayout->addWidget(kmRadio_);
    algoLayout->addWidget(rgRadio_);
    algoLayout->addStretch();
    algoBox->setLayout(algoLayout);

    // ── k-Means param panel ───────────────────────────────────────────────────
    auto* kmPanel  = new QWidget;
    auto* kmLayout = new QVBoxLayout(kmPanel);
    kmLayout->setContentsMargins(0, 0, 0, 0);

    auto* kRow   = new QHBoxLayout;
    auto* kLabel = new QLabel("k (clusters):");
    kSpin_       = new QSpinBox;
    kSpin_->setRange(1, 32);
    kSpin_->setValue(5);
    kRow->addWidget(kLabel);
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

    xyWeightRow_    = new QWidget;
    auto* xyLayout  = new QHBoxLayout(xyWeightRow_);
    xyLayout->setContentsMargins(0, 0, 0, 0);
    auto* xyLbl     = new QLabel("XY weight:");
    xyWeightSlider_ = new QSlider(Qt::Horizontal);
    xyWeightSlider_->setRange(1, 200);
    xyWeightSlider_->setValue(50);
    xyWeightLabel_  = new QLabel("50");
    xyWeightLabel_->setMinimumWidth(30);
    xyLayout->addWidget(xyLbl);
    xyLayout->addWidget(xyWeightSlider_);
    xyLayout->addWidget(xyWeightLabel_);
    xyWeightRow_->setVisible(false);

    kmLayout->addLayout(kRow);
    kmLayout->addWidget(spaceBox);
    kmLayout->addWidget(xyWeightRow_);

    // ── Region Growing param panel ────────────────────────────────────────────
    auto* rgPanel  = new QWidget;
    auto* rgLayout = new QVBoxLayout(rgPanel);
    rgLayout->setContentsMargins(0, 0, 0, 0);

    auto* thrRow     = new QHBoxLayout;
    auto* thrLabel   = new QLabel("Threshold:");
    thresholdSlider_ = new QSlider(Qt::Horizontal);
    thresholdSlider_->setRange(1, 150);
    thresholdSlider_->setValue(30);
    thresholdLabel_  = new QLabel("30");
    thresholdLabel_->setMinimumWidth(30);
    thrRow->addWidget(thrLabel);
    thrRow->addWidget(thresholdSlider_);
    thrRow->addWidget(thresholdLabel_);

    auto* seedRow   = new QHBoxLayout;
    seedCountLabel_ = new QLabel("Seeds planted: 0");
    clearSeedsBtn_  = new QPushButton("Clear Seeds");
    seedRow->addWidget(seedCountLabel_);
    seedRow->addStretch();
    seedRow->addWidget(clearSeedsBtn_);

    auto* seedHint = new QLabel("-> Click the original image to plant seeds");
    seedHint->setStyleSheet("color: gray; font-style: italic;");

    rgLayout->addLayout(thrRow);
    rgLayout->addLayout(seedRow);
    rgLayout->addWidget(seedHint);

    // ── Stacked widget ────────────────────────────────────────────────────────
    paramStack_ = new QStackedWidget;
    paramStack_->addWidget(kmPanel);  // index 0
    paramStack_->addWidget(rgPanel);  // index 1

    auto* paramsBox    = new QGroupBox("Parameters");
    auto* paramsLayout = new QVBoxLayout;
    paramsLayout->addWidget(paramStack_);
    paramsBox->setLayout(paramsLayout);

    // ── Run button ────────────────────────────────────────────────────────────
    runBtn_ = new QPushButton("Run");
    runBtn_->setFixedHeight(36);
    runBtn_->setEnabled(false);

    // ── Master layout ─────────────────────────────────────────────────────────
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(toolbar);
    mainLayout->addLayout(imagesLayout, 1);
    mainLayout->addWidget(algoBox);
    mainLayout->addWidget(paramsBox);
    mainLayout->addWidget(runBtn_);

    // ── Connections ───────────────────────────────────────────────────────────
    connect(loadBtn_,   &QPushButton::clicked, this, &SegmentationWidget::onLoadImage);
    connect(exportBtn_, &QPushButton::clicked, this, &SegmentationWidget::onExportResult);
    connect(runBtn_,    &QPushButton::clicked, this, &SegmentationWidget::onRun);

    connect(algoGroup_, &QButtonGroup::idClicked,
            this, &SegmentationWidget::onAlgorithmChanged);

    connect(kSpin_, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &SegmentationWidget::onKChanged);

    connect(xyWeightSlider_, &QSlider::valueChanged,
            this, &SegmentationWidget::onXYWeightChanged);

    connect(thresholdSlider_, &QSlider::valueChanged,
            this, &SegmentationWidget::onThresholdChanged);

    connect(clearSeedsBtn_, &QPushButton::clicked,
            this, &SegmentationWidget::onClearSeeds);

    originalLabel_->setPixelClickedCallback([this](int x, int y) {
    onPixelClicked(x, y);
     });

    connect(rgbXyRadio_, &QRadioButton::toggled,
            xyWeightRow_, &QWidget::setVisible);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void SegmentationWidget::onLoadImage()
{
    const QString path = QFileDialog::getOpenFileName(
        this, "Open Image", QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif)");
    if (path.isEmpty()) return;

    if (!controller_.loadImage(path)) {
        QMessageBox::warning(this, "Error", "Failed to load image:\n" + path);
        return;
    }

    baseOriginalPixmap_ = controller_.originalPixmap();
    displayPixmap(originalLabel_, baseOriginalPixmap_);
    originalLabel_->setImageSize(
        baseOriginalPixmap_.width(),
        baseOriginalPixmap_.height());

    resultLabel_->clear();
    resultLabel_->setText("Result will appear here");
    currentResult_ = {};

    controller_.clearSeeds();
    seedPoints_.clear();
    updateSeedLabel();

    runBtn_->setEnabled(true);
    exportBtn_->setEnabled(false);
}

void SegmentationWidget::onExportResult()
{
    if (currentResult_.isNull()) return;
    const QString path = QFileDialog::getSaveFileName(
        this, "Export Result", "result.png",
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;BMP (*.bmp)");
    if (path.isEmpty()) return;
    if (!currentResult_.save(path))
        QMessageBox::warning(this, "Error", "Could not save file:\n" + path);
}

void SegmentationWidget::onAlgorithmChanged(int id)
{
    paramStack_->setCurrentIndex(id);
    if (id == 0) {
        controller_.clearSeeds();
        seedPoints_.clear();
        updateSeedLabel();
        drawSeedOverlay();
    }
}

void SegmentationWidget::onKChanged(int /*value*/) {}

void SegmentationWidget::onXYWeightChanged(int value)
{
    xyWeightLabel_->setText(QString::number(value));
}

void SegmentationWidget::onThresholdChanged(int value)
{
    thresholdLabel_->setText(QString::number(value));
}

void SegmentationWidget::onClearSeeds()
{
    controller_.clearSeeds();
    seedPoints_.clear();
    updateSeedLabel();
    drawSeedOverlay();
}

void SegmentationWidget::onPixelClicked(int x, int y)
{
    if (algoGroup_->checkedId() != 1) return;
    controller_.addSeed(x, y);
    seedPoints_.push_back({x, y});
    updateSeedLabel();
    drawSeedOverlay();
}

void SegmentationWidget::onRun()
{
    if (!controller_.hasImage()) return;

    runBtn_->setEnabled(false);
    runBtn_->setText("Running...");
    QCoreApplication::processEvents();

    QPixmap result;

    if (algoGroup_->checkedId() == 0) {
        const int k = kSpin_->value();
        const KMeansProcessor::Space space = rgbXyRadio_->isChecked()
            ? KMeansProcessor::Space::RGB_XY
            : KMeansProcessor::Space::RGB;
        const float xyWeight = static_cast<float>(xyWeightSlider_->value());
        result = controller_.runKMeans(k, space, xyWeight);
    } else {
        if (controller_.seedCount() == 0) {
            QMessageBox::information(this, "No Seeds",
                "Please click on the image to plant at least one seed point.");
            runBtn_->setEnabled(true);
            runBtn_->setText("Run");
            return;
        }
        const float threshold = static_cast<float>(thresholdSlider_->value());
        result = controller_.runRegionGrowing(threshold);
    }

    runBtn_->setEnabled(true);
    runBtn_->setText("Run");

    if (result.isNull()) {
        QMessageBox::warning(this, "Error", "Segmentation failed.");
        return;
    }

    currentResult_ = result;
    displayPixmap(resultLabel_, result);
    exportBtn_->setEnabled(true);
}

// ── Private helpers ───────────────────────────────────────────────────────────

void SegmentationWidget::displayPixmap(QLabel* label, const QPixmap& pm)
{
    if (pm.isNull()) return;
    label->setPixmap(pm.scaled(label->size(),
                               Qt::KeepAspectRatio,
                               Qt::SmoothTransformation));
}

void SegmentationWidget::updateSeedLabel()
{
    seedCountLabel_->setText(
        QString("Seeds planted: %1").arg(controller_.seedCount()));
}

void SegmentationWidget::drawSeedOverlay()
{
    if (baseOriginalPixmap_.isNull()) return;

    QPixmap overlay = baseOriginalPixmap_.copy();
    QPainter painter(&overlay);
    painter.setRenderHint(QPainter::Antialiasing);

    const QList<QColor> colors = {
        Qt::red, Qt::blue, Qt::green, Qt::yellow,
        Qt::cyan, Qt::magenta, Qt::white, Qt::darkRed
    };

    for (int i = 0; i < static_cast<int>(seedPoints_.size()); ++i) {
        const QPoint& pt = seedPoints_[i];
        QColor col = colors[i % colors.size()];

        painter.setPen(QPen(Qt::white, 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(pt, 8, 8);

        painter.setPen(QPen(col, 1.5));
        painter.setBrush(col);
        painter.drawEllipse(pt, 5, 5);

        painter.setPen(Qt::black);
        painter.setFont(QFont("Arial", 7, QFont::Bold));
        painter.drawText(pt.x() - 3, pt.y() + 3, QString::number(i + 1));
    }

    painter.end();
    displayPixmap(originalLabel_, overlay);
}

void SegmentationWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (!baseOriginalPixmap_.isNull())
        displayPixmap(originalLabel_, baseOriginalPixmap_);
    if (!currentResult_.isNull())
        displayPixmap(resultLabel_, currentResult_);
}
