#pragma once

#include "Config.hpp"
#include "GraphicsView.hpp"
#include "Minimap.hpp"

#include <ImageMagick-7/Magick++.h>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QMimeData>
#include <QMovie>
#include <QObject>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileSystemWatcher>

class ImageView : public QWidget
{
    Q_OBJECT
public:
    ImageView(const Config &config, QWidget *parent = nullptr);

    bool openFile(const QString &path) noexcept;
    bool reloadFile() noexcept;
    void setDPR(float dpr) noexcept;

    QSize size() noexcept;
    void zoomIn() noexcept;
    void zoomOut() noexcept;
    void rotateClock() noexcept;
    void rotateAnticlock() noexcept;
    void fitWidth() noexcept;
    void fitHeight() noexcept;
    void scrollLeft() noexcept;
    void scrollRight() noexcept;
    void scrollUp() noexcept;
    void scrollDown() noexcept;
    void flipLeftRight() noexcept;
    void flipUpDown() noexcept;
    void toggleAutoReload() noexcept;
    void setAutoReload(bool enabled) noexcept;

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

    inline bool success() noexcept
    {
        return m_success;
    }

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

    bool m_isGif{false}, m_isMinimapMode{false}, m_success{false}, m_auto_reload{false};

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
    Config m_config;
    QString m_mimeType;
    QFileSystemWatcher *m_file_watcher{nullptr};
};
