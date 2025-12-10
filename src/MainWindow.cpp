#include "MainWindow.hpp"

#include "ImageView.hpp"
#include "toml.hpp"

#include <QDesktopServices>
#include <QFileDialog>
#include <QKeySequence>
#include <QMessageBox>
#include <QScreen>
#include <QShortcut>
#include <QTabBar>
#include <QWindow>
#include <qnamespace.h>
#include <qshortcut.h>

void
MainWindow::readArgs(argparse::ArgumentParser &parser) noexcept
{
    if (parser.is_used("version"))
    {
        qDebug() << "iv version " << __IV_VERSION;
        qApp->exit(0);
        return;
    }

    // Construct the main window
    this->construct();

    if (parser.is_used("files"))
    {
        auto files = parser.get<std::vector<std::string>>("files");
        OpenFiles(files);
    }
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setAttribute(Qt::WA_NativeWindow);
    Magick::InitializeMagick(nullptr);
    setMinimumSize(600, 400);
}

void
MainWindow::construct() noexcept
{
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_tab_widget);
    layout->addWidget(m_panel);
    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    setCentralWidget(widget);

    initCommandMap();
    initConfig();
    initConnections();
    show();

    this->setContentsMargins(0, 0, 0, 0);
    m_tab_widget->setContentsMargins(0, 0, 0, 0);
    layout->setContentsMargins(0, 0, 0, 0);
    widget->setContentsMargins(0, 0, 0, 0);
    m_panel->setContentsMargins(0, 0, 0, 0);
    m_panel->layout()->setContentsMargins(5, 0, 5, 0);

    // Remove frame and padding around tab widget to make it flush with window edges
    m_tab_widget->tabBar()->setStyleSheet("QTabBar { margin: 0; padding: 0; }");
    layout->setSpacing(0);
    m_tab_widget->setStyleSheet("border: 0;");
    m_tab_widget->setDocumentMode(true);
    m_tab_widget->tabBar()->setVisible(m_config.ui.tabs_shown);
    m_tab_widget->setTabBarAutoHide(m_config.ui.tabs_autohide);
}

void
MainWindow::initDefaultKeybinds() noexcept
{
    if (!m_default_keybindings)
        return;

    m_config.shortcutMap["m"]      = "toggle_minimap";
    m_config.shortcutMap["Ctrl+W"] = "close_file";
    m_config.shortcutMap["o"]      = "open_file";
    m_config.shortcutMap["q"]      = "open_file_location";
    m_config.shortcutMap["="]      = "zoom_in";
    m_config.shortcutMap["-"]      = "zoom_out";
    m_config.shortcutMap[">"]      = "rotate_clock";
    m_config.shortcutMap["<"]      = "rotate_anticlock";
    m_config.shortcutMap["1"]      = "fit_width";
    m_config.shortcutMap["2"]      = "fit_height";
    m_config.shortcutMap["h"]      = "scroll_left";
    m_config.shortcutMap["j"]      = "scroll_down";
    m_config.shortcutMap["k"]      = "scroll_up";
    m_config.shortcutMap["l"]      = "scroll_right";
    m_config.shortcutMap["t"]      = "toggle_tabs";
    m_config.shortcutMap["F11"]    = "toggle_fullscreen";

    for (auto iter = m_config.shortcutMap.begin(); iter != m_config.shortcutMap.end(); iter++)
    {
        QShortcut *shortcut = new QShortcut(QKeySequence(iter.key()), this);
        connect(shortcut, &QShortcut::activated, this, [this, iter]() { m_commandMap[iter.value()](); });
    }
}

void
MainWindow::initConnections() noexcept
{

    QList<QScreen *> outputs = QGuiApplication::screens();
    // connect(m_tab_widget, &QTabWidget::currentChanged, this,
    //         &MainWindow::handleCurrentTabChanged);

    QWindow *win = window()->windowHandle();

    m_dpr = m_screen_dpr_map.value(QGuiApplication::primaryScreen()->name(), 1.0f);

    connect(win, &QWindow::screenChanged, this, [&](QScreen *screen)
    {
        if (std::holds_alternative<QMap<QString, float>>(m_config.rendering.dpr))
        {
            m_dpr = m_screen_dpr_map.value(screen->name(), 1.0f);
            if (m_imgv)
                m_imgv->setDPR(m_dpr);
        }
        else if (std::holds_alternative<float>(m_config.rendering.dpr))
        {
            m_dpr = std::get<float>(m_config.rendering.dpr);
        }
    });

    connect(m_tab_widget, &TabWidget::currentChanged, [&](int index)
    {
        m_imgv = qobject_cast<ImageView *>(m_tab_widget->widget(index));
        updateFileinfoInPanel();
    });

    connect(m_tab_widget, &TabWidget::tabCloseRequested, this, &MainWindow::handleTabClose);
}

