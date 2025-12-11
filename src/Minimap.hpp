#pragma once

#include <QBrush>
#include <QGraphicsPixmapItem>
#include <QVBoxLayout>
#include <QWidget>
#include <qgraphicsitem.h>
#include <qgraphicsscene.h>
#include <qgraphicsview.h>

class Minimap : public QGraphicsView
{
    Q_OBJECT
public:
    Minimap(QGraphicsView *parent = nullptr) : QGraphicsView(parent)
    {
        setScene(m_scene);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_overlay_rect->setPen(QPen(QColor(255, 0, 0, 100), 5));
        m_overlay_rect->setBrush(QBrush(QColor(255, 0, 0, 75)));
        setFrameShadow(QGraphicsView::Plain);
        setFrameShape(QGraphicsView::NoFrame);
        m_overlay_rect->setZValue(10);
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
        m_scene->setSceneRect(m_pix_item->boundingRect());
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

    void setPixmap(const QPixmap &pix) noexcept
    {
        m_pix_item->setPixmap(pix);
        m_scene->setSceneRect(m_pix_item->boundingRect());
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
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

    inline bool forceHidden() noexcept
    {
        return m_force_hidden;
    }

    inline void setForceHidden(bool state) noexcept
    {
        m_force_hidden = state;
        if (state)
            setVisible(false);
    }

    inline void setOverlayRectBorderWidth(int width) noexcept
    {
        if (width < 0)
            return;

        QPen p = m_overlay_rect->pen();
        p.setWidth(width);
        m_overlay_rect->setPen(p);
    }

    inline void setOverlayRectBorderColor(const QColor &color) noexcept
    {
        QPen p = m_overlay_rect->pen();
        p.setColor(color);
        m_overlay_rect->setPen(p);
    }

    void setMinimapSize(int w, int h)
    {
        setMinimumSize(1, 1); // allow any size
        resize(w, h);         // apply user-requested size
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

    inline void setLocation(Location loc) noexcept
    {
        m_location = loc;
    }

    inline Location location() noexcept
    {
        return m_location;
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QGraphicsView::resizeEvent(event);

        // keep the image fitted to the new widget size
        fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    }

private:
    bool m_force_hidden{false};
    QGraphicsPixmapItem *m_pix_item{new QGraphicsPixmapItem()};
    QGraphicsScene *m_scene{new QGraphicsScene(this)};
    QGraphicsRectItem *m_overlay_rect{new QGraphicsRectItem()};
    Location m_location{Location::BOTTOM_RIGHT};
};
