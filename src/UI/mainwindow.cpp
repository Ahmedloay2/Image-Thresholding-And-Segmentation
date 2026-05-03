/**
 * @file mainwindow.cpp
 * @brief MainWindow implementation — UI events and display only.
 */

#include "UI/mainwindow.hpp"
#include "ui/ui_mainwindow.h"

#include <QFileDialog>
#include <QLabel>

 // ── Constructor / Destructor ──────────────────────────────────────────────────

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui_(new Ui::MainWindow)
{
    ui_->setupUi(this);
    // Set margins in code — avoids Qt 6.10 uic QRect/QMargins bug
    ui_->rootLayout->setContentsMargins(0, 0, 0, 0);
    ui_->threshLayout->setContentsMargins(0, 0, 0, 0);
    ui_->leftLayout->setContentsMargins(16, 16, 16, 16);
    ui_->rightLayout->setContentsMargins(16, 16, 16, 16);
    ui_->toolbarLayout->setContentsMargins(16, 8, 16, 8);
    ui_->paramsLayout->setContentsMargins(16, 12, 16, 12);
    ui_->rowWindowSizeLayout->setContentsMargins(0, 0, 0, 0);
    ui_->rowCLayout->setContentsMargins(0, 0, 0, 0);
    ui_->rowKLayout->setContentsMargins(0, 0, 0, 0);
    // ── Connect toolbar ───────────────────────────────────────────────────────
    connect(ui_->btnLoadImage, &QPushButton::clicked,
        this, &MainWindow::onLoadImage);

    // ── Connect Run button ────────────────────────────────────────────────────
    connect(ui_->btnRun, &QPushButton::clicked,
        this, &MainWindow::onRunClicked);

    // ── Connect method selector ───────────────────────────────────────────────
    connect(ui_->cmbMethod, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &MainWindow::onMethodChanged);

    // ── Connect sliders ───────────────────────────────────────────────────────
    connect(ui_->sliderWindowSize, &QSlider::valueChanged,
        this, &MainWindow::onWindowSizeChanged);
    connect(ui_->sliderC, &QSlider::valueChanged,
        this, &MainWindow::onCChanged);
    connect(ui_->sliderK, &QSlider::valueChanged,
        this, &MainWindow::onKChanged);

    // ── Initial UI state ──────────────────────────────────────────────────────
    updateParamsVisibility(METHOD_OPTIMAL);
}

MainWindow::~MainWindow()
{
    delete ui_;
}

// ── Toolbar slots ─────────────────────────────────────────────────────────────

void MainWindow::onLoadImage()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        "Open Image",
        QString(),
        "Images (*.png *.jpg *.jpeg *.bmp *.tiff *.tif *.webp *.pgm *.ppm);;All Files (*)"
    );

    if (path.isEmpty()) return;

    if (!controller_.loadImage(path)) {
        ui_->lblOriginalImage->setText("Failed to load image.");
        return;
    }

    displayPixmap(ui_->lblOriginalImage, controller_.originalPixmap());
    ui_->lblImageInfo->setText(controller_.imageInfo());
    ui_->lblResultImage->setText("Press  ▶ Run  to process");
}

// ── Method selector slot ──────────────────────────────────────────────────────

void MainWindow::onMethodChanged(int index)
{
    updateParamsVisibility(index);
    // Do NOT auto-run — user presses Run explicitly
}

// ── Slider slots ──────────────────────────────────────────────────────────────

void MainWindow::onWindowSizeChanged(int value)
{
    if (value % 2 == 0) {
        ui_->sliderWindowSize->setValue(value + 1);
        return;
    }
    ui_->lblWindowSizeVal->setText(QString::number(value));
}

void MainWindow::onCChanged(int value)
{
    ui_->lblCVal->setText(QString::number(value));
}

void MainWindow::onKChanged(int value)
{
    ui_->lblKVal->setText(QString::number(value));
}

// ── Run button slot ───────────────────────────────────────────────────────────

void MainWindow::onRunClicked()
{
    runCurrentMethod();
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MainWindow::displayPixmap(QLabel* label, const QPixmap& pixmap)
{
    if (pixmap.isNull()) return;
    label->setPixmap(
        pixmap.scaled(label->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation)
    );
}

void MainWindow::updateParamsVisibility(int methodIndex)
{
    bool isLocal = (methodIndex == METHOD_LOCAL);
    bool isSpectralManual = (methodIndex == METHOD_SPECTRAL_MANUAL);

    // Show/hide WindowSize row widgets
    ui_->lblWindowSize->setVisible(isLocal);
    ui_->sliderWindowSize->setVisible(isLocal);
    ui_->lblWindowSizeVal->setVisible(isLocal);

    // Show/hide C row widgets
    ui_->lblC->setVisible(isLocal);
    ui_->sliderC->setVisible(isLocal);
    ui_->lblCVal->setVisible(isLocal);

    // Show/hide K row widgets
    ui_->lblK->setVisible(isSpectralManual);
    ui_->sliderK->setVisible(isSpectralManual);
    ui_->lblKVal->setVisible(isSpectralManual);
}

void MainWindow::runCurrentMethod()
{
    if (!controller_.hasImage()) return;

    QPixmap result;
    int method = ui_->cmbMethod->currentIndex();

    switch (method)
    {
    case METHOD_OPTIMAL:
        result = controller_.runOptimal();
        break;

    case METHOD_OTSU:
        result = controller_.runOtsu();
        break;

    case METHOD_SPECTRAL_AUTO:
        result = controller_.runSpectralAuto();
        break;

    case METHOD_SPECTRAL_MANUAL:
        result = controller_.runSpectralManual(
            ui_->sliderK->value()
        );
        break;

    case METHOD_LOCAL:
        result = controller_.runLocal(
            ui_->sliderWindowSize->value(),
            static_cast<double>(ui_->sliderC->value())
        );
        break;

    default:
        break;
    }

    displayPixmap(ui_->lblResultImage, result);
}