void
MainWindow::handleTabClose(int index) noexcept
{
    m_panel->clear();
    QWidget *widget = m_tab_widget->widget(index);
    if (!widget)
        return;
    widget->close();
    widget->deleteLater();
    m_tab_widget->removeTab(index);
}

void
MainWindow::OpenFiles(const QList<QString> &files) noexcept
{
    for (const QString &filepath : files)
        OpenFile(filepath);
}

void
MainWindow::OpenFiles(const std::vector<std::string> &files) noexcept
{
    for (const std::string &filepath : files)
        OpenFile(QString::fromStdString(filepath));
}

void
MainWindow::OpenFile(const QString &filepath) noexcept
{
    QString fp = filepath;

    if (fp.isEmpty())
    {

        QStringList filepaths = openFileDialog();

        if (filepaths.isEmpty())
            return;

        OpenFiles(filepaths);
        return;
    }

    if (fp.startsWith("~"))
        fp = fp.replace(0, 1, QString::fromLocal8Bit(getenv("HOME")));

    m_imgv       = new ImageView(m_config, m_tab_widget);
    bool success = m_imgv->openFile(filepath);
    if (!success)
        return;
    m_tab_widget->addTab(m_imgv, fp);
    m_tab_widget->setCurrentWidget(m_imgv); // Make it the active tab
    connect(m_imgv, &ImageView::openFilesRequested, this,
            [&](const QStringList &files) { OpenFiles(files); }); // drop event
}

void
MainWindow::CloseFile() noexcept
{
    if (m_tab_widget->currentIndex() >= 0)
        m_tab_widget->tabCloseRequested(m_tab_widget->currentIndex()); // Make it the active tab
}

void
MainWindow::ZoomIn() noexcept
{
    m_imgv->zoomIn();
}

void
MainWindow::ZoomOut() noexcept
{
    if (m_imgv)
        m_imgv->zoomOut();
}

void
MainWindow::RotateClock() noexcept
{
    if (m_imgv)
        m_imgv->rotateClock();
}

void
MainWindow::RotateAnticlock() noexcept
{
    if (m_imgv)
        m_imgv->rotateAnticlock();
}

void
MainWindow::FitHeight() noexcept
{
    if (m_imgv)
        m_imgv->fitHeight();
}

void
MainWindow::FitWidth() noexcept
{
    if (m_imgv)
        m_imgv->fitWidth();
}

void
MainWindow::Scroll(Direction dir) noexcept
{
    switch (dir)
    {
        case Direction::LEFT:
            m_imgv->scrollLeft();
            break;

        case Direction::RIGHT:
            m_imgv->scrollRight();
            break;

        case Direction::UP:
            m_imgv->scrollUp();
            break;

        case Direction::DOWN:
            m_imgv->scrollDown();
            break;
    }
}

void
MainWindow::handleFileDrop() noexcept
{
}

void
MainWindow::ToggleMinimap() noexcept
{
    m_imgv->toggleMinimap();
}

void
MainWindow::dropEvent(QDropEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        QList<QString> files;
        for (const QUrl &url : e->mimeData()->urls())
        {
            QString file = url.toLocalFile();
            files.append(file);
        }
        OpenFiles(files);
    }
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls() || e->mimeData()->hasText())
        e->acceptProposedAction();
    else
        e->ignore();
}

QStringList
MainWindow::openFileDialog() noexcept
{
    QStringList extensions = {
#ifdef HAS_LIBAVIF
        "*.avif",
#endif
        "*.jpg",  "*.bmp",  "*.cgm", "*.dpx", "*.emf", "*.exr",  "*.fits", "*.gif", "*.heic", "*.heif",
        "*.jp2",  "*.jpeg", "*.jxl", "*.pcx", "*.png", "*.psd",  "*.sgi",  "*.svg", "*.tga",  "*.tiff",
        "*.ico",  "*.webp", "*.wmf", "*.xbm", "*.cr2", "*.crw",  "*.dds",  "*.eps", "*.raf",  "*.jng",
        "*.dcr",  "*.mrw",  "*.nef", "*.orf", "*.pef", "*.pict", "*.pnm",  "*.pbm", "*.pgm",  "*.ppm",
        "*.rgb",  "*.arw",  "*.srf", "*.sr2", "*.xcf", "*.xpm"};

    QString filter = QString("Image Files (%1);;All Files (*)").arg(extensions.join(' '));

    return QFileDialog::getOpenFileNames(this, "Open File", QString(), filter);
}

