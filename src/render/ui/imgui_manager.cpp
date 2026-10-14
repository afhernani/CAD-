#include "cad/render/ui/imgui_manager.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include <windows.h>    // AÑADIR para el diálogo de color
#include <commdlg.h>
#include <iostream>

// Función auxiliar local para abrir el diálogo de color de Windows
namespace {
    sf::Color showColorDialog(sf::Color initialColor, void* windowHandle) {
        CHOOSECOLORA cc;
        static COLORREF customColors[16];
        
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = static_cast<HWND>(windowHandle); // Usamos el handle de la ventana
        cc.lpCustColors = customColors;
        cc.rgbResult = RGB(initialColor.r, initialColor.g, initialColor.b);
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;

        if (ChooseColorA(&cc)) {
            COLORREF chosenColor = cc.rgbResult;
            return sf::Color(GetRValue(chosenColor), GetGValue(chosenColor), GetBValue(chosenColor));
        }
        
        return initialColor; // Si el usuario cancela, devuelve el color original
    }
}

namespace cad {

ImGuiManager::ImGuiManager() {}

ImGuiManager::~ImGuiManager() {
    shutdown();
}

void ImGuiManager::init(sf::RenderWindow& window, const std::string& fontPath) {
    if (initialized_) return;
    windowPtr_ = &window; //guardar el puntero a la ventana.
    
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

    // Botón para crear nueva capa (TU CÓDIGO ORIGINAL)
    if (ImGui::Button("+ Nueva Capa")) {
        createNewLayer(engine);
    }

    ImGui::Separator();

    // Cabecera de columnas (TU CÓDIGO ORIGINAL)
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
    int layerIndex = 0; 
    for (auto& [name, layer] : engine.doc.layers) {
        bool isCurrentLayer = (name == engine.doc.currentLayerName);

        // 1. Nombre (TU CÓDIGO ORIGINAL)
        if (isCurrentLayer) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
        
        if (ImGui::Selectable(name.c_str(), isCurrentLayer, ImGuiSelectableFlags_SpanAllColumns)) {
            engine.doc.setCurrentLayer(name);
            engine.statusMessage = "Capa actual: " + name;
        }
        
        if (isCurrentLayer) ImGui::PopStyleColor();
        ImGui::NextColumn();

        // =====================================================================
        // 2. Color (MODIFICADO: InvisibleButton para garantizar clic en toda el área)
        // =====================================================================
        std::string colorBtnId = "##colorBtn_" + std::to_string(layerIndex);
        ImVec2 btnSize(26, 20); // Tamaño del área clicable

        // El InvisibleButton captura el clic en el 100% del rectángulo, sin fallos de alineación
        if (ImGui::InvisibleButton(colorBtnId.c_str(), btnSize)) {
            std::cout << "[DEBUG] ¡Clic registrado en el color de la capa: " << name << "!" << std::endl;
        
            if (windowPtr_) {
                // Pasamos el handle nativo de la ventana de SFML
                sf::Color newColor = showColorDialog(layer.color, windowPtr_->getSystemHandle());
                layer.color = newColor;
                engine.statusMessage = "Color de capa '" + name + "' actualizado.";
            } else {
                std::cerr << "[ERROR] windowPtr_ es nulo." << std::endl;
            }
        }
        
        // Dibujar el color manualmente sobre el área exacta del botón invisible
        ImVec2 p_min = ImGui::GetItemRectMin();
        ImVec2 p_max = ImGui::GetItemRectMax();
        
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(
            layer.color.r / 255.0f,
            layer.color.g / 255.0f,
            layer.color.b / 255.0f,
            1.0f
        ));
        
        // Rellenar el color y añadir un borde sutil para que se vea bien
        ImGui::GetWindowDrawList()->AddRectFilled(p_min, p_max, col);
        ImGui::GetWindowDrawList()->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border));

        // Tooltip al pasar el ratón
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Cambiar color de la capa '%s'", name.c_str());
        }
        
        ImGui::NextColumn();
        // =====================================================================

        // 3, 4, 5. Checkboxes (V, F, L) (TU CÓDIGO ORIGINAL)
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

        if (count > 1) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), 
                               "Nota: La geometría mostrada corresponde a la 1ª entidad.");
            ImGui::Separator();
        }

        Entity* firstEntity = engine.selectedEntities[0];

        // =========================================================================
        // 1. PROPIEDAD COMÚN: CAMBIAR CAPA (Aplica a TODAS las seleccionadas)
        // =========================================================================
        ImGui::Text("Capa:");
        
        // Construir la lista de capas separada por '\0' para el Combo de ImGui
        std::string layersStr;
        for (const auto& pair : engine.doc.layers) {
            layersStr += pair.first + "\0";
        }
        layersStr += "\0"; // Doble terminador nulo requerido por ImGui

        const char* preview = firstEntity->layerName.c_str();
        if (ImGui::BeginCombo("##LayerCombo", preview)) {
            for (const auto& pair : engine.doc.layers) {
                bool is_selected = (firstEntity->layerName == pair.first);
                if (ImGui::Selectable(pair.first.c_str(), is_selected)) {
                    // Aplicar el cambio de capa a TODAS las entidades seleccionadas
                    for (Entity* ent : engine.selectedEntities) {
                        ent->layerName = pair.first;
                    }
                    engine.doc.isModified = true;
                    engine.statusMessage = "Capa cambiada a: " + pair.first;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();
        ImGui::Text("Geometría:");

        // =========================================================================
        // 2. PROPIEDADES ESPECÍFICAS POR TIPO (Editables en tiempo real)
        // =========================================================================
        
        // --- LÍNEA ---
        if (auto* line = dynamic_cast<Line*>(firstEntity)) {
            ImGui::InputDouble("P1 X", &line->p1.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("P1 Y", &line->p1.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("P2 X", &line->p2.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("P2 Y", &line->p2.y, 0.1, 1.0, "%.4f");
            
            // Mostrar longitud calculada (solo lectura)
            double length = std::hypot(line->p2.x - line->p1.x, line->p2.y - line->p1.y);
            ImGui::Text("Longitud: %.4f", length);
            
            engine.doc.isModified = true; // Marcar como modificado al editar
        }
        
        // --- CÍRCULO ---
        else if (auto* circle = dynamic_cast<Circle*>(firstEntity)) {
            ImGui::InputDouble("Centro X", &circle->center.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Centro Y", &circle->center.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Radio", &circle->radius, 0.1, 1.0, "%.4f");
            if (circle->radius < 0.0) circle->radius = 0.0; // Evitar radios negativos
            
            engine.doc.isModified = true;
        }
        
        // --- ARCO ---
        else if (auto* arc = dynamic_cast<Arc*>(firstEntity)) {
            ImGui::InputDouble("Centro X", &arc->center.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Centro Y", &arc->center.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Radio", &arc->radius, 0.1, 1.0, "%.4f");
            if (arc->radius < 0.0) arc->radius = 0.0;
            
            ImGui::InputDouble("Ángulo Inicio (°)", &arc->startAngle, 1.0, 5.0, "%.2f");
            ImGui::InputDouble("Ángulo Fin (°)", &arc->endAngle, 1.0, 5.0, "%.2f");
            
            engine.doc.isModified = true;
        }
        
        // --- POLILÍNEA ---
        else if (auto* pline = dynamic_cast<Polyline*>(firstEntity)) {
            ImGui::Text("Vértices: %zu", pline->points.size());
            
            // Permitir cambiar si está cerrada o abierta
            bool isClosed = pline->closed;
            if (ImGui::Checkbox("Polilínea Cerrada", &isClosed)) {
                pline->closed = isClosed;
                engine.doc.isModified = true;
            }
            
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), 
                               "(Edición de vértices individuales pendiente de implementación)");
        }
        
        // --- ENTIDAD DESCONOCIDA ---
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Tipo de entidad no soportado para edición.");
        }
    }
    
    ImGui::End();
}

} // namespace cad