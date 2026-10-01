// include/cad/core/constants.hpp
#pragma once

namespace cad {
    // Ajusta estos valores a los que uses realmente en tu proyecto
    constexpr float WINDOW_WIDTH = 1280.0f;
    constexpr float WINDOW_HEIGHT = 720.0f;
    constexpr float MENU_HEIGHT = 30.0f;
    constexpr float TOOLBAR_HEIGHT = 60.0f;
    constexpr float COMMAND_HEIGHT = 90.0f;
    constexpr float STATUS_HEIGHT = 25.0f;
    
    // El alto del canvas se calcula restando las barras de la altura total
    constexpr float CANVAS_HEIGHT = WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT;
}