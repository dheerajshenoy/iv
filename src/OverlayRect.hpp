#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QScrollBar>
#include <QVariant>
#include <qgraphicsitem.h>
#include <qscopedpointer.h>

class OverlayRect : public QObject, public QGraphicsRectItem
{
    Q_OBJECT
public:
    OverlayRect(QGraphicsItem *parent = nullptr) noexcept : QObject(), QGraphicsRectItem(parent)
    {
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setAcceptedMouseButtons(Qt::LeftButton);
        setFlag(ItemSendsGeometryChanges);
    }

    inline void setImageItem(QGraphicsItem *img) noexcept
    {
        m_imageItem = img;
    }

    inline void setMainView(QGraphicsView *view) noexcept
    {
        m_view = view;
    }

    inline void setClickable(bool clickable) noexcept
    {
        setFlag(QGraphicsItem::ItemIsMovable, clickable);
    }

protected:
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override
    {
        QGraphicsRectItem::mouseMoveEvent(event);
        emit overlayMoved();
    }

    // Use mouse press, move and release events to move this item
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override
    {
        if (change == ItemPositionChange && scene())
        {

            // value is the new position.
            QPointF newPos     = value.toPointF();
            QRectF imgRect     = m_imageItem->sceneBoundingRect();
            QRectF overlayRect = this->sceneBoundingRect(); // overlay’s current size in scene coords
            QPointF delta      = newPos - pos();            // proposed move delta

            // Compute proposed new scene rect
            QRectF newSceneRect = overlayRect.translated(delta);

            // Clamp
            if (newSceneRect.left() < imgRect.left())
                newPos.setX(newPos.x() + imgRect.left() - newSceneRect.left());
            if (newSceneRect.top() < imgRect.top())
                newPos.setY(newPos.y() + imgRect.top() - newSceneRect.top());
            if (newSceneRect.right() > imgRect.right())
                newPos.setX(newPos.x() + imgRect.right() - newSceneRect.right());
            if (newSceneRect.bottom() > imgRect.bottom())
                newPos.setY(newPos.y() + imgRect.bottom() - newSceneRect.bottom());

            return newPos;
        }
        return QGraphicsItem::itemChange(change, value);
    }

signals:
    void overlayMoved();

private:
    QGraphicsView *m_view{nullptr};
    QGraphicsItem *m_imageItem{nullptr};
};
