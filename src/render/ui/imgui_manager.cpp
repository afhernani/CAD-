#include "cad/render/ui/imgui_manager.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"

namespace cad {

ImGuiManager::ImGuiManager() {}

ImGuiManager::~ImGuiManager() {
    shutdown();
}

void ImGuiManager::init(sf::RenderWindow& window, const std::string& fontPath) {
    if (initialized_) return;
    
    ImGui::SFML::Init(window);
    
    // Cargar fuente
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
    ImGui::SFML::UpdateFontTexture();
    
    initialized_ = true;
}

void ImGuiManager::shutdown() {
    if (initialized_) {
        ImGui::SFML::Shutdown();
        initialized_ = false;
    }
}

void ImGuiManager::update(sf::RenderWindow& window) {
    static sf::Clock deltaClock;
    ImGui::SFML::Update(window, deltaClock.restart());
}

void ImGuiManager::render(sf::RenderWindow& window) {
    ImGui::SFML::Render(window);
}

void ImGuiManager::processEvent(const sf::Event& event, sf::RenderWindow& window) {
    ImGui::SFML::ProcessEvent(window, event);
}

bool ImGuiManager::wantCaptureMouse() const {
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiManager::wantCaptureKeyboard() const {
    return ImGui::GetIO().WantCaptureKeyboard;
}

// =========================================================================
// PANEL DE CAPAS
// =========================================================================

void ImGuiManager::toggleLayerPanel() {
    layerPanelVisible_ = !layerPanelVisible_;
}

bool ImGuiManager::isLayerPanelVisible() const {
    return layerPanelVisible_;
}

void ImGuiManager::drawLayerPanel(Engine& engine) {
    if (!layerPanelVisible_) return;
    
    ImGui::Begin("Gestor de Capas", &layerPanelVisible_);
    // El segundo parámetro (&layerPanelVisible_) permite cerrar la ventana con la X
    // y actualiza automáticamente la variable

    // Botón para crear nueva capa
    if (ImGui::Button("+ Nueva Capa")) {
        createNewLayer(engine);
    }

    ImGui::Separator();

    // Cabecera de columnas
    ImGui::Columns(5, "layerColumns", false);
    ImGui::Text("Nombre");
    ImGui::NextColumn();
    ImGui::Text("Color");
    ImGui::NextColumn();
    ImGui::Text("V");
    ImGui::NextColumn();
    ImGui::Text("F");
    ImGui::NextColumn();
    ImGui::Text("L");
    ImGui::NextColumn();
    ImGui::Separator();

    // Iterar sobre todas las capas
    int layerIndex = 0; // Para generar IDs únicos en ImGui
    for (auto& [name, layer] : engine.doc.layers) {
        bool isCurrentLayer = (name == engine.doc.currentLayerName);

        // 1. Nombre
        if (isCurrentLayer) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        
        if (ImGui::Selectable(name.c_str(), isCurrentLayer, ImGuiSelectableFlags_SpanAllColumns)) {
            engine.doc.setCurrentLayer(name);
            engine.statusMessage = "Capa actual: " + name;
        }
        
        if (isCurrentLayer) ImGui::PopStyleColor();
        ImGui::NextColumn();

        // 2. Color (Blindado)
        float color[3] = {
            layer.color.r / 255.0f,
            layer.color.g / 255.0f,
            layer.color.b / 255.0f
        };
        
        // Usamos el índice para el ID, evitando problemas con nombres de capa
        std::string colorId = "##color_" + std::to_string(layerIndex);
        if (ImGui::ColorEdit3(colorId.c_str(), color, 
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoAlpha)) {
            
            // Actualizamos el color en el documento
            layer.color.r = static_cast<sf::Uint8>(std::round(color[0] * 255.0f));
            layer.color.g = static_cast<sf::Uint8>(std::round(color[1] * 255.0f));
            layer.color.b = static_cast<sf::Uint8>(std::round(color[2] * 255.0f));
            
            engine.statusMessage = "Color de capa '" + name + "' actualizado.";
        }
        ImGui::NextColumn();

        // 3, 4, 5. Checkboxes (V, F, L)
        std::string visId = "##vis_" + std::to_string(layerIndex);
        if (ImGui::Checkbox(visId.c_str(), &layer.visible)) {
            engine.statusMessage = std::string("Capa '") + name + "' " + (layer.visible ? "visible" : "oculta") + ".";
        }
        ImGui::NextColumn();

        std::string frozenId = "##frozen_" + std::to_string(layerIndex);
        if (ImGui::Checkbox(frozenId.c_str(), &layer.frozen)) {
            engine.statusMessage = std::string("Capa '") + name + "' " + (layer.frozen ? "congelada" : "descongelada") + ".";
        }
        ImGui::NextColumn();

        std::string lockedId = "##locked_" + std::to_string(layerIndex);
        if (ImGui::Checkbox(lockedId.c_str(), &layer.locked)) {
            engine.statusMessage = std::string("Capa '") + name + "' " + (layer.locked ? "bloqueada" : "desbloqueada") + ".";
        }
        ImGui::NextColumn();

        layerIndex++;
    }

    ImGui::Columns(1);
    ImGui::End();
}

void ImGuiManager::createNewLayer(Engine& engine) {
    std::string newName = "Capa_" + std::to_string(engine.doc.layers.size() + 1);
    
    // Crear la capa con valores por defecto
    Layer newLayer;
    newLayer.name = newName;
    newLayer.color = sf::Color::White;
    newLayer.visible = true;
    newLayer.frozen = false;
    newLayer.locked = false;
    
    // Añadir al documento
    engine.doc.layers[newName] = newLayer;
    engine.doc.setCurrentLayer(newName);
    engine.statusMessage = "Capa '" + newName + "' creada y activada.";
}

// =========================================================================
// PANEL DE PROPIEDADES
// =========================================================================

void ImGuiManager::togglePropertiesPanel() {
    propertiesPanelVisible_ = !propertiesPanelVisible_;
}

bool ImGuiManager::isPropertiesPanelVisible() const {
    return propertiesPanelVisible_;
}
// FUNCIÓN AUXILIAR PARA OBTENER EL NOMBRE DEL TIPO
static std::string getEntityTypeName(const Entity* e) {
    if (dynamic_cast<const Line*>(e)) return "Línea";
    if (dynamic_cast<const Circle*>(e)) return "Círculo";
    if (dynamic_cast<const Arc*>(e)) return "Arco";
    if (dynamic_cast<const Polyline*>(e)) return "Polilínea";
    // Añade aquí los demás: Polygon, Ellipse, Dimension, BlockInsert, etc.
    return "Entidad Desconocida";
}

void ImGuiManager::drawPropertiesPanel(Engine& engine) {
    if (!propertiesPanelVisible_) return;
    
    ImGui::Begin("Propiedades", &propertiesPanelVisible_);
    
    if (engine.selectedEntities.empty()) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No hay entidades seleccionadas");
    } else {
        size_t count = engine.selectedEntities.size();
        ImGui::Text("%zu entidad(es) seleccionada(s)", count);
        ImGui::Separator();
        
        // Mostrar un resumen de TODAS las entidades seleccionadas
        for (size_t i = 0; i < count; ++i) {
            const Entity* ent = engine.selectedEntities[i];
            
            // Formato: "1. Tipo: Línea | Capa: 0"
            ImGui::Text("%zu. Tipo: %-10s | Capa: %s", 
                i + 1, 
                getEntityTypeName(ent).c_str(), 
                ent->layerName.c_str());
        }
        
        // Si solo hay 1 entidad, podemos preparar el terreno para mostrar sus datos específicos
        if (count == 1) {
            ImGui::Separator();
            ImGui::Text("Detalles específicos:");
            const Entity* first = engine.selectedEntities[0];
            ImGui::Text("ID Interno: %d", first->id);
            
            // Aquí en el futuro podrás añadir:
            // if (auto* line = dynamic_cast<const Line*>(first)) { ImGui::Text("Longitud: ..."); }
        }
    }
    
    ImGui::End();
}

} // namespace cad