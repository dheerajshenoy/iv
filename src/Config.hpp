#pragma once

#include "Minimap.hpp"

#include <QMap>
#include <QSize>
#include <QString>

struct Config
{

    struct UI
    {
        bool minimap_shown{true};
        bool minimap_auto_hide{true};
        bool minimap_image{true};
        float minimap_padding{10.0f};
        QSize minimap_size{QSize(200, 200)};
        float minimap_image_opacity{0.7f};
        Minimap::Location minimap_location{Minimap::Location::BOTTOM_RIGHT};
        bool minimap_overlay_movable{true};
        bool minimap_clickable{true};
        QString minimap_border_color{"#550000FF"};
        int minimap_border_width{1};

        QString minimap_overlay_color{"#55FF0000"};
        QString minimap_overlay_border_color{"#5500FF00"};
        int minimap_overlay_border_width{1};

        bool vscrollbar_shown{true};
        bool vscrollbar_auto_hide{true};

        bool hscrollbar_shown{true};
        bool hscrollbar_auto_hide{true};

        bool tabs_shown{true};
        bool tabs_autohide{true};
        QString tab_bar_position{"top"};

        bool statusbar_shown{true};
        bool menubar_shown{true};
    };

    struct Rendering
    {
        std::variant<float, QMap<QString, float>> dpr{};
    };

    struct Behavior
    {
        bool auto_reload{false};
        bool config_hot_reload{true};
        bool save_recent_files{true};
        int recent_files_limit{10};
        bool auto_fit{false};
    };

    QMap<QString, QString> shortcutMap;
    UI ui{};
    Rendering rendering{};
    Behavior behavior{};
};
