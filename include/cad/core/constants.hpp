// include/cad/core/constants.hpp
#pragma once

namespace cad {
    // Ajusta estos valores a los que uses realmente en tu proyecto
    constexpr float WINDOW_WIDTH = 800.0f;
    constexpr float WINDOW_HEIGHT = 600.0f;
    constexpr float MENU_HEIGHT = 30.0f;
    constexpr float TOOLBAR_HEIGHT = 40.0f;
    constexpr float COMMAND_HEIGHT = 100.0f;
    constexpr float STATUS_HEIGHT = 30.0f;
    
    // El alto del canvas se calcula restando las barras de la altura total
    constexpr float CANVAS_HEIGHT = WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT;
}