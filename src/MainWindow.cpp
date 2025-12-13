#include "MainWindow.hpp"

#include "ImageView.hpp"
#include "toml.hpp"

#include <QActionGroup>
#include <QDesktopServices>
#include <QFileDialog>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QScreen>
#include <QShortcut>
#include <QTabBar>
#include <QTimer>
#include <QWindow>
#include <qnamespace.h>
#include <qshortcut.h>

void
MainWindow::readArgs(argparse::ArgumentParser &parser) noexcept
{

    // Check if version or commands flag is used
    if (parser.is_used("version"))
    {
        qDebug() << "Iv VERSION " << __IV_VERSION;
        exit(0);
    }

    if (parser.is_used("commands"))
    {
        initCommandMap();
        qInfo() << "Available commands:\n";
        for (auto it = m_commandMap.begin(); it != m_commandMap.end(); it++)
            qInfo() << it.key();
        exit(0);
    }

    if (parser.is_used("config"))
    {
        const QString configPath = QString::fromStdString(parser.get<std::string>("config"));
        if (!configPath.isEmpty())
            m_config_file_path = configPath;
    }
    else
    {
        m_config_file_path = CONFIG_DIR + "config.toml";
    }

    if (parser.is_used("not-tabbed"))
        m_not_tabbed = true;

    if (parser.is_used("files"))
    {
        auto files = parser.get<std::vector<std::string>>("files");
        if (m_not_tabbed)
            OpenFiles(files);
        else
        {
            // Construct the main window
            this->construct();
            OpenFiles(files);
        }
    }
    else
    {
        // Construct the main window
        this->construct();
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
    initCommandMap();
    initConfig();
    initConnections();

    m_file_menu = menuBar()->addMenu("&File");
    m_view_menu = menuBar()->addMenu("&View");
    m_help_menu = menuBar()->addMenu("&Help");

    initGui();
}

void
MainWindow::initGui() noexcept
{

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(m_tab_widget);
    layout->addWidget(m_panel);
    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    setCentralWidget(widget);

    m_open_file_action = m_file_menu->addAction(QString("Open File\t%1").arg(m_config.shortcutMap["open_file"]), this,
                                                &MainWindow::openFileDialog);

    m_recent_files_menu = m_file_menu->addMenu("Recent Files");
    m_recent_file_manager->PopulateRecentFilesMenu(
        m_recent_files_menu,
        std::function<void(const QString &)>([this](const QString &filepath) { OpenFile(filepath); }));

    m_open_containing_folder_action = m_file_menu->addAction(
        QString("Open Containing Folder\t%1").arg(m_config.shortcutMap["open_containing_folder"]), this,
        &MainWindow::OpenContainingFolder);
    
    m_edit_menu = menuBar()->addMenu("&Edit");
    
    m_copy_

    m_file_properties_action =
        m_file_menu->addAction(QString("File Properties\t%1").arg(m_config.shortcutMap["file_properties"]), this,
                               &MainWindow::ShowFileProperties);

    m_close_file_action = m_file_menu->addAction(QString("Close File\t%1").arg(m_config.shortcutMap["close_file"]),
                                                 this, &MainWindow::CloseFile);
    m_exit_action =
        m_file_menu->addAction(QString("Exit\t%1").arg(m_config.shortcutMap["exit"]), this, &MainWindow::close);

    m_zoom_menu = m_view_menu->addMenu("Zoom");

    m_zoom_in_action =
        m_zoom_menu->addAction(QString("In\t%1").arg(m_config.shortcutMap["zoom_in"]), this, &MainWindow::ZoomIn);

    m_zoom_out_action =
        m_zoom_menu->addAction(QString("Out\t%1").arg(m_config.shortcutMap["zoom_out"]), this, &MainWindow::ZoomOut);

    m_zoom_reset_action = m_zoom_menu->addAction(QString("Reset\t%1").arg(m_config.shortcutMap["zoom_reset"]), this,
                                                 &MainWindow::ZoomReset);

    m_rotate_menu         = m_view_menu->addMenu("Rotate");
    m_rotate_clock_action = m_rotate_menu->addAction(QString("Clockwise\t%1").arg(m_config.shortcutMap["rotate_clock"]),
                                                     this, &MainWindow::RotateClock);
    m_rotate_anticlock_action = m_rotate_menu->addAction(
        QString("Anticlockwise\t%1").arg(m_config.shortcutMap["rotate_anticlock"]), this, &MainWindow::RotateAnticlock);

    m_fit_menu = m_view_menu->addMenu("Fit");

    m_fit_width_action =
        m_fit_menu->addAction(QString("Width\t%1").arg(m_config.shortcutMap["fit_width"]), this, &MainWindow::FitWidth);
    m_fit_height_action = m_fit_menu->addAction(QString("Height\t%1").arg(m_config.shortcutMap["fit_height"]), this,
                                                &MainWindow::FitHeight);
    m_fit_window_action = m_fit_menu->addAction(QString("Window\t%1").arg(m_config.shortcutMap["fit_window"]), this,
                                                &MainWindow::FitWindow);

    m_auto_fit_action = m_fit_menu->addAction(QString("Auto Fit\t%1").arg(m_config.shortcutMap["auto_fit"]), this,
                                              &MainWindow::ToggleAutoFit);
    m_auto_fit_action->setCheckable(true);
    m_auto_fit_action->setChecked(m_config.behavior.auto_fit);

    m_flip_menu = m_view_menu->addMenu("Flip");

    m_flip_horizontal_action = m_flip_menu->addAction(
        QString("Horizontal\t%1").arg(m_config.shortcutMap["flip_horizontal"]), [&]() { Flip(Direction::LEFT); });
    m_flip_vertical_action = m_flip_menu->addAction(QString("Vertical\t%1").arg(m_config.shortcutMap["flip_vertical"]),
                                                    [&]() { Flip(Direction::UP); });

    m_toggle_menu           = m_view_menu->addMenu("Toggle");
    m_toggle_minimap_action = m_toggle_menu->addAction(
        QString("Minimap\t%1").arg(m_config.shortcutMap["toggle_minimap"]), this, &MainWindow::ToggleMinimap);
    m_toggle_minimap_action->setCheckable(true);
    m_toggle_minimap_action->setChecked(m_config.ui.minimap_shown);

    m_toggle_menubar_action = m_toggle_menu->addAction(
        QString("Menubar\t%1").arg(m_config.shortcutMap["toggle_menubar"]), this, &MainWindow::ToggleMenubar);
    m_toggle_menubar_action->setCheckable(true);
    m_toggle_menubar_action->setChecked(m_config.ui.menubar_shown);

    m_toggle_tabbar_action = m_toggle_menu->addAction(QString("Tabs\t%1").arg(m_config.shortcutMap["toggle_tabs"]),
                                                      this, &MainWindow::ToggleTabBar);
    m_toggle_tabbar_action->setCheckable(true);
    m_toggle_tabbar_action->setChecked(m_config.ui.tabs_shown);

    m_toggle_hscrollbar_action = m_toggle_menu->addAction(
        QString("H Scrollbar\t%1").arg(m_config.shortcutMap["toggle_hscrollbar"]), this, &MainWindow::ToggleHScrollBar);
    m_toggle_hscrollbar_action->setCheckable(true);
    m_toggle_hscrollbar_action->setChecked(m_config.ui.tabs_shown);

    m_toggle_vscrollbar_action = m_toggle_menu->addAction(
        QString("V Scrollbar\t%1").arg(m_config.shortcutMap["toggle_vscrollbar"]), this, &MainWindow::ToggleVScrollBar);
    m_toggle_vscrollbar_action->setCheckable(true);
    m_toggle_vscrollbar_action->setChecked(m_config.ui.tabs_shown);

    m_toggle_panel_action = m_toggle_menu->addAction(
        QString("Statusbar\t%1").arg(m_config.shortcutMap["toggle_statusbar"]), this, &MainWindow::ToggleStatusbar);
    m_toggle_panel_action->setCheckable(true);
    m_toggle_panel_action->setChecked(m_config.ui.statusbar_shown);

    m_toggle_auto_reload_action = m_toggle_menu->addAction(
        QString("Auto Reload\t%1").arg(m_config.shortcutMap["auto_reload"]), this, &MainWindow::ToggleAutoReload);
    m_toggle_auto_reload_action->setCheckable(true);
    m_toggle_auto_reload_action->setChecked(m_config.behavior.auto_reload);

    m_help_menu->addAction("About", this, [&]()
    {
        // TODO: Add custom widget
        QMessageBox::information(
            this, "About iv",
            QString("iv version %1\n\nA simple and fast image viewer built with Qt and ImageMagick.")
                .arg(__IV_VERSION));
    });

    this->setContentsMargins(0, 0, 0, 0);
    m_tab_widget->setContentsMargins(0, 0, 0, 0);
    layout->setContentsMargins(0, 0, 0, 0);
    widget->setContentsMargins(0, 0, 0, 0);

    // Remove frame and padding around tab widget to make it flush with window edges
    m_tab_widget->tabBar()->setStyleSheet("QTabBar { margin: 0; padding: 0; }");
    layout->setSpacing(0);
    m_tab_widget->setStyleSheet("border: 0;");
    m_tab_widget->tabBar()->setVisible(m_config.ui.tabs_shown);
    m_tab_widget->setTabBarAutoHide(m_config.ui.tabs_autohide);

    menuBar()->setVisible(m_config.ui.menubar_shown);
    m_panel->setVisible(m_config.ui.statusbar_shown);

    updateMenuActions(false);
    this->show();
}

void
MainWindow::initDefaultKeybinds() noexcept
{
    if (!m_default_keybindings)
        return;

    m_config.shortcutMap["m"]            = "toggle_minimap";
    m_config.shortcutMap["Ctrl+Shift+M"] = "toggle_menubar";
    m_config.shortcutMap["Ctrl+W"]       = "close_file";
    m_config.shortcutMap["o"]            = "open_file";
    m_config.shortcutMap["q"]            = "open_containing_folder";
    m_config.shortcutMap["="]            = "zoom_in";
    m_config.shortcutMap["-"]            = "zoom_out";
    m_config.shortcutMap[">"]            = "rotate_clock";
    m_config.shortcutMap["<"]            = "rotate_anticlock";
    m_config.shortcutMap["1"]            = "fit_width";
    m_config.shortcutMap["2"]            = "fit_height";
    m_config.shortcutMap["h"]            = "scroll_left";
    m_config.shortcutMap["j"]            = "scroll_down";
    m_config.shortcutMap["k"]            = "scroll_up";
    m_config.shortcutMap["l"]            = "scroll_right";
    m_config.shortcutMap["t"]            = "toggle_tabs";
    m_config.shortcutMap["F11"]          = "toggle_fullscreen";

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
    connect(m_tab_widget, &QTabWidget::currentChanged, this, &MainWindow::handleCurrentTabChanged);

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
    QProgressDialog progress("Opening files...", "Cancel", 0, files.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    for (const QString &filepath : files)
    {
        OpenFile(filepath);
        progress.setValue(progress.value() + 1);
        if (progress.wasCanceled())
            break;
    }
    progress.close();
}

void
MainWindow::OpenFiles(const std::vector<std::string> &files) noexcept
{
    QProgressDialog progress("Opening files...", "Cancel", 0, files.size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    for (const std::string &filepath : files)
    {
        OpenFile(QString::fromStdString(filepath));
        progress.setValue(progress.value() + 1);
        if (progress.wasCanceled())
            break;
    }
    progress.close();
}

void
MainWindow::OpenFileInNewWindow(const QString &filepath) noexcept
{
    MainWindow *win = new MainWindow();
    win->construct();
    // win->show();
    win->OpenFile(filepath);
}

void
MainWindow::OpenFile(const QString &filepath) noexcept
{
    if (m_not_tabbed)
    {
        OpenFileInNewWindow(filepath);
        return;
    }

    QString fp = filepath;

    if (fp.isEmpty())
    {
        QStringList filepaths = openFileDialog();
        OpenFiles(filepaths);
        return;
    }

    if (fp.startsWith("~"))
        fp = fp.replace(0, 1, QString::fromLocal8Bit(getenv("HOME")));

    m_imgv = new ImageView(m_config, m_tab_widget);

    bool success = m_imgv->openFile(filepath);
    if (!success)
    {
        qWarning() << "Failed to open file:" << filepath;
        QMessageBox::warning(this, "Open File Error", QString("Failed to open file:\n%1").arg(filepath));

        m_imgv->deleteLater();
        m_imgv = nullptr;
    }
    else
    {
        updateMenuActions(true);
        updateFileinfoInPanel();
        m_recent_file_manager->addFilePath(filepath);

        if (m_config.behavior.auto_reload)
            m_imgv->setAutoReload(true);

        m_tab_widget->addTab(m_imgv, fp);
        m_tab_widget->setCurrentWidget(m_imgv); // Make it the active tab

        connect(m_imgv, &ImageView::openFilesRequested, this,
                [&](const QStringList &files) { OpenFiles(files); }); // drop event
    }
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
MainWindow::ZoomReset() noexcept
{
    if (m_imgv)
        m_imgv->zoomReset();
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
MainWindow::FitWindow() noexcept
{
    if (m_imgv)
        m_imgv->fitWindow();
}

void
MainWindow::ToggleAutoFit() noexcept
{
    // Fit when window is resized
    if (m_imgv)
        m_imgv->setAutoFit(!m_imgv->autoFit());
}

void
MainWindow::ToggleAutoReload() noexcept
{
    if (m_imgv)
        m_imgv->toggleAutoReload();
}

void
MainWindow::ToggleMenubar() noexcept
{
    bool isVisible = menuBar()->isVisible();
    menuBar()->setVisible(!isVisible);
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
    if (!m_imgv)
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
    toml::table toml;

    try
    {
        toml = toml::parse_file(m_config_file_path.toStdString());
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
        m_config.ui.tabs_shown              = ui["tabs_shown"].value_or(true);
        m_config.ui.tabs_autohide           = ui["tabs_auto_hide"].value_or(true);
        m_config.ui.tab_bar_position        = ui["tab_bar_position"].value_or("top");
        m_config.ui.menubar_shown           = ui["menubar_shown"].value_or(true);
        m_config.ui.statusbar_shown         = ui["statusbar_shown"].value_or(true);
        m_config.ui.hscrollbar_shown        = ui["hscrollbar_shown"].value_or(true);
        m_config.ui.hscrollbar_auto_hide    = ui["hscrollbar_auto_hide"].value_or(true);
        m_config.ui.vscrollbar_shown        = ui["vscrollbar_shown"].value_or(true);
        m_config.ui.vscrollbar_auto_hide    = ui["vscrollbar_auto_hide"].value_or(true);
        m_config.ui.minimap_shown           = ui["minimap_shown"].value_or(false);
        m_config.ui.minimap_auto_hide       = ui["minimap_auto_hide"].value_or(true);
        m_config.ui.minimap_padding         = ui["minimap_padding"].value_or(10.0f);
        m_config.ui.minimap_clickable       = ui["minimap_clickable"].value_or(true);
        m_config.ui.minimap_overlay_movable = ui["minimap_overlay_movable"].value_or(true);
        m_config.ui.minimap_border_color    = ui["minimap_border_color"].value_or("#550000FF");
        m_config.ui.minimap_border_width    = ui["minimap_border_width"].value_or(1);

        if (ui["minimap_size"].is_table())
        {
            const auto size_table    = ui["minimap_size"];
            const int width          = size_table["width"].value_or(200);
            const int height         = size_table["height"].value_or(200);
            m_config.ui.minimap_size = QSize(width, height);
        }

        m_config.ui.minimap_image         = ui["minimap_image"].value_or(true);
        m_config.ui.minimap_image_opacity = ui["minimap_image_opacity"].value_or(0.7f);
        QString minimap_location          = ui["minimap_location"].value_or("bottom-right");

        {
            Minimap::Location loc;
            if (minimap_location == "top-left")
                loc = Minimap::Location::TOP_LEFT;
            else if (minimap_location == "top-center")
                loc = Minimap::Location::TOP_CENTER;
            else if (minimap_location == "top-right")
                loc = Minimap::Location::TOP_RIGHT;
            else if (minimap_location == "bottom-left")
                loc = Minimap::Location::BOTTOM_LEFT;
            else if (minimap_location == "bottom-center")
                loc = Minimap::Location::BOTTOM_CENTER;
            else if (minimap_location == "center")
                loc = Minimap::Location::CENTER;
            else if (minimap_location == "center-left")
                loc = Minimap::Location::CENTER_LEFT;
            else if (minimap_location == "center-right")
                loc = Minimap::Location::CENTER_RIGHT;
            else
                loc = Minimap::Location::BOTTOM_RIGHT;
            m_config.ui.minimap_location = loc;
        }

        m_config.ui.minimap_overlay_color        = ui["minimap_overlay_color"].value_or("#55FF0000");
        m_config.ui.minimap_overlay_border_color = ui["minimap_overlay_border"].value_or("#5500FF00");
        m_config.ui.minimap_overlay_border_width = ui["minimap_overlay_border_width"].value_or(0);
    }

    auto behavior = toml["behavior"];

    if (behavior)
    {
        m_config.behavior.auto_reload        = behavior["auto_reload"].value_or(false);
        m_config.behavior.save_recent_files  = behavior["save_recent_files"].value_or(true);
        m_config.behavior.recent_files_limit = behavior["recent_files_limit"].value_or(10);
        m_config.behavior.auto_fit           = behavior["auto_fit"].value_or(false);
        m_config.behavior.config_hot_reload  = behavior["config_hot_reload"].value_or(true);
    }

    if (m_config.behavior.config_hot_reload)
    {
        if (!m_config_file_watcher)
        {
            m_config_file_watcher = new QFileSystemWatcher(this);
            m_config_file_watcher->addPath(m_config_file_path);
            connect(m_config_file_watcher, &QFileSystemWatcher::fileChanged, this, &MainWindow::onConfigFileChanged);
        }
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

    if (m_config.behavior.save_recent_files)
    {
        if (!m_recent_file_manager)
            m_recent_file_manager =
                new RecentFilesManager(CONFIG_DIR + "recent_files.json", m_config.behavior.recent_files_limit);
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

    m_commandMap["left_edge"] = [this]()
    {
        m_imgv->scrollToLeftEdge();
    };

    m_commandMap["right_edge"] = [this]()
    {
        m_imgv->scrollToRightEdge();
    };

    m_commandMap["top_edge"] = [this]()
    {
        m_imgv->scrollToTopEdge();
    };

    m_commandMap["bottom_edge"] = [this]()
    {
        m_imgv->scrollToBottomEdge();
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
        ToggleTabBar();
    };

    m_commandMap["toggle_fullscreen"] = [this]()
    {
        if (this->isFullScreen())
            this->showNormal();
        else
            this->showFullScreen();
    };

    m_commandMap["toggle_menubar"] = [this]()
    {
        ToggleMenubar();
    };

    m_commandMap["zoom_in"] = [this]()
    {
        ZoomIn();
    };
    m_commandMap["zoom_out"] = [this]()
    {
        ZoomOut();
    };

    m_commandMap["zoom_reset"] = [this]()
    {
        ZoomReset();
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

    m_commandMap["fit_window"] = [this]()
    {
        FitWindow();
    };

    m_commandMap["auto_fit"] = [this]()
    {
        ToggleAutoFit();
    };

    m_commandMap["file_properties"] = [this]()
    {
        ShowFileProperties();
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

    m_commandMap["flip_horizontal"] = [this]()
    {
        Flip(Direction::LEFT);
    };

    m_commandMap["flip_vertical"] = [this]()
    {
        Flip(Direction::UP);
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

    for (int i = 1; i < 11; i++)
    {
        m_commandMap[QString("tab_%1").arg(i)] = [this, i]()
        {
            SwitchToTab(i - 1);
        };
    }
}

void
MainWindow::setupKeybinding(const QString &action, const QString &key) noexcept
{
    // Remove existing shortcut for this action, if any
    if (m_shortcut_map.contains(action))
    {
        QShortcut *existingShortcut = m_shortcut_map[action];
        existingShortcut->disconnect();
        existingShortcut->deleteLater();
        m_shortcut_map.remove(action);
    }

    const auto it = m_commandMap.find(action);
    if (it == m_commandMap.end())
        return;

    QShortcut *shortcut = new QShortcut(QKeySequence(key), this);
    connect(shortcut, &QShortcut::activated, this, [it]() { it.value()(); });
    m_config.shortcutMap[action] = key;
    m_shortcut_map[action]       = shortcut;
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

void
MainWindow::NextTab() noexcept
{
    int currentIndex = m_tab_widget->currentIndex();
    int tabCount     = m_tab_widget->count();
    if (tabCount == 0)
        return;
    int nextIndex = (currentIndex + 1) % tabCount;
    m_tab_widget->setCurrentIndex(nextIndex);
}

void
MainWindow::PrevTab() noexcept
{
    int currentIndex = m_tab_widget->currentIndex();
    int tabCount     = m_tab_widget->count();
    if (tabCount == 0)
        return;
    int prevIndex = (currentIndex - 1 + tabCount) % tabCount;
    m_tab_widget->setCurrentIndex(prevIndex);
}

void
MainWindow::SwitchToTab(int index) noexcept
{
    int tabCount = m_tab_widget->count();
    if (index < 0 || index >= tabCount)
        return;
    m_tab_widget->setCurrentIndex(index);
}

void
MainWindow::OpenContainingFolder() noexcept
{
    if (!m_imgv)
        return;
    QString filedir = m_imgv->fileDir();
    QDesktopServices::openUrl(QUrl(filedir));
}

void
MainWindow::updateMenuActions(bool state) noexcept
{
    m_zoom_menu->setEnabled(state);
    m_rotate_menu->setEnabled(state);
    m_fit_menu->setEnabled(state);
    m_close_file_action->setEnabled(state);
    m_toggle_auto_reload_action->setEnabled(state);
    m_open_containing_folder_action->setEnabled(state);
    m_file_properties_action->setEnabled(state);
    m_flip_menu->setEnabled(state);
}

void
MainWindow::handleCurrentTabChanged(int index) noexcept
{
    m_imgv = qobject_cast<ImageView *>(m_tab_widget->widget(index));
    updateMenuActions(m_imgv != nullptr);
    updateFileinfoInPanel();
}

void
MainWindow::resizeEvent(QResizeEvent *e)
{
    if (m_imgv && m_imgv->autoFit())
    {
        switch (m_imgv->fitMode())
        {
            case ImageView::FitMode::WIDTH:
                m_imgv->fitWidth();
                break;

            case ImageView::FitMode::HEIGHT:
                m_imgv->fitHeight();
                break;

            case ImageView::FitMode::WINDOW:
                m_imgv->fitWindow();
                break;

            default:
                break;
        }
    }

    QMainWindow::resizeEvent(e);
}

void
MainWindow::ShowFileProperties() noexcept
{
    if (!m_imgv)
        return;

    m_imgv->showFilePropertiesDialog();
}

void
MainWindow::onConfigFileChanged(const QString &path) noexcept
{
    // Stop watching temporarily to avoid duplicate signals
    m_config_file_watcher->removePath(path);

    // Use a single-shot timer to debounce rapid changes
    // and wait for file to be ready after replacement
    QTimer::singleShot(100, this, [this, path]()
    {
        if (!QFile::exists(path))
        {
            qWarning() << "Config file does not exist after change:" << path;
            // Try to re-add watch anyway in case it comes back
            m_config_file_watcher->addPath(path);
            return;
        }

        // Verify file is readable before reloading
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
        {
            qWarning() << "Config file exists but cannot be opened:" << path;
            m_config_file_watcher->addPath(path);
            return;
        }
        file.close();

        initConfig();
        applyConfigChanges();

        // Re-add the watch
        m_config_file_watcher->addPath(path);
    });
}

void
MainWindow::applyConfigChanges() noexcept
{
    // m_open_file_action->setShortcut(m_config.shortcutMap["open_file"]);
    // m_open_containing_folder_action->setShortcut(m_config.shortcutMap["open_containing_folder"]);
    // m_file_properties_action->setShortcut(m_config.shortcutMap["file_properties"]);
    // m_close_file_action->setShortcut(m_config.shortcutMap["close_file"]);
    // m_exit_action->setShortcut(m_config.shortcutMap["exit"]);
    // m_zoom_in_action->setShortcut(m_config.shortcutMap["zoom_in"]);
    // m_zoom_out_action->setShortcut(m_config.shortcutMap["zoom_out"]);
    // m_rotate_clock_action->setShortcut(m_config.shortcutMap["rotate_clock"]);
    // m_rotate_anticlock_action->setShortcut(m_config.shortcutMap["rotate_anticlock"]);
    // m_fit_width_action->setShortcut(m_config.shortcutMap["fit_width"]);
    // m_fit_height_action->setShortcut(m_config.shortcutMap["fit_height"]);
    // m_fit_window_action->setShortcut(m_config.shortcutMap["fit_window"]);
    // m_auto_fit_action->setShortcut(m_config.shortcutMap["auto_fit"]);
    // m_toggle_minimap_action->setShortcut(m_config.shortcutMap["toggle_minimap"]);
    // m_toggle_panel_action->setShortcut(m_config.shortcutMap["toggle_statusbar"]);
    // m_toggle_tabbar_action->setShortcut(m_config.shortcutMap["toggle_tabs"]);
    // m_toggle_auto_reload_action->setShortcut(m_config.shortcutMap["auto_reload"]);

    m_toggle_panel_action->setChecked(m_config.ui.statusbar_shown);
    m_toggle_minimap_action->setChecked(m_config.ui.minimap_shown);
    m_auto_fit_action->setChecked(m_config.behavior.auto_fit);
    m_toggle_auto_reload_action->setChecked(m_config.behavior.auto_reload);
    m_tab_widget->setTabPosition(tabBarPositionFromString(m_config.ui.tab_bar_position));
    m_tab_widget->tabBar()->setVisible(m_config.ui.tabs_shown);
    m_tab_widget->setTabBarAutoHide(m_config.ui.tabs_autohide);
    menuBar()->setVisible(m_config.ui.menubar_shown);
    m_panel->setVisible(m_config.ui.statusbar_shown);

    for (int i = 0; i < m_tab_widget->count(); i++)
    {
        auto temp = qobject_cast<ImageView *>(m_tab_widget->widget(i));
        if (temp)
        {
            temp->setConfig(m_config);
            temp->UpdateFromConfig();
        }
    }
}

QTabWidget::TabPosition
MainWindow::tabBarPositionFromString(const QString &locationStr) const noexcept
{
    QTabWidget::TabPosition loc;
    if (locationStr == "top")
        loc = QTabWidget::TabPosition::North;
    else if (locationStr == "bottom")
        loc = QTabWidget::TabPosition::South;
    else if (locationStr == "left")
        loc = QTabWidget::TabPosition::West;
    else if (locationStr == "right")
        loc = QTabWidget::TabPosition::East;
    else
        loc = QTabWidget::TabPosition::North;
    return loc;
}

void
MainWindow::ToggleTabBar() noexcept
{
    QTabBar *tabbar = m_tab_widget->tabBar();
    tabbar->setVisible(!tabbar->isVisible());
}

void
MainWindow::ToggleHScrollBar() noexcept
{
    if (m_imgv)
        m_imgv->toggleHScrollbar();
}

void
MainWindow::ToggleVScrollBar() noexcept
{
    if (m_imgv)
        m_imgv->toggleVScrollbar();
}
