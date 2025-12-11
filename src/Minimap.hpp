#pragma once

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPen>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QWidget>

class Minimap : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Minimap(QGraphicsView *parent = nullptr) : QGraphicsView(parent)
    {
        // Scene setup
        setScene(m_scene);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setFrameShadow(QFrame::Plain);
        setBackgroundBrush(Qt::NoBrush);
        setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

        // Overlay rectangle
        m_overlay_rect->setPen(QPen(QColor(255, 0, 0, 150), 3));
        m_overlay_rect->setBrush(QColor(255, 0, 0, 75));
        m_overlay_rect->setZValue(10);

        // Add items to scene
        m_scene->addItem(m_pix_item);
        m_scene->addItem(m_overlay_rect);
    }

    enum class Location
    {
        TOP_LEFT = 0,
        TOP_CENTER,
        TOP_RIGHT,
        BOTTOM_LEFT,
        BOTTOM_CENTER,
        BOTTOM_RIGHT,
        CENTER_LEFT,
        CENTER,
        CENTER_RIGHT
    };

    void showOverlayOnly(bool enabled) noexcept
    {
        m_pix_item->setVisible(!enabled);
        updateSceneRect();
    }

    void setPixmap(const QPixmap &pix) noexcept
    {
        m_pix_item->setPixmap(pix);
        updateSceneRect();
    }

    void setRotation(int angle) noexcept
    {
        m_pix_item->setTransformOriginPoint(m_pix_item->boundingRect().center());
        m_pix_item->setRotation(angle);
        updateSceneRect();
    }

    inline void setPixmapOpacity(qreal opacity) noexcept
    {
        m_pix_item->setOpacity(opacity);
    }

    inline void setOverlayRect(const QRectF &rect) noexcept
    {
        m_overlay_rect->setRect(rect);
    }

    inline void setOverlayRectColor(const QColor &color) noexcept
    {
        m_overlay_rect->setBrush(color);
    }

    inline void setOverlayRectBorderWidth(int width) noexcept
    {
        if (width < 0)
            return;
        QPen pen = m_overlay_rect->pen();
        pen.setWidth(width);
        m_overlay_rect->setPen(pen);
    }

    inline void setOverlayRectBorderColor(const QColor &color) noexcept
    {
        QPen pen = m_overlay_rect->pen();
        pen.setColor(color);
        m_overlay_rect->setPen(pen);
    }

    inline bool forceHidden() const noexcept
    {
        return m_force_hidden;
    }

    inline void setForceHidden(bool state) noexcept
    {
        m_force_hidden = state;
        setVisible(!state);
    }

    void setMinimapSize(int w, int h)
    {
        setMinimumSize(1, 1);
        resize(w, h);
        updateSceneRect();
    }

    inline void setLocation(Location loc) noexcept
    {
        m_location = loc;
    }

    inline Location location() const noexcept
    {
        return m_location;
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QGraphicsView::resizeEvent(event);
        updateSceneRect();
    }

private:
    void updateSceneRect()
    {
        if (!m_pix_item->pixmap().isNull() && m_pix_item->isVisible())
        {
            m_scene->setSceneRect(m_pix_item->sceneBoundingRect());
            fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        }
    }

    bool m_force_hidden{false};
    QGraphicsPixmapItem *m_pix_item{new QGraphicsPixmapItem()};
    QGraphicsScene *m_scene{new QGraphicsScene(this)};
    QGraphicsRectItem *m_overlay_rect{new QGraphicsRectItem()};
    Location m_location{Location::BOTTOM_RIGHT};
};
