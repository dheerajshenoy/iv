#include "ImageView.hpp"

#include "GraphicsView.hpp"
#include "Magick++/Exception.h"

#include <QEvent>
#include <QFileInfo>
#include <QGraphicsProxyWidget>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QScrollBar>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QtConcurrent/qtconcurrentreducekernel.h>
#include <qnamespace.h>

#ifdef HAS_LIBAVIF
#include <avif/avif.h>
#endif

#include <fstream>

// #ifdef HAS_LIBEXIV2
#include <exiv2/exiv2.hpp>
// #endif

ImageView::ImageView(const Config &config, QWidget *parent) : QWidget(parent), m_config(config)
{
    QVBoxLayout *layout = new QVBoxLayout();
    m_gview             = new GraphicsView();
    m_gscene            = new QGraphicsScene();
    layout->addWidget(m_gview);
    m_gview->setScene(m_gscene);
    m_gscene->addItem(m_pix_item);

    layout->setContentsMargins(0, 0, 0, 0);
    m_gview->setContentsMargins(0, 0, 0, 0);

    m_minimap = new Minimap(m_gview);
    m_minimap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_minimap->raise();

    // Minimap click to move viewport
    connect(m_minimap, &Minimap::minimapClicked, this, [this](const QPointF &pos)
    {
        m_gview->centerOn(pos);
        updateMinimapRegion();
    });

    // Overlay rectangle
    m_overlay_rect = new OverlayRect();
    m_overlay_rect->setImageItem(m_pix_item);
    m_overlay_rect->setMainView(m_minimap);
    m_minimap->scene()->addItem(m_overlay_rect);

    // Overlay moved handler
    connect(m_overlay_rect, &OverlayRect::overlayMoved, this, [this]()
    {
        const QRectF overlayRect    = m_overlay_rect->boundingRect(); // overlay in scene coords
        const QPointF overlayCenter = m_overlay_rect->mapToScene(overlayRect.center());
        m_gview->centerOn(overlayCenter);
        updateMinimapRegion();
    });

    setLayout(layout);
    m_hscrollbar = m_gview->horizontalScrollBar();
    m_vscrollbar = m_gview->verticalScrollBar();

    UpdateFromConfig();
}

bool
ImageView::openFile(const QString &filepath) noexcept
{
    if (!QFile::exists(filepath))
        return false;

    m_filepath       = filepath;
    const auto bytes = QFileInfo(m_filepath).size();
    m_filesize       = humanReadableSize(bytes);

    m_isGif = false;
    stopGifAnimation();

    m_mimeType = getMimeType(filepath);

    m_success = false;

    QtConcurrent::run([this, filepath]() { m_success = render(); }).waitForFinished();

    if (m_success)
    {
        m_gview->fitInView(m_pix_item, Qt::KeepAspectRatio);
    }

    return m_success;
}

QImage
ImageView::magickImageToQImage(Magick::Image &image) noexcept
{
    const int width  = image.columns();
    const int height = image.rows();

    const bool hasAlpha            = image.alpha();
    const std::string format       = hasAlpha ? "RGBA" : "RGB";
    const QImage::Format imgFormat = hasAlpha ? QImage::Format_RGBA8888 : QImage::Format_RGB888;

    const int bytesPerPixel = hasAlpha ? 4 : 3;
    const int bytesPerLine  = width * bytesPerPixel; // tightly packed

    std::vector<unsigned char> buffer(width * height * bytesPerPixel);

    try
    {
        image.write(0, 0, width, height, format, Magick::CharPixel, buffer.data());
    }
    catch (...)
    {
        return QImage();
    }

    QImage img(buffer.data(), width, height, bytesPerLine, imgFormat);

    // Must copy the image data, as the buffer will go out of scope
    return img.copy();
}

