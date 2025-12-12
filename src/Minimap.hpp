#pragma once

#include "OverlayRect.hpp"

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

        // Add items to scene
        m_scene->addItem(m_pix_item);
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

    inline void setClickable(bool clickable) noexcept
    {
        setInteractive(clickable);
    }

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

    inline bool forceHidden() const noexcept
    {
        return m_force_hidden;
    }

    inline void setForceHidden(bool state) noexcept
    {
        m_force_hidden = state;
        setVisible(!state);
    }

    void setMinimapScale(qreal s) noexcept
    {
        QTransform t;
        t.scale(s, s);      // absolute scale
        setTransform(t);    // replace the current transform
        // Set Minimum Size
        QSizeF size = m_pix_item->boundingRect().size() * s;
        setMinimumSize(size.toSize());
        // updateSceneRect();  // adjust the scene rect if needed
    }

    inline void setLocation(Location loc) noexcept
    {
        m_location = loc;
    }

    inline Location location() const noexcept
    {
        return m_location;
    }

signals:
    void minimapClicked(const QPointF &pos);

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        updateSceneRect();
        QGraphicsView::resizeEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override
    {
        emit minimapClicked(mapToScene(event->pos()));
        QGraphicsView::mousePressEvent(event);
    }

private:
    void updateSceneRect()
    {
        if (!m_pix_item->pixmap().isNull())
        {
            m_scene->setSceneRect(m_pix_item->sceneBoundingRect());
            // fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
        }
    }

    bool m_force_hidden{false};
    QGraphicsPixmapItem *m_pix_item{new QGraphicsPixmapItem()};
    QGraphicsScene *m_scene{new QGraphicsScene(this)};
    Location m_location{Location::BOTTOM_RIGHT};
    QPointF m_dragStart;
};
