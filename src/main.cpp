#include <QApplication>
#include "UI/mainwindow.hpp"
#include <opencv2/core/utils/logger.hpp>

int main(int argc, char* argv[])
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_WARNING);
    QApplication app(argc, argv);
    MainWindow window;
    window.resize(1280, 800);
    window.show();
    return app.exec();
}