#ifdef HAS_LIBAVIF
QImage
ImageView::avifToQImage() noexcept
{
    QImage img;
    int width, height;
    std::vector<uint8_t> pixels;
    std::string filename = m_filepath.toStdString();
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file)
    {
        QMessageBox::critical(this, "Open File", "Failed to open file");
        qCritical() << "Failed to open file";
        return img;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        QMessageBox::critical(this, "Open File", "Failed to read file");
        qCritical() << "Failed to read file\n";
        return img;
    }

    // Set up decoder
    avifDecoder *decoder = avifDecoderCreate();
    avifResult result    = avifDecoderSetIOMemory(decoder, buffer.data(), buffer.size());
    if (result != AVIF_RESULT_OK)
    {
        const char *err = avifResultToString(result);
        qCritical() << "Failed to set AVIF IO: " << err << "\n";
        QMessageBox::critical(this, "Open File", err);
        avifDecoderDestroy(decoder);
        return img;
    }

    result = avifDecoderParse(decoder);
    if (result != AVIF_RESULT_OK)
    {
        const char *err = avifResultToString(result);
        qCritical() << "Failed to parse AVIF: " << err << "\n";
        QMessageBox::critical(this, "Open File", err);
        avifDecoderDestroy(decoder);
        return img;
    }

    result = avifDecoderNextImage(decoder);
    if (result != AVIF_RESULT_OK)
    {
        const char *err = avifResultToString(result);
        qCritical() << "Failed to decode AVIF image: " << err << "\n";
        avifDecoderDestroy(decoder);
        return img;
    }

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, decoder->image);
    rgb.format = AVIF_RGB_FORMAT_RGBA;

    avifResult resalloc = avifRGBImageAllocatePixels(&rgb);
    avifResult resconv  = avifImageYUVToRGB(decoder->image, &rgb);

    width  = rgb.width;
    height = rgb.height;
    pixels.assign(rgb.pixels, rgb.pixels + rgb.rowBytes * height);

    avifRGBImageFreePixels(&rgb);
    avifDecoderDestroy(decoder);

    return QImage(pixels.data(), width, height, QImage::Format_RGBA8888).copy();
}

bool
ImageView::renderAvif() noexcept
{
    QImage img = avifToQImage();
    if (img.isNull())
        return false;
    loadImage(img);
    return true;
}
#endif

bool
ImageView::render() noexcept
{
    if (m_mimeType == "image/avif")
    {
#ifdef HAS_LIBAVIF
        return renderAvif();
#else
        QMessageBox::warning(this, "Open File",
                             "You have tried to open an AVIF file. IV currently does not open AVIF. Please install "
                             "`libavif` library and then build IV again.");
        return false;
#endif
    }

    Magick::Image image;
    try
    {
        image.read(m_filepath.toStdString());
    }
    catch (const Magick::ErrorFileOpen &e)
    {
        qDebug() << "Error opening image: " << e.what();
        // QMessageBox::critical(this, "Error opening image: ", e.what());
        return false;
    }
    catch (const Magick::ErrorCorruptImage &e)
    {
        qDebug() << "Error corrupt image: " << e.what();
        // QMessageBox::critical(this, "Error corrupt image: ", e.what());
        return false;
    }
    catch (const Magick::ErrorMissingDelegate &e)
    {
        qDebug() << "Error missing delegate: " << e.what();
        QMessageBox::critical(this, "Error missing delegate: ", e.what());
        return false;
    }
    catch (const Magick::Exception &e)
    {
        qDebug() << "Magick++ exception: " << e.what();
        QMessageBox::critical(this, "Magick++ exception: ", e.what());
        return false;
    }
    catch (const std::exception &e)
    {
        qDebug() << "Standard exception: " << e.what();
        QMessageBox::critical(this, "Standard exception: ", e.what());
        return false;
    }
    catch (...)
    {
        qDebug() << "Unknown error occurred while opening image.";
        QMessageBox::critical(this, "Unknown error", "An unknown error occurred while opening the image.");
        return false;
    }

    if (hasMoreThanOneFrame())
        renderAnimatedImage();
    else
    {
        QImage img = magickImageToQImage(image);
        loadImage(img);
    }

    return true;
}

void
ImageView::zoomIn() noexcept
{
    m_gview->zoomIn();
    updateMinimapRegion();
}

void
ImageView::zoomOut() noexcept
{
    m_gview->zoomOut();
    updateMinimapRegion();
}

void
ImageView::rotateClock() noexcept
{
    // Save current viewport center in scene coordinates
    QPointF viewCenter = m_gview->mapToScene(m_gview->viewport()->rect().center());

    // Update rotation state
    m_rotation = (m_rotation + 90) % 360;

    // Rotate the pixmap item around its center
    m_pix_item->setTransformOriginPoint(m_pix_item->sceneBoundingRect().center());
    m_pix_item->setRotation(m_rotation);
    m_gview->setSceneRect(m_pix_item->sceneBoundingRect());

    m_gview->centerOn(viewCenter);
    m_minimap->setRotation(m_rotation);
    updateMinimapRegion();
}

