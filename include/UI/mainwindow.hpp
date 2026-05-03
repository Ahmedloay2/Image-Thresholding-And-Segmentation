#pragma once

/**
 * @file mainwindow.hpp
 * @brief Main application window.
 *
 * SRP: MainWindow only handles UI events and display.
 *      All processing is delegated to ThresholdController.
 *      MainWindow never calls ThresholdProcessor directly.
 */

#include <QMainWindow>
#include "UI/threshold/threshold_controller.hpp"
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    // ── Toolbar ──────────────────────────────────────────────────────────────
    void onLoadImage();
    void onRunClicked();

    // ── Method selector ───────────────────────────────────────────────────────
    void onMethodChanged(int index);

    // ── Sliders ───────────────────────────────────────────────────────────────
    void onWindowSizeChanged(int value);
    void onCChanged(int value);
    void onKChanged(int value);

private:
    Ui::MainWindow* ui_;
    ThresholdController controller_;

    // ── UI helpers ────────────────────────────────────────────────────────────

    /// Display a pixmap scaled to fit a QLabel while keeping aspect ratio.
    static void displayPixmap(QLabel* label, const QPixmap& pixmap);

    /// Show/hide the params panel and the correct slider rows for the current method.
    void updateParamsVisibility(int methodIndex);

    /// Re-run the currently selected method and update the result label.
    void runCurrentMethod();

    // ── Method index constants ─────────────────────────────────────────────────
    static constexpr int METHOD_OPTIMAL         = 0;
    static constexpr int METHOD_OTSU            = 1;
    static constexpr int METHOD_SPECTRAL_AUTO   = 2;
    static constexpr int METHOD_SPECTRAL_MANUAL = 3;
    static constexpr int METHOD_LOCAL           = 4;
};