void
MainWindow::updateFileinfoInPanel() noexcept
{
    if (!m_imgv || !m_imgv->success())
        return;

    const QString &filepath = m_imgv->filePath();
    const QSize &size       = m_imgv->size();
    m_panel->setFileName(filepath);
    m_panel->setImageSize(size.width(), size.height());
    m_panel->setFileSize(m_imgv->fileSize());
    m_imgv->updateMinimapPosition();
    this->setWindowTitle(QString("iv: %1").arg(filepath));
}

void
MainWindow::initConfig() noexcept
{

    const QString config_file = CONFIG_DIR + QDir::separator() + "config.toml";
    toml::table toml;

    try
    {
        toml = toml::parse_file(config_file.toStdString());
    }
    catch (const toml::parse_error &e)
    {
        QMessageBox::critical(this, "Config Error", e.what());
        return;
    }

    // Read tab options
    auto ui = toml["ui"];

    if (ui)
    {
        m_config.ui.tabs_shown    = ui["shown"].value_or(true);
        m_config.ui.tabs_autohide = ui["auto_hide"].value_or(true);

        m_config.ui.hscrollbar_shown             = ui["shown"].value_or(true);
        m_config.ui.hscrollbar_auto_hide         = ui["auto_hide"].value_or(true);
        m_config.ui.vscrollbar_shown             = ui["shown"].value_or(true);
        m_config.ui.vscrollbar_auto_hide         = ui["auto_hide"].value_or(true);
        m_config.ui.minimap_shown                = ui["shown"].value_or(false);
        m_config.ui.auto_hide_minimap            = ui["auto_hide"].value_or(true);
        m_config.ui.minimap_overlay_color        = ui["color"].value_or("#55FF0000");
        m_config.ui.minimap_overlay_border_color = ui["border"].value_or("#5500FF00");
        m_config.ui.minimap_overlay_border_width = ui["border_width"].value_or(0);
    }

    auto rendering = toml["rendering"];

    // If DPR is specified in config, use that (can be scalar or map)
    if (rendering && rendering["dpr"])
    {
        if (rendering["dpr"].is_value())
        {
            m_config.rendering.dpr = rendering["dpr"].value_or(1.0f); // scalar
        }
        else if (rendering["dpr"].is_table())
        {
            auto dpr_table = rendering["dpr"];
            for (auto &[screen_name, value] : *dpr_table.as_table())
            {
                float dpr_value          = value.value_or(1.0f);
                QString screen_str       = QString::fromStdString(std::string(screen_name.str()));
                QList<QScreen *> screens = QApplication::screens();
                for (QScreen *screen : screens)
                {
                    if (screen->name() == screen_str)
                    {
                        m_screen_dpr_map[screen->name()] = dpr_value;
                        break;
                    }
                }
            }

            m_config.rendering.dpr = m_screen_dpr_map;
        }
    }
    else
    {
        m_config.rendering.dpr = m_screen_dpr_map.value(QApplication::primaryScreen()->name(), 1.0f);
    }

    // Read Keybindings

    auto keys = toml["keybindings"];

    if (keys)
    {
        for (auto &[action, value] : *keys.as_table())
        {
            if (value.is_value())
                setupKeybinding(QString::fromStdString(std::string(action.str())),
                                QString::fromStdString(value.value_or<std::string>("")));
        }
    }
}