void
ImageView::rotateAnticlock() noexcept
{
    // Save current viewport center in scene coordinates
    QPointF viewCenter = m_gview->mapToScene(m_gview->viewport()->rect().center());

    // Update rotation state
    m_rotation = (m_rotation + 270) % 360;

    // Rotate the pixmap item around its center
    m_pix_item->setTransformOriginPoint(m_pix_item->sceneBoundingRect().center());
    m_pix_item->setRotation(m_rotation);
    m_gview->setSceneRect(m_pix_item->sceneBoundingRect());

    m_gview->centerOn(viewCenter);
    m_minimap->setRotation(m_rotation);
    updateMinimapRegion();
}

void
ImageView::fitHeight() noexcept
{
    m_fit_mode = FitMode::HEIGHT;
    QTransform t;
    t.rotate(m_rotation);
    auto pixHeight  = t.mapRect(m_pix_item->boundingRect()).height();
    auto viewHeight = m_gview->viewport()->rect().height();

    float scaleFactor = viewHeight / pixHeight;

    t.scale(scaleFactor, scaleFactor);
    m_gview->setTransform(t);
    // m_gview->centerOn(m_pix_item);
}

void
ImageView::fitWidth() noexcept
{
    m_fit_mode = FitMode::WIDTH;
    QTransform t;
    t.rotate(m_rotation);

    auto pixwidth  = t.mapRect(m_pix_item->boundingRect()).width();
    auto viewwidth = m_gview->viewport()->rect().width();

    float scaleFactor = viewwidth / pixwidth;

    t.scale(scaleFactor, scaleFactor);
    m_gview->setTransform(t);
    // m_gview->centerOn(m_pix_item);
}

void
ImageView::fitWindow() noexcept
{
    m_fit_mode = FitMode::WINDOW;
    m_gview->fitInView(m_pix_item, Qt::KeepAspectRatio);
    // m_gview->centerOn(m_pix_item);
}

void
ImageView::scrollLeft() noexcept
{
    m_hscrollbar->setValue(m_hscrollbar->value() - 30);
}

void
ImageView::scrollRight() noexcept
{
    m_hscrollbar->setValue(m_hscrollbar->value() + 30);
}

void
ImageView::scrollUp() noexcept
{
    m_vscrollbar->setValue(m_vscrollbar->value() - 30);
}

void
ImageView::scrollDown() noexcept
{
    m_vscrollbar->setValue(m_vscrollbar->value() + 30);
}

void
ImageView::flipLeftRight() noexcept
{
    QPixmap pix  = m_pix_item->pixmap();
    QTransform t = m_gview->transform();
    t.scale(-1, 1);
    m_gview->setTransform(t);
    m_minimap->setTransform(t);
}

void
ImageView::flipUpDown() noexcept
{
    QPixmap pix  = m_pix_item->pixmap();
    QTransform t = m_gview->transform();
    t.scale(1, -1);
    m_gview->setTransform(t);
    m_minimap->setTransform(t);
}

QString
ImageView::fileName() noexcept
{
    return QFileInfo(m_filepath).fileName();
}

QString
ImageView::baseName() noexcept
{
    return QFileInfo(m_filepath).baseName();
}

QString
ImageView::humanReadableSize(qint64 bytes) noexcept
{
    static const char *sizes[] = {"B", "KB", "MB", "GB", "TB"};
    double len                 = bytes;
    int order                  = 0;
    while (len >= 1024 && order < 4)
    {
        order++;
        len /= 1024.0;
    }
    return QString("%1 %2").arg(len, 0, 'f', 2).arg(sizes[order]);
}

void
ImageView::renderAnimatedImage() noexcept
{
    if (m_movie)
    {
        m_movie->stop();
        disconnect(m_movie, &QMovie::frameChanged, this, &ImageView::updateGifFrame);
        m_movie->deleteLater();
        m_movie = nullptr;
    }
    m_isGif = true;
    m_movie = new QMovie(m_filepath, QByteArray(), this);
    connect(m_movie, &QMovie::frameChanged, this, &ImageView::updateGifFrame);
    m_movie->start();
}

void
ImageView::updateGifFrame(int /*frameNumber*/) noexcept
{
    if (!m_movie)
        return;

    QPixmap frame = m_movie->currentPixmap();

    m_pix_item->setPixmap(frame);

    m_minimap->setPixmap(frame);
    if (!m_config.ui.minimap_image)
        m_minimap->showOverlayOnly(true);
}

