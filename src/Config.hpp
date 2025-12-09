#pragma once

#include <QMap>
#include <QString>

struct Config
{

    // Minimap Settings
    bool minimap_shown;
    bool auto_hide_minimap;
    QString minimap_overlay_color;
    QString minimap_overlay_border_color;
    int minimap_overlay_border_width;

    // Scrollbar
    bool vscrollbar_shown;
    bool vscrollbar_auto_hide;
    bool hscrollbar_shown;
    bool hscrollbar_auto_hide;

    // Tabs
    bool tabs_shown;
    bool tabs_autohide;

    // Screen dpr scalar or map
    std::variant<float, QMap<QString, float>> dpr{};

    // Shortcuts
    QMap<QString, QString> shortcutMap;
};
