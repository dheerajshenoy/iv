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

#ifdef HAS_LIBAVIF
#include <avif/avif.h>
#endif

#include <fstream>

ImageView::ImageView(const Config &config, QWidget *parent) : QWidget(parent), m_config(config)
{
    QVBoxLayout *layout = new QVBoxLayout();
    m_gview             = new GraphicsView();
    m_gscene            = new QGraphicsScene();
    layout->addWidget(m_gview);
    m_gview->setScene(m_gscene);
    m_gscene->addItem(m_pix_item);

    m_minimap = new Minimap(m_gview);
    m_minimap->setOverlayRectColor(QColor::fromString(m_config.minimap_overlay_color));
    m_minimap->setOverlayRectBorderWidth(m_config.minimap_overlay_border_width);
    m_minimap->setOverlayRectBorderColor(QColor::fromString(m_config.minimap_overlay_border_color));
    m_minimap->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    m_minimap->raise();
    m_minimap->setForceHidden(!m_config.minimap_shown);

    setLayout(layout);
    m_hscrollbar = m_gview->horizontalScrollBar();
    m_vscrollbar = m_gview->verticalScrollBar();

    if (!m_config.vscrollbar_auto_hide)
        m_gview->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (!m_config.vscrollbar_shown)
        m_gview->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (!m_config.hscrollbar_auto_hide)
        m_gview->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    if (!m_config.hscrollbar_shown)
        m_gview->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    if (m_config.minimap_shown)
    {
        connect(m_hscrollbar, &QScrollBar::valueChanged, this, [&](int /*value */) { updateMinimapRegion(); });
        connect(m_vscrollbar, &QScrollBar::valueChanged, this, [&](int /*value */) { updateMinimapRegion(); });
        connect(m_gview, &GraphicsView::openFilesRequested, this, &ImageView::openFilesRequested);
    }
}