void
ImageView::stopGifAnimation() noexcept
{
    if (m_movie)
        m_movie->stop();
}

void
ImageView::startGifAnimation() noexcept
{
    if (m_movie)
        m_movie->start();
}

void
ImageView::showEvent(QShowEvent *e)
{
    if (m_isGif)
    {
        startGifAnimation();
    }

    QWidget::showEvent(e);
}

void
ImageView::hideEvent(QHideEvent *e)
{
    if (m_isGif)
    {
        stopGifAnimation();
    }
    QWidget::hideEvent(e);
}

bool
ImageView::hasMoreThanOneFrame() noexcept
{
    std::list<Magick::Image> frames;

    try
    {
        Magick::pingImages(&frames, m_filepath.toStdString());
        return frames.size() > 1;
    }
    catch (const Magick::Exception &error)
    {
        return false;
    }
}

void
ImageView::updateMinimapRegion() noexcept
{
    if (m_minimap->forceHidden())
        return;

    // Full image rect in scene coordinates
    QRectF imageRect = m_pix_item->sceneBoundingRect();

    // Current visible viewport in scene coordinates
    QRectF viewRect = m_gview->mapToScene(m_gview->viewport()->rect()).boundingRect();

    // Clamp viewRect inside imageRect
    QRectF clampedRect = viewRect.intersected(imageRect);

    // Update overlay rectangle in minimap coordinates
    if (!clampedRect.isNull())
    {
        // Block signals to prevent triggering overlayMoved
        m_overlay_rect->blockSignals(true);

        // Set the position to the top-left of the clamped rect
        m_overlay_rect->setPos(clampedRect.topLeft());

        // Set the rectangle size (in local coordinates, starting at 0,0)
        m_overlay_rect->setRect(0, 0, clampedRect.width(), clampedRect.height());

        // Unblock signals
        m_overlay_rect->blockSignals(false);
    }

    // Hide minimap if the entire image fits in the viewport
    constexpr qreal EPS = 1.0;
    bool fullyVisible   = (viewRect.left() <= imageRect.left() + EPS && viewRect.top() <= imageRect.top() + EPS &&
                         viewRect.right() >= imageRect.right() - EPS && viewRect.bottom() >= imageRect.bottom() - EPS);
    m_minimap->setVisible(!fullyVisible);
}

void
ImageView::updateMinimapPosition() noexcept
{
    if (!m_minimap || !m_gview)
        return;

    const int padding = m_minimap->padding();

    const auto viewport = m_gview->viewport()->rect();
    const int gw        = viewport.width();
    const int gh        = viewport.height();

    int x = 0;
    int y = 0;
    QFlags<Qt::AlignmentFlag> alignment;

    switch (m_minimap->location())
    {
        case Minimap::Location::TOP_LEFT:
            x         = padding;
            y         = padding;
            alignment = Qt::AlignTop | Qt::AlignLeft;
            break;

        case Minimap::Location::TOP_RIGHT:
            x         = gw - m_minimap->width() - padding;
            y         = padding;
            alignment = Qt::AlignTop | Qt::AlignRight;
            break;

        case Minimap::Location::BOTTOM_LEFT:
            x         = padding;
            y         = gh - m_minimap->height() - padding;
            alignment = Qt::AlignBottom | Qt::AlignLeft;
            break;

        case Minimap::Location::BOTTOM_RIGHT:
            x         = gw - m_minimap->width() - padding;
            y         = gh - m_minimap->height() - padding;
            alignment = Qt::AlignBottom | Qt::AlignRight;
            break;

        case Minimap::Location::TOP_CENTER:
            x         = (gw - m_minimap->width()) / 2;
            y         = padding;
            alignment = Qt::AlignTop | Qt::AlignHCenter;
            break;

        case Minimap::Location::BOTTOM_CENTER:
            x         = (gw - m_minimap->width()) / 2;
            y         = gh - m_minimap->height() - padding;
            alignment = Qt::AlignBottom | Qt::AlignHCenter;
            break;

        case Minimap::Location::CENTER_LEFT:
            x         = padding;
            y         = (gh - m_minimap->height()) / 2;
            alignment = Qt::AlignVCenter | Qt::AlignLeft;
            break;

        case Minimap::Location::CENTER_RIGHT:
            x         = gw - m_minimap->width() - padding;
            y         = (gh - m_minimap->height()) / 2;
            alignment = Qt::AlignVCenter | Qt::AlignRight;
            break;

        case Minimap::Location::CENTER:
            x         = (gw - m_minimap->width()) / 2;
            y         = (gh - m_minimap->height()) / 2;
            alignment = Qt::AlignCenter;
            break;
    }

    // Move the minimap relative to the QGraphicsView widget
    m_minimap->move(x, y);
    m_minimap->setAlignment(alignment);
}

