#pragma once

#include "Config.hpp"
#include "Panel.hpp"
#include "RecentFilesManager.hpp"
#include "TabWidget.hpp"
#include "argparse.hpp"

#include <QApplication>
#include <QMainWindow>
#include <QMimeData>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <qevent.h>

#define __IV_VERSION "0.2.0"
#define CONFIG_DIR QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + QDir::separator() + "iv"

class ImageView;

class MainWindow : public QMainWindow
{
public:
    MainWindow(QWidget *parent = nullptr);

    enum class Direction
    {
        LEFT = 0,
        RIGHT,
        UP,
        DOWN
    };

    void construct() noexcept;
    void readArgs(argparse::ArgumentParser &parser) noexcept;
    void initConnections() noexcept;
    void handleFileDrop() noexcept;

    void OpenFile(const QString &filepath = QString()) noexcept;
    void OpenFiles(const QStringList &files) noexcept;
    void OpenFiles(const std::vector<std::string> &files) noexcept;
    void CloseFile() noexcept;
    void ZoomIn() noexcept;
    void ZoomOut() noexcept;
    void RotateClock() noexcept;
    void RotateAnticlock() noexcept;
    void FitWidth() noexcept;
    void FitHeight() noexcept;
    void FitWindow() noexcept;
    void Scroll(Direction dir) noexcept;
    void Flip(Direction dir) noexcept;
    void ToggleMinimap() noexcept;
    void PrevTab() noexcept;
    void NextTab() noexcept;
    void SwitchToTab(int index) noexcept;
    void OpenContainingFolder() noexcept;
    void ToggleAutoReload() noexcept;
    void ToggleAutoFit() noexcept;
    void ShowFileProperties() noexcept;

    inline void ToggleStatusbar() const noexcept
    {
        m_panel->setVisible(!m_panel->isVisible());
    }

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    void initDefaultKeybinds() noexcept;
    void initCommandMap() noexcept;
    void initConfig() noexcept;
    void setupKeybinding(const QString &action, const QString &key) noexcept;
    void updateFileinfoInPanel() noexcept;
    QStringList openFileDialog() noexcept;
    void handleTabClose(int index) noexcept;
    void updateMenuActions(bool state) noexcept;
    void handleCurrentTabChanged(int index) noexcept;
    bool m_default_keybindings{true};

    QMenu *m_file_menu{nullptr};
    QMenu *m_view_menu{nullptr};
    QMenu *m_edit_menu{nullptr};
    QMenu *m_help_menu{nullptr};
    QMenu *m_recent_files_menu{nullptr};

    QMenu *m_zoom_menu{nullptr};
    QMenu *m_rotate_menu{nullptr};
    QMenu *m_fit_menu{nullptr};
    QMenu *m_flip_menu{nullptr};
    QMenu *m_toggle_menu{nullptr};

    QAction *m_open_file_action{nullptr};
    QAction *m_close_file_action{nullptr};
    QAction *m_exit_action{nullptr};
    QAction *m_zoom_in_action{nullptr};
    QAction *m_zoom_out_action{nullptr};
    QAction *m_rotate_clock_action{nullptr};
    QAction *m_rotate_anticlock_action{nullptr};
    QAction *m_fit_width_action{nullptr};
    QAction *m_fit_height_action{nullptr};
    QAction *m_fit_window_action{nullptr};
    QAction *m_auto_fit_action{nullptr};
    QAction *m_open_containing_folder_action{nullptr};
    QAction *m_toggle_minimap_action{nullptr};
    QAction *m_toggle_auto_reload_action{nullptr};
    QAction *m_toggle_panel_action{nullptr};
    QAction *m_file_properties_action{nullptr};


    TabWidget *m_tab_widget{new TabWidget()};
    Panel *m_panel{new Panel()};
    ImageView *m_imgv{nullptr};
    QMap<QString, std::function<void()>> m_commandMap;
    float m_dpr{1.0f};
    Config m_config;
    RecentFilesManager *m_recent_file_manager{nullptr};
    QMap<QString, float> m_screen_dpr_map; // DPR per screen
};
