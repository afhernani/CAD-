#include "cad/core/config/config.hpp"
#include "../third_party/json.hpp"  // nlohmann/json
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace cad {

bool Config::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[Config] No se pudo abrir '" << path 
                  << "'. Usando valores por defecto." << std::endl;
        return false;
    }

    try {
        json j;
        file >> j;

        // Sección window
        if (j.contains("window")) {
            auto& w = j["window"];
            if (w.contains("width")) window.width = w["width"].get<int>();
            if (w.contains("height")) window.height = w["height"].get<int>();
            if (w.contains("title")) window.title = w["title"].get<std::string>();
        }

        // Sección ui
        if (j.contains("ui")) {
            auto& u = j["ui"];
            if (u.contains("menu_height")) ui.menu_height = u["menu_height"].get<int>();
            if (u.contains("toolbar_height")) ui.toolbar_height = u["toolbar_height"].get<int>();
            if (u.contains("command_height")) ui.command_height = u["command_height"].get<int>();
            if (u.contains("status_height")) ui.status_height = u["status_height"].get<int>();
        }

        // Sección snap
        if (j.contains("snap")) {
            auto& s = j["snap"];
            if (s.contains("pixel_tolerance")) snap.pixel_tolerance = s["pixel_tolerance"].get<double>();
            if (s.contains("enabled")) snap.enabled = s["enabled"].get<bool>();
        }

        // Sección grid
        if (j.contains("grid")) {
            auto& g = j["grid"];
            if (g.contains("enabled")) grid.enabled = g["enabled"].get<bool>();
            if (g.contains("base_size")) grid.base_size = g["base_size"].get<double>();
        }

        // Sección paths
        if (j.contains("paths")) {
            auto& p = j["paths"];
            if (p.contains("font")) paths.font = p["font"].get<std::string>();
        }

        std::cout << "[Config] Configuración cargada desde '" << path << "'" << std::endl;
        return true;

    } catch (const json::exception& e) {
        std::cerr << "[Config] Error parseando JSON: " << e.what() << std::endl;
        std::cerr << "[Config] Usando valores por defecto." << std::endl;
        return false;
    }
}

bool Config::saveToFile(const std::string& path) const {
    json j;
    j["window"] = {
        {"width", window.width},
        {"height", window.height},
        {"title", window.title}
    };
    j["ui"] = {
        {"menu_height", ui.menu_height},
        {"toolbar_height", ui.toolbar_height},
        {"command_height", ui.command_height},
        {"status_height", ui.status_height}
    };
    j["snap"] = {
        {"pixel_tolerance", snap.pixel_tolerance},
        {"enabled", snap.enabled}
    };
    j["grid"] = {
        {"enabled", grid.enabled},
        {"base_size", grid.base_size}
    };
    j["paths"] = {
        {"font", paths.font}
    };

    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << j.dump(4);  // 4 = indentación bonita
    return true;
}

} // namespace cad