void
ImageView::resizeEvent(QResizeEvent *e)
{
    updateMinimapPosition();
    updateMinimapRegion();
    QWidget::resizeEvent(e);
}

void
ImageView::dropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        QList<QString> files;
        for (const auto &url : e->mimeData()->urls())
        {
            auto file = url.toLocalFile();
            files.append(file);
        }
        emit openFilesRequested(files);
    }
}

void
ImageView::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls() || e->mimeData()->hasText())
        e->acceptProposedAction();
    else
        e->ignore();
}

QSize
ImageView::size() noexcept
{
    return m_pix_item->pixmap().size();
}

QString
ImageView::getMimeType(const QString &filePath) noexcept
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(filePath);
    return type.name();
}

void
ImageView::loadImage(const QImage &img) noexcept
{
    m_pix = QPixmap::fromImage(img);

    m_pix.setDevicePixelRatio(m_dpr);
    m_pix_item->setPixmap(m_pix);

    m_minimap->setPixmap(m_pix);
    if (!m_config.ui.minimap_image)
        m_minimap->showOverlayOnly(true);

    m_gview->setSceneRect(m_pix_item->boundingRect());
}

void
ImageView::setDPR(float dpr) noexcept
{
    m_dpr = dpr;

    if (m_pix_item->pixmap().isNull())
        return;

    render();
}

bool
ImageView::reloadFile() noexcept
{
    if (m_filepath.isEmpty())
        return false;

    const auto bytes = QFileInfo(m_filepath).size();
    m_filesize       = humanReadableSize(bytes);

    m_isGif = false;
    stopGifAnimation();
    m_mimeType = getMimeType(m_filepath);
    m_success  = false;

    m_success = render();

    if (!m_success)
    {
        if (!m_auto_reload)
            QMessageBox::critical(this, "Error opening image", "Failed to open image: " + m_filepath);
        return m_success;
    }

    return m_success;
}

// Watch for file changes and auto-reload
void
ImageView::toggleAutoReload() noexcept
{
    setAutoReload(!m_auto_reload);
}

void
ImageView::toggleHScrollbar() noexcept
{
    m_hscrollbar->setVisible(!m_hscrollbar->isVisible());
}

void
ImageView::toggleVScrollbar() noexcept
{
    m_vscrollbar->setVisible(!m_vscrollbar->isVisible());
}

void
ImageView::setAutoReload(bool enabled) noexcept
{
    m_auto_reload = enabled;
    if (m_auto_reload)
    {
        if (!m_file_watcher)
            m_file_watcher = new QFileSystemWatcher(this);

        if (!m_file_watcher->files().contains(m_filepath))
            m_file_watcher->addPath(m_filepath);

        connect(m_file_watcher, &QFileSystemWatcher::fileChanged, this, &ImageView::onFileReloadRequested,
                Qt::UniqueConnection);
    }
    else
    {
        if (m_file_watcher)
        {
            m_file_watcher->removePath(m_filepath);
            m_file_watcher->deleteLater();
            m_file_watcher = nullptr;
        }
    }
}

bool
ImageView::waitUntilReadableAsync() noexcept
{
    QFile f(m_filepath);
    return f.open(QIODevice::ReadOnly);
}

void
ImageView::onFileReloadRequested(const QString &path) noexcept
{
    if (path != m_filepath)
        return;

    tryReloadLater(0);
}

void
ImageView::tryReloadLater(int attempt) noexcept
{
    if (attempt > 15) // ~15 * 100ms = 1.5s
        return;       // give up

    if (waitUntilReadableAsync())
    {
        reloadFile();

        // IMPORTANT: file may have been removed and replaced → watcher loses it
        if (m_file_watcher && !m_file_watcher->files().contains(m_filepath))
            m_file_watcher->addPath(m_filepath);

        return;
    }

    QTimer::singleShot(100, this, [this, attempt]() { tryReloadLater(attempt + 1); });
}

