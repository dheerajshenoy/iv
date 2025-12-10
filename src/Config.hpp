#pragma once

#include <QMap>
#include <QString>

struct Config
{

    struct UI
    {
        bool minimap_shown{true};
        bool auto_hide_minimap{true};
        QString minimap_overlay_color{"#55FF0000"};
        QString minimap_overlay_border_color{"#5500FF00"};
        int minimap_overlay_border_width{1};
        bool vscrollbar_shown{true};
        bool vscrollbar_auto_hide{true};
        bool hscrollbar_shown{true};
        bool hscrollbar_auto_hide{true};
        bool tabs_shown{true};
        bool tabs_autohide{true};
    };

    struct Rendering
    {
        std::variant<float, QMap<QString, float>> dpr{};
    };

    struct Behavior
    {
        bool auto_reload{false};
    };

    QMap<QString, QString> shortcutMap;
    UI ui{};
    Rendering rendering{};
    Behavior behavior{};
};
