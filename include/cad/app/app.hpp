#pragma once
#include "cad/commands/engine.hpp"
#include "cad/core/snap/snap_engine.hpp"
#include "cad/render/view.hpp"
#include "cad/render/renderer.hpp"
#include "cad/render/ui/ui_manager.hpp"
#include "cad/render/ui/imgui_manager.hpp"
#include "cad/core/config/config.hpp"
#include "cad/core/constants.hpp"
#include <SFML/Graphics.hpp>
// #include "imgui.h"
// #include "imgui-SFML.h"
#include <string>
#include <vector>
#include <chrono>

namespace cad {

    class App {
    public:
        App();
        ~App();
        void run();

        bool confirmUnsavedChanges();
        void handleFileCommand(const std::string& upperCmd);


    private:
        sf::RenderWindow window_;
        sf::Font font_;
        Config config_; 

        // Los 4 pilares de la aplicación
        Engine engine_;
        View view_;
        Renderer renderer_;
        UIManager ui_;
        ImGuiManager imguiManager_;
        SnapEngine snapEngine_;
        SnapEngine::SnapResult lastSnapResult_;

        // --- Cursores ---
        sf::Cursor arrowCursor_;
        sf::Cursor transparentCursor_;
        bool cursorsLoaded_ = false;

        // --- Línea de comandos con historial ---
        std::string inputBuffer_;
        std::vector<std::string> commandHistory_;
        int commandScrollOffset_ = 0;
        bool isTyping_ = true;
        int historyIndex_ = -1;
        int autocompleteIndex_ = -1;
        std::string autocompleteBase_;

        // --- Scroll de comandos ---
        bool isDraggingCommandScroll_ = false;
        int dragStartY_ = 0;
        int dragStartOffset_ = 0;

        // --- Pan ---
        bool isPanning_ = false;
        sf::Vector2f panStartMouse_;

        // --- Control de visualización ---
        bool showAxes_ = true;
        // bool showEntityListPanel_ = false;
        std::string entityListText_;
        int entityListScrollOffset_ = 0;
        // panel de capas
        // bool showLayerPanel_ = false;
        //int layerPanelScrollOffset_ = 0;

        // --- Estado del ratón ---
        sf::Vector2i currentMouseScreenPos_;
        Point2D currentMouseWorldPos_;

        // // --- Constantes de layout ---
        // static constexpr unsigned int WINDOW_WIDTH = 1280;
        // static constexpr unsigned int WINDOW_HEIGHT = 720;
        // static constexpr unsigned int MENU_HEIGHT = 30;
        // static constexpr unsigned int TOOLBAR_HEIGHT = 60;
        // static constexpr unsigned int COMMAND_HEIGHT = 90;
        // static constexpr unsigned int STATUS_HEIGHT = 25;
        // static constexpr float CANVAS_HEIGHT = static_cast<float>(
        //     WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT);

        // --- Métodos Principales ---
        void handleEvents();
        void render();
        void updateEntityList();

        // void drawImGuiLayerManager();

        // --- Refactorización de Eventos (Dispatchers) ---
        void handleWindowEvent(const sf::Event& event);
        void handleMouseMovement(const sf::Event& event);
        void handleMouseScroll(const sf::Event& event);
        void handleMouseButton(const sf::Event& event);
        void handleKeyboardInput(const sf::Event& event);

        // --- Helpers de Detección de UI (Hit Testing) ---
        //bool isOverLayerPanel(int mx, int my) const;
        //bool isOverPropertiesPanel(int mx, int my) const;
        bool isOverCommandConsole(int mx, int my) const;
        bool isOverToolbar(int mx, int my) const;
        bool isOverCanvas(int mx, int my) const;

        // --- Helpers de Lógica de Negocio. Métodos auxiliares para refactorizar handleEvents ---
        void handleToolbarClick(int mx, int my);
        void handleCanvasClick(const Point2D& worldPoint);
        void processTextInput();
        void handleKeyboardNavigation(const sf::Event& event);
        void changeLayerColor(const std::string& layerName);

        // Helper UTF-8
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
        // Variables para la selección por ventana (Window / Crossing)
        bool isSelectingByWindow_ = false;
        Point2D selectionStartPoint_;
        Point2D selectionEndPoint_;
        
        // Constante compartida
        static constexpr int CHARS_PER_PIXEL_FACTOR = 9;
        
    };

} // namespace cad