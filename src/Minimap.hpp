#pragma once

#include "OverlayRect.hpp"

#include <QBrush>
#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPen>
#include <QResizeEvent>
#include <QSize>
#include <QVBoxLayout>
#include <QWidget>
#include <qgraphicsitem.h>
#include <qnamespace.h>

// Custom pixmap item with border support
class BorderedPixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT
public:
    BorderedPixmapItem(QGraphicsItem *parent = nullptr) : QObject(), QGraphicsPixmapItem(parent) {}

    void setBorderColor(const QColor &color)
    {
        m_border_color = color;
        update();
    }

    void setBorderWidth(int width)
    {
        m_border_width = width;
        update();
    }

    void setBorderVisible(bool visible)
    {
        m_border_visible = visible;
        update();
    }

    void setBorder(const QColor &color, int width)
    {
        m_border_color   = color;
        m_border_width   = width;
        m_border_visible = true;
        update();
    }

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override
    {
        // Draw the pixmap first
        QGraphicsPixmapItem::paint(painter, option, widget);

        // Draw border on top if enabled
        if (m_border_visible && m_border_width > 0 && !pixmap().isNull())
        {
            painter->save();
            QPen pen(m_border_color, m_border_width);
            pen.setCosmetic(true);
            painter->setPen(pen);
            painter->setBrush(Qt::NoBrush);

            // Draw border around the pixmap bounds
            QRectF rect     = boundingRect();
            qreal halfWidth = m_border_width / 2.0;
            rect.adjust(halfWidth, halfWidth, -halfWidth, -halfWidth);

            painter->drawRect(rect);
            painter->restore();
        }
    }

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
    {
        emit pixmapClicked(mapToScene(event->pos()));
        QGraphicsPixmapItem::mousePressEvent(event);
    }

signals:
    void pixmapClicked(const QPointF &pos);

private:
    bool m_border_visible{false};
    int m_border_width{5};
    QColor m_border_color{Qt::black};
};

class Minimap : public QGraphicsView
{
    Q_OBJECT
public:
    explicit Minimap(QWidget *parent = nullptr) : QGraphicsView(parent)
    {
        // Scene setup
        setScene(m_scene);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setFrameShape(QFrame::NoFrame);
        setFrameShadow(QFrame::Plain);
        setFrameStyle(QFrame::NoFrame);
        setBackgroundBrush(Qt::NoBrush);
        setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        // setStyleSheet("background: transparent");

        // Add items to scene
        m_scene->addItem(m_pix_item);

        connect(m_pix_item, &BorderedPixmapItem::pixmapClicked, this, &Minimap::minimapClicked);

        // Important: Set viewport to transparent
        viewport()->setAutoFillBackground(false);
        setStyleSheet("QGraphicsView { background: transparent; }");
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

    inline void setMinimapPadding(float padding) noexcept
    {
        m_padding = padding;
    }

    inline float padding() const noexcept
    {
        return m_padding;
    }

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
        m_pix_item->setPos(0, 0); // Ensure pixmap is at origin
        m_pix_item->setBorder(m_border_color, m_border_width);
        m_pix_item->setBorderVisible(true);
        updateSceneRect();
    }

    void setRotation(int angle) noexcept
    {
        m_pix_item->setTransformOriginPoint(m_pix_item->boundingRect().center());
        m_pix_item->setRotation(angle);
        updateSceneRect();
    }

    inline void setPixmapOpacity(qreal opacity) const noexcept
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

    void setMinimapSize(QSize size) noexcept
    {
        setFixedSize(size);
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

    inline void setBorder(const float width, const QColor &color) noexcept
    {
        m_border_width = width;
        m_border_color = color;
        m_pix_item->setBorder(color, width);
    }

signals:
    void minimapClicked(const QPointF &pos);

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        updateSceneRect();
        QGraphicsView::resizeEvent(event);
    }

private:
    float m_padding{10.0f};
    bool m_force_hidden{false};
    float m_border_width{1.0f};
    BorderedPixmapItem *m_pix_item{new BorderedPixmapItem()};
    QColor m_border_color{Qt::black};
    QGraphicsScene *m_scene{new QGraphicsScene(this)};
    Location m_location{Location::BOTTOM_RIGHT};
    QPointF m_dragStart;

    void updateSceneRect() noexcept
    {
        if (m_pix_item->pixmap().isNull())
            return;
        fitInView(m_pix_item, Qt::KeepAspectRatio);
    }
};