bool
ImageView::openFile(const QString &filepath) noexcept
{
    m_filepath           = filepath;
    QFuture<void> future = QtConcurrent::run([&]
    {
        auto bytes  = QFileInfo(m_filepath).size();
        QString str = humanReadableSize(bytes);
        m_filesize  = str;
    });

    m_isGif = false;
    stopGifAnimation();

    m_mimeType = getMimeType(filepath);

    m_success = false;

    if (m_mimeType == "image/avif")
#ifdef HAS_LIBAVIF
        success = renderAvif();
#else
    {
        QMessageBox::warning(this, "Open File",
                             "You have tried to open an AVIF file. IV currently does not open AVIF. Please install "
                             "`libavif` library and then build IV again.");
        m_success = false;
        return m_success;
    }
#endif
    else
        m_success = render();

    if (!m_success)
    {
        QMessageBox::critical(this, "Error opening image", "Failed to open image: " + filepath);
        return m_success;
    }

    m_gview->fitInView(m_pix_item, Qt::KeepAspectRatio);
    m_gview->centerOn(m_pix_item);

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

    Magick::Blob blob;
    try
    {
        image.write(&blob, format);
    }
    catch (...)
    {
        return QImage();
    }

    const uchar *data = static_cast<const uchar *>(blob.data());

    QImage img(data, width, height, bytesPerLine, imgFormat);
    // image.write(0, 0, width, height, format, Magick::CharPixel, img.bits());

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

    img = QImage(pixels.data(), width, height, QImage::Format_RGBA8888).copy();
    return img;
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
    Magick::Image image;
    try
    {
        image.read(m_filepath.toStdString());
    }
    catch (const Magick::ErrorFileOpen &e)
    {
        qDebug() << "Error opening image: " << e.what();
        QMessageBox::critical(this, "Error opening image: ", e.what());
        return false;
    }
    catch (const Magick::ErrorMissingDelegate &e)
    {
        qDebug() << "Error missing delegate: " << e.what();
        QMessageBox::critical(this, "Error missing delegate: ", e.what());
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

    if (m_isMinimapMode)
        updateMinimapRegion();
}

void
ImageView::zoomOut() noexcept
{
    m_gview->zoomOut();
    if (m_isMinimapMode)
        updateMinimapRegion();
}

void
ImageView::rotateClock() noexcept
{
    m_rotation     = (m_rotation + 90) % 360;
    QTransform mat = m_gview->transform();
    mat.rotate(90);
    m_gview->setTransform(mat);
    m_minimap->setTransform(mat);
    m_gview->centerOn(m_pix_item);
}

void
ImageView::rotateAnticlock() noexcept
{
    m_rotation     = (m_rotation - 90) % 360;
    QTransform mat = m_gview->transform();
    mat.rotate(-90);
    m_gview->setTransform(mat);
    m_minimap->setTransform(mat);
    m_gview->centerOn(m_pix_item);
}

void
ImageView::fitHeight() noexcept
{
    QTransform t;
    t.rotate(m_rotation);
    auto pixHeight  = t.mapRect(m_pix_item->boundingRect()).height();
    auto viewHeight = m_gview->viewport()->rect().height();

    float scaleFactor = viewHeight / pixHeight;

    t.scale(scaleFactor, scaleFactor);
    m_gview->setTransform(t);
    m_gview->centerOn(m_pix_item);
}

void
ImageView::fitWidth() noexcept
{
    QTransform t;
    t.rotate(m_rotation);

    auto pixwidth  = t.mapRect(m_pix_item->boundingRect()).width();
    auto viewwidth = m_gview->viewport()->rect().width();

    float scaleFactor = viewwidth / pixwidth;

    t.scale(scaleFactor, scaleFactor);
    m_gview->setTransform(t);
    m_gview->centerOn(m_pix_item);
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
    m_vscrollbar->setValue(m_vscrollbar->value() + 30);
}

void
ImageView::scrollDown() noexcept
{
    m_vscrollbar->setValue(m_vscrollbar->value() - 30);
}

void
ImageView::flipLeftRight() noexcept
{
    QPixmap pix = m_pix_item->pixmap();
    pix         = pix.transformed(QTransform().scale(-1, 1));
    m_pix_item->setPixmap(pix);
    m_minimap->setPixmap(pix);
}

void
ImageView::flipUpDown() noexcept
{
    QPixmap pix = m_pix_item->pixmap();
    pix         = pix.transformed(QTransform().scale(1, -1));
    m_pix_item->setPixmap(pix);
    m_minimap->setPixmap(pix);
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
        delete m_movie;
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
    int margin        = 100;
    QRectF paddedRect = m_pix_item->boundingRect().adjusted(-margin, -margin, margin, margin);
    m_gscene->setSceneRect(paddedRect);
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

    QRectF visible = m_gview->mapToScene(m_gview->viewport()->rect()).boundingRect();
    QRectF image   = m_pix_item->sceneBoundingRect(); // Full image, in scene coords

    bool fullyVisible = visible.contains(image);
    m_minimap->setVisible(!fullyVisible);

    if (!fullyVisible)
        m_minimap->setOverlayRect(visible); // Pass in scene-space rect
}

void
ImageView::updateMinimapPosition() noexcept
{
    m_minimap->move(m_gview->viewport()->width() - m_minimap->width() - 30,
                    m_gview->viewport()->height() - m_minimap->height() - 30);
}

void
ImageView::resizeEvent(QResizeEvent *e)
{
    updateMinimapPosition();
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

    int margin    = 50;
    QRectF padded = m_pix_item->boundingRect().adjusted(-margin, -margin, margin, margin);
    m_gview->setSceneRect(padded);
}

void
ImageView::setDPR(float dpr) noexcept
{
    m_dpr = dpr;

    if (m_pix_item->pixmap().isNull())
        return;

#ifdef HAS_LIBAVIF
    if (m_mimeType == "image/avif")
        renderAvif();
    else
        render();
#else
    render();
#endif
}
