#pragma once
#include "engine.hpp"
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <chrono>

namespace cad {

    class App {
    public:
        App();
        void run();

    private:
        sf::RenderWindow window_;
        sf::Font font_;
        Engine engine_;
        
        // --- Cursores personalizados ---
        sf::Cursor crosshairCursor_;
        sf::Cursor arrowCursor_;
        sf::Cursor transparentCursor_;  // Cursor invisible
        bool cursorsLoaded_ = false;
        bool cursorVisible_ = true;  // Controla si el cursor del sistema es visible 
        bool showNativeCursor_ = true; //para debug

        // --- Línea de comandos con historial ---
        std::string inputBuffer_;
        std::vector<std::string> commandHistory_;
        int commandScrollOffset_ = 0;
        bool isTyping_ = true; 

        // --- Sistema de Vista (Zoom y Pan) ---
        float viewScale_;
        float viewPanX_;
        float viewPanY_;
        
        bool isPanning_ = false;
        sf::Vector2f panStartMouse_;
        float panStartViewX_;
        float panStartViewY_;

        // --- Control de visualización ---
        bool showAxes_;
        sf::Vector2f currentMouseWorldPos_;
        sf::Vector2i currentMouseScreenPos_;  // Posición del ratón en píxeles de pantalla

        // --- Object Snaps ---
        bool isSnapped_;
        Point2D snappedPoint_;

        // --- Dimensiones ---
        static constexpr unsigned int WINDOW_WIDTH = 1280;
        static constexpr unsigned int WINDOW_HEIGHT = 720;
        static constexpr unsigned int MENU_HEIGHT = 30;
        static constexpr unsigned int TOOLBAR_HEIGHT = 60;
        static constexpr unsigned int COMMAND_HEIGHT = 90;
        static constexpr unsigned int STATUS_HEIGHT = 25;
        static constexpr unsigned int CANVAS_HEIGHT = WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT;

        // --- Métodos ---
        void handleEvents();
        void render();
        
        sf::Vector2f worldToScreen(double wx, double wy);
        sf::Vector2f screenToWorld(float sx, float sy);
        
        void drawGrid();
        void drawAxes();
        void drawEntities();
        void drawUI();
        void drawToolbar();
        void drawCommandWindow();
        void findSnap();
        void drawCrosshair();

        // Buffer para mostrar ayuda en ventana de comandos
        std::vector<std::string> helpLines_;
        bool showingHelp_ = false;
        
    };

} // namespace cad