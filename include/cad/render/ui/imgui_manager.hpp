#pragma once
#include "imgui.h"
#include "imgui-SFML.h"
#include "cad/commands/engine.hpp"
#include <SFML/Graphics.hpp>
#include <string>

namespace cad {

class ImGuiManager {
public:
    ImGuiManager();
    ~ImGuiManager();
    
    // Ciclo de vida
    void init(sf::RenderWindow& window, const std::string& fontPath);
    void shutdown();
    
    // Actualización y renderizado (llamar en App::render)
    void update(sf::RenderWindow& window);
    void render(sf::RenderWindow& window);
    
    // Procesar eventos (llamar en App::handleEvents)
    void processEvent(const sf::Event& event, sf::RenderWindow& window);
    
    // Consultas de estado (para que App sepa si ImGui captura input)
    bool wantCaptureMouse() const;
    bool wantCaptureKeyboard() const;
    
    // Gestión del Panel de Capas
    void toggleLayerPanel();
    bool isLayerPanelVisible() const;
    void drawLayerPanel(Engine& engine);
    
    // Gestión del Panel de Propiedades
    void togglePropertiesPanel();
    bool isPropertiesPanelVisible() const;
    void drawPropertiesPanel(Engine& engine);

    // Dibujar la barra de menú principal
    void drawMainMenu(Engine& engine, bool& showAxes);
    // Gestión de archivos recientes
    void addRecentFile(const std::string& path);
    
private:
    bool initialized_ = false;
    bool layerPanelVisible_ = false;
    bool propertiesPanelVisible_ = false;

    sf::RenderWindow* windowPtr_ = nullptr;
    
    // Helper para crear capas
    void createNewLayer(Engine& engine);
    std::vector<std::string> recentFiles_;
};

} // namespace cad