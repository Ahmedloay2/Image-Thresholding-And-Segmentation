#pragma once

#include <QLabel>
#include <QMouseEvent>
#include <QPoint>
#include <functional>

class ClickableLabel : public QLabel
{
public:
    explicit ClickableLabel(QWidget* parent = nullptr)
        : QLabel(parent)
    {
        setMouseTracking(false);
    }

    void setImageSize(int w, int h) { imgW_ = w; imgH_ = h; }

    // Instead of a signal, use a callback
    void setPixelClickedCallback(std::function<void(int, int)> cb)
    {
        callback_ = std::move(cb);
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() != Qt::LeftButton || imgW_ <= 0 || imgH_ <= 0) {
            QLabel::mousePressEvent(event);
            return;
        }

        const QSize labelSz = size();
        const float scaleW  = static_cast<float>(labelSz.width())  / imgW_;
        const float scaleH  = static_cast<float>(labelSz.height()) / imgH_;
        const float scale   = std::min(scaleW, scaleH);

        const int pmW     = static_cast<int>(imgW_ * scale);
        const int pmH     = static_cast<int>(imgH_ * scale);
        const int offsetX = (labelSz.width()  - pmW) / 2;
        const int offsetY = (labelSz.height() - pmH) / 2;

        const int lx = event->pos().x() - offsetX;
        const int ly = event->pos().y() - offsetY;

        if (lx < 0 || ly < 0 || lx >= pmW || ly >= pmH) {
            QLabel::mousePressEvent(event);
            return;
        }

        const int imgX = static_cast<int>(lx / scale);
        const int imgY = static_cast<int>(ly / scale);

        if (callback_) callback_(imgX, imgY);
        QLabel::mousePressEvent(event);
    }

private:
    int imgW_ = 0;
    int imgH_ = 0;
    std::function<void(int, int)> callback_;
};