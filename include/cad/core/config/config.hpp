#pragma once
#include <string>
#include <unordered_map>

namespace cad {

// Estructura que agrupa la configuración por secciones
struct Config {
    struct Window {
        int width = 1280;
        int height = 720;
        std::string title = "CAD+ v0.5";
    };

    struct UI {
        int menu_height = 30;
        int toolbar_height = 60;
        int command_height = 90;
        int status_height = 25;
    };

    struct Snap {
        double pixel_tolerance = 10.0;
        bool enabled = true;
    };

    struct Grid {
        bool enabled = false;
        double base_size = 10.0;
    };

    struct Paths {
        std::string font = "assets/arial.ttf";
    };

    Window window;
    UI ui;
    Snap snap;
    Grid grid;
    Paths paths;

    // Carga la configuración desde un fichero JSON
    // Devuelve true si se cargó correctamente, false si usa valores por defecto
    bool loadFromFile(const std::string& path);

    // Guarda la configuración actual (útil si el usuario cambia ajustes)
    bool saveToFile(const std::string& path) const;
};

} // namespace cad