#ifdef HAS_LIBEXIV2
QMap<QString, QString>
ImageView::getEXIF() noexcept
{
    QMap<QString, QString> exifData;

    try
    {
        Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(m_filepath.toStdString());
        image->readMetadata();
        Exiv2::ExifData &exif = image->exifData();

        if (exif.empty())
        {
            qInfo() << "No EXIF data found in the image.";
            return exifData;
        }

        for (const auto &datum : exif)
        {
            QString key   = QString::fromStdString(datum.key());
            QString value = QString::fromStdString(datum.toString());
            exifData.insert(key, value);
        }
    }
    catch (const Exiv2::Error &e)
    {
        qDebug() << "Error reading EXIF data: " << e.what();
    }

    return exifData;
}
#endif

void
ImageView::UpdateFromConfig() noexcept
{
    m_minimap->setForceHidden(!m_config.ui.minimap_shown);
    m_minimap->setPixmapOpacity(m_config.ui.minimap_image_opacity);
    m_minimap->setLocation(m_config.ui.minimap_location);
    m_minimap->setMinimapSize(m_config.ui.minimap_size);
    m_minimap->setMinimapPadding(m_config.ui.minimap_padding);
    updateMinimapPosition();

    m_minimap->setClickable(m_config.ui.minimap_clickable);
    m_overlay_rect->setClickable(m_config.ui.minimap_overlay_movable);
    setOverlayRectColor(QColor::fromString(m_config.ui.minimap_overlay_color));
    setOverlayRectBorderWidth(m_config.ui.minimap_overlay_border_width);
    setOverlayRectBorderColor(QColor::fromString(m_config.ui.minimap_overlay_border_color));

    m_minimap->setPixmap(m_pix);
    if (!m_config.ui.minimap_image)
        m_minimap->showOverlayOnly(true);

    // Disconnect old connections to avoid duplicates
    disconnect(m_hscrollbar, &QScrollBar::valueChanged, this, nullptr);
    disconnect(m_vscrollbar, &QScrollBar::valueChanged, this, nullptr);

    if (!m_config.ui.vscrollbar_auto_hide)
        m_gview->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (!m_config.ui.vscrollbar_shown)
        m_gview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (!m_config.ui.hscrollbar_auto_hide)
        m_gview->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (!m_config.ui.hscrollbar_shown)
        m_gview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (m_config.ui.minimap_shown)
    {
        connect(m_hscrollbar, &QScrollBar::valueChanged, this, [&](int /*value */) { updateMinimapRegion(); });
        connect(m_vscrollbar, &QScrollBar::valueChanged, this, [&](int /*value */) { updateMinimapRegion(); });
        connect(m_gview, &GraphicsView::openFilesRequested, this, &ImageView::openFilesRequested);
    }
}

void
ImageView::scrollToLeftEdge() noexcept
{
    m_hscrollbar->setValue(m_hscrollbar->minimum());
}

void
ImageView::scrollToRightEdge() noexcept
{
    m_hscrollbar->setValue(m_hscrollbar->maximum());
}

void
ImageView::scrollToTopEdge() noexcept
{
    m_vscrollbar->setValue(m_vscrollbar->minimum());
}

void
ImageView::scrollToBottomEdge() noexcept
{
    m_vscrollbar->setValue(m_vscrollbar->maximum());
}

void
ImageView::showFilePropertiesDialog() noexcept
{
    if (!m_prop_widget)
    {
        m_prop_widget = new PropertiesWidget(this);
    }

    QFileInfo fileInfo(m_filepath);
    // Get properties in QMap<QString, QString> format
    QMap<QString, QString> properties;
    properties.insert("File Name", fileInfo.fileName());
    properties.insert("File Path", fileInfo.absoluteFilePath());
    properties.insert("File Size", m_filesize);
    properties.insert("File Type", m_mimeType);
    properties.insert("Image Dimensions", QString("%1 x %2").arg(width()).arg(height()));
    properties.insert("Last Modified", fileInfo.lastModified().toString());
    m_prop_widget->setProperties(properties);

#ifdef HAS_LIBEXIV2
    QMap<QString, QString> exifData = getEXIF();
    if (!exifData.isEmpty())
    {
        for (auto it = exifData.constBegin(); it != exifData.constEnd(); ++it)
        {
            properties.insert(it.key(), it.value());
        }
        m_prop_widget->setEXIF(exifData);
    }
#endif
    m_prop_widget->show();
}
