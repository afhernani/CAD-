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

        bool isDraggingCommandScroll_ = false;
        int dragStartY_ = 0;
        int dragStartOffset_ = 0;

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
        
        bool showGrid_ = false;        // Cuadrícula activada/desactivada
        double gridBaseSize_ = 10.0;   // Tamaño base de la cuadrícula

        // --- Object Snaps ---
        bool isSnapped_;
        Point2D snappedPoint_;

        enum class SnapType { NONE, ENDPOINT, MIDPOINT, CENTER, INTERSECTION };
        SnapType currentSnapType_ = SnapType::NONE;

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

        // Convierte string UTF-8 a sf::String para renderizado correcto de acentos
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
        // Dibuja el feedback visual (entidad fantasma) según el modo actual
        void drawDrawingFeedback();
        // Dibuja los grips (pinzamientos) para las entidades seleccionadas
        void drawGrips();
        // Variables para Historial y Autocompletado
        int historyIndex_ = -1;
        int autocompleteIndex_ = -1;
        std::string autocompleteBase_;
        
    };

} // namespace cad