void
MainWindow::initCommandMap() noexcept
{

    m_commandMap["reload_file"] = [this]()
    {
        if (m_imgv)
        {
            QString filepath = m_imgv->filePath();
            m_imgv->reloadFile();
        }
    };
    m_commandMap["open_file"] = [this]()
    {
        OpenFile();
    };

    m_commandMap["close_file"] = [this]()
    {
        CloseFile();
    };

    m_commandMap["toggle_tabs"] = [this]()
    {
        QTabBar *tabbar = m_tab_widget->tabBar();
        tabbar->setVisible(!tabbar->isVisible());
    };

    m_commandMap["toggle_fullscreen"] = [this]()
    {
        if (this->isFullScreen())
            this->showNormal();
        else
            this->showFullScreen();
    };

    m_commandMap["open_file_location"] = [this]()
    {
        QString filedir = qobject_cast<ImageView *>(m_tab_widget->currentWidget())->fileDir();
        QDesktopServices::openUrl(QUrl(filedir));
    };

    m_commandMap["zoom_in"] = [this]()
    {
        ZoomIn();
    };
    m_commandMap["zoom_out"] = [this]()
    {
        ZoomOut();
    };
    m_commandMap["rotate_clock"] = [this]()
    {
        RotateClock();
    };
    m_commandMap["rotate_anticlock"] = [this]()
    {
        RotateAnticlock();
    };
    m_commandMap["fit_width"] = [this]()
    {
        FitWidth();
    };
    m_commandMap["fit_height"] = [this]()
    {
        FitHeight();
    };

    m_commandMap["scroll_left"] = [this]()
    {
        Scroll(Direction::LEFT);
    };

    m_commandMap["scroll_down"] = [this]()
    {
        Scroll(Direction::DOWN);
    };

    m_commandMap["scroll_up"] = [this]()
    {
        Scroll(Direction::UP);
    };

    m_commandMap["scroll_right"] = [this]()
    {
        Scroll(Direction::RIGHT);
    };

    m_commandMap["toggle_minimap"] = [this]()
    {
        ToggleMinimap();
    };

    m_commandMap["flip_left"] = [this]()
    {
        Flip(Direction::LEFT);
    };

    m_commandMap["flip_right"] = [this]()
    {
        Flip(Direction::RIGHT);
    };

    m_commandMap["flip_up"] = [this]()
    {
        Flip(Direction::UP);
    };

    m_commandMap["flip_down"] = [this]()
    {
        Flip(Direction::DOWN);
    };

    // Command to switch to tabs 0-9 and also next and prev tab
    m_commandMap["next_tab"] = [this]()
    {
        NextTab();
    };
    m_commandMap["prev_tab"] = [this]()
    {
        PrevTab();
    };

    m_commandMap["tab_last"] = [this]()
    {
        int lastIndex = m_tab_widget->count() - 1;
        SwitchToTab(lastIndex);
    };

    m_commandMap["open_containing_folder"] = [this]()
    {
        OpenContainingFolder();
    };

    for(int i=1; i < 11; i++) {
        m_commandMap[QString("tab_%1").arg(i)] = [this, i]()
        {
            SwitchToTab(i - 1);
        };
    }
}

void
MainWindow::setupKeybinding(const QString &action, const QString &key) noexcept
{
    auto it = m_commandMap.find(action);
    if (it == m_commandMap.end())
        return;

    QShortcut *shortcut = new QShortcut(QKeySequence(key), this);
    connect(shortcut, &QShortcut::activated, this, [=]() { it.value()(); });

    m_config.shortcutMap[action] = key;
}

void
MainWindow::Flip(Direction dir) noexcept
{
    switch (dir)
    {
        case Direction::LEFT:
            m_imgv->flipLeftRight();
            break;

        case Direction::UP:
            m_imgv->flipUpDown();
            break;

        default:
            break;
    }
}

void MainWindow::NextTab() noexcept
{
    int currentIndex = m_tab_widget->currentIndex();
    int tabCount     = m_tab_widget->count();
    if (tabCount == 0)
        return;
    int nextIndex = (currentIndex + 1) % tabCount;
    m_tab_widget->setCurrentIndex(nextIndex);
}

void MainWindow::PrevTab() noexcept
{
    int currentIndex = m_tab_widget->currentIndex();
    int tabCount     = m_tab_widget->count();
    if (tabCount == 0)
        return;
    int prevIndex = (currentIndex - 1 + tabCount) % tabCount;
    m_tab_widget->setCurrentIndex(prevIndex);
}

void MainWindow::SwitchToTab(int index) noexcept
{
    int tabCount = m_tab_widget->count();
    if (index < 0 || index >= tabCount)
        return;
    m_tab_widget->setCurrentIndex(index);
}

void MainWindow::OpenContainingFolder() noexcept
{
    if (!m_imgv)
        return;
    QString filedir = m_imgv->fileDir();
    QDesktopServices::openUrl(QUrl(filedir));
}
