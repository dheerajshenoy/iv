#pragma once

#include "Config.hpp"
#include "Panel.hpp"
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

#define __IV_VERSION "0.1.0"
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
    void Scroll(Direction dir) noexcept;
    void Flip(Direction dir) noexcept;
    void ToggleMinimap() noexcept;
    void PrevTab() noexcept;
    void NextTab() noexcept;
    void SwitchToTab(int index) noexcept;
    void OpenContainingFolder() noexcept;
    void ToggleAutoReload() noexcept;

protected:
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    void initDefaultKeybinds() noexcept;
    void initCommandMap() noexcept;
    void initConfig() noexcept;
    void setupKeybinding(const QString &action, const QString &key) noexcept;
    void updateFileinfoInPanel() noexcept;
    QStringList openFileDialog() noexcept;
    void handleTabClose(int index) noexcept;
    bool m_default_keybindings{true};

    TabWidget *m_tab_widget{new TabWidget()};
    Panel *m_panel{new Panel()};
    ImageView *m_imgv{nullptr};
    QMap<QString, std::function<void()>> m_commandMap;
    float m_dpr{1.0f};
    Config m_config;

    QMap<QString, float> m_screen_dpr_map; // DPR per screen
};
