#pragma once

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QNativeGestureEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QWheelEvent>
#include <qevent.h>

class GraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    GraphicsView(QWidget *parent = nullptr);
    inline void setZoomFactor(float factor) noexcept
    {
        m_zoomFactor = factor;
    }

    void zoomIn() noexcept;
    void zoomOut() noexcept;

signals:
    void openFilesRequested(const QList<QString> &files);
    void zoomInRequested();
    void zoomOutRequested();

protected:
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::NativeGesture)
            return nativeGestureEvent(static_cast<QNativeGestureEvent *>(event));
        return QGraphicsView::event(event);
    }
    void wheelEvent(QWheelEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private:
    float m_zoomFactor{1.25f};
    bool nativeGestureEvent(QNativeGestureEvent *event) noexcept;
};
