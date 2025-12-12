#pragma once

#include "Config.hpp"
#include "GraphicsView.hpp"
#include "Minimap.hpp"

#include <ImageMagick-7/Magick++.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMimeData>
#include <QMovie>
#include <QObject>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>

class ImageView : public QWidget
{
    Q_OBJECT
public:
    ImageView(const Config &config, QWidget *parent = nullptr);

    bool openFile(const QString &path) noexcept;
    bool reloadFile() noexcept;
    void setDPR(float dpr) noexcept;

    enum class FitMode
    {
        WINDOW = 0,
        WIDTH,
        HEIGHT
    };

    QSize size() noexcept;
    void scrollToLeftEdge() noexcept;
    void scrollToRightEdge() noexcept;
    void scrollToTopEdge() noexcept;
    void scrollToBottomEdge() noexcept;
    void zoomIn() noexcept;
    void zoomOut() noexcept;
    void rotateClock() noexcept;
    void rotateAnticlock() noexcept;
    void fitWindow() noexcept;
    void fitWidth() noexcept;
    void fitHeight() noexcept;
    void scrollLeft() noexcept;
    void scrollRight() noexcept;
    void scrollUp() noexcept;
    void scrollDown() noexcept;
    void flipLeftRight() noexcept;
    void flipUpDown() noexcept;
    void toggleAutoReload() noexcept;
    void toggleHScrollbar() noexcept;
    void toggleVScrollbar() noexcept;
    void setAutoReload(bool enabled) noexcept;

    static inline QPixmap rotatePixmap90(const QPixmap &src)
    {
        static const QTransform rot90 = QTransform().rotate(90);
        return src.transformed(rot90, Qt::FastTransformation);
    }

    inline void setAutoFit(bool enabled) noexcept
    {
        m_auto_fit = enabled;
    }

    inline bool autoFit() noexcept
    {
        return m_auto_fit;
    }

    inline void setConfig(const Config &config) noexcept
    {
        m_config = config;
    }

    void UpdateFromConfig() noexcept;

    QString fileName() noexcept;
    QString baseName() noexcept;
    inline QString filePath() noexcept
    {
        return m_filepath;
    }
    inline QString fileSize() noexcept
    {
        return m_filesize;
    }
    inline QString fileDir() noexcept
    {
        return QFileInfo(m_filepath).absolutePath();
    }

    inline void toggleMinimap() noexcept
    {
        m_minimap->setForceHidden(!m_minimap->forceHidden());
        updateMinimapRegion();
    }

    inline void setOverlayRectColor(const QColor &color) noexcept
    {
        if (color.isValid())
            m_overlay_rect->setBrush(color);
    }

    inline void setOverlayRectBorderWidth(int width) noexcept
    {
        if (width < 0)
            return;
        m_overlay_rect->setPen(QPen(m_overlay_rect->pen().color(), width));
    }

    inline void setOverlayRectBorderColor(const QColor &color) noexcept
    {
        if (color.isValid())
            m_overlay_rect->setPen(QPen(color, m_overlay_rect->pen().width()));
    }

    inline bool success() noexcept
    {
        return m_success;
    }

    inline GraphicsView *gview() noexcept
    {
        return m_gview;
    }

    inline FitMode fitMode() noexcept
    {
        return m_fit_mode;
    }

#ifdef HAS_LIBEXIV2
    QMap<QString, QString> getEXIF() noexcept;
#endif

    void updateMinimapPosition() noexcept;
signals:
    void openFilesRequested(const QList<QString> &files);

protected:
    void showEvent(QShowEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    void loadImage(const QImage &img) noexcept;
    bool render() noexcept;
#ifdef HAS_LIBAVIF
    bool renderAvif() noexcept;
    QImage avifToQImage() noexcept;
#endif
    void renderAnimatedImage() noexcept;
    QString humanReadableSize(qint64 bytes) noexcept;
    void updateGifFrame(int frameNumber) noexcept;
    void stopGifAnimation() noexcept;
    void startGifAnimation() noexcept;
    bool hasMoreThanOneFrame() noexcept;
    void updateMinimapRegion() noexcept;
    QString getMimeType(const QString &filepath) noexcept;

    void onFileReloadRequested(const QString &path) noexcept;
    bool waitUntilReadableAsync() noexcept;
    void tryReloadLater(int attempt) noexcept;

    bool m_isGif{false}, m_success{false}, m_auto_reload{false}, m_auto_fit{false};

    float m_dpr{1.0f};
    QImage magickImageToQImage(Magick::Image &image) noexcept;
    GraphicsView *m_gview;
    QGraphicsScene *m_gscene;
    QGraphicsPixmapItem *m_pix_item{new QGraphicsPixmapItem()};
    QString m_filepath, m_filesize;
    float m_zoomFactor{1.25};
    int m_rotation{0};
    QPixmap m_pix;
    QScrollBar *m_vscrollbar, *m_hscrollbar;
    QMovie *m_movie{nullptr};
    Minimap *m_minimap{nullptr};
    OverlayRect *m_overlay_rect{nullptr};
    Config m_config;
    QString m_mimeType;
    QFileSystemWatcher *m_file_watcher{nullptr};
    FitMode m_fit_mode;
};
