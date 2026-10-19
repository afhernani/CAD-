#include "cad/render/ui/imgui_manager.hpp"
#include "../../../assets/IconsFontAwesome6.h"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/ellipse.hpp"
#include "cad/core/geometry/entities/dimension.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/core/geometry/entities/image.hpp"      // Ajusta el nombre si tu archivo se llama diferente
#include "cad/core/geometry/entities/text.hpp"
#include "cad/core/geometry/entities/hatch.hpp"
#include <windows.h>    // AÑADIR para el diálogo de color
#include <commdlg.h>
#include <iostream>
#include <cmath> // Para M_PI o conversiones de ángulos
#include <filesystem> 

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
    
    // AÑADIR ESTO: Cambia el estilo de ImGui a un tema oscuro profesional
    ImGui::StyleColorsDark(); 
    
    // Opcional: Ajustes finos para que encaje mejor con tu CAD
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;      // Bordes ligeramente redondeados
    style.FrameRounding = 2.0f;       // Botones y inputs redondeados
    style.ScrollbarRounding = 4.0f;   // Barras de scroll redondeadas

    // Cargar fuente
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 16.0f);
    ImGui::SFML::UpdateFontTexture();
    
    // Mostrar el directorio de trabajo actual
    std::filesystem::path cwd = std::filesystem::current_path();
    std::cout << "[DEBUG] Directorio de trabajo actual: " << cwd << std::endl;

    // Probar varias rutas candidatas hasta encontrar los iconos
    std::vector<std::filesystem::path> candidatePaths = {
        cwd / "assets" / "icons",                          // Si se ejecuta desde la raíz
        cwd / ".." / "assets" / "icons",                   // Si se ejecuta desde build/
        cwd / ".." / ".." / "assets" / "icons",            // Si se ejecuta desde build/Release/
        "E:/Store/Docs/github/cad/assets/icons"            // Ruta absoluta de respaldo
    };

    std::filesystem::path iconsDir;
    for (const auto& path : candidatePaths) {
        if (std::filesystem::exists(path)) {
            iconsDir = path;
            std::cout << "[DEBUG] Iconos encontrados en: " << iconsDir << std::endl;
            break;
        }
    }

    if (iconsDir.empty()) {
        std::cout << "[DEBUG] No se encontró la carpeta de iconos en ninguna ruta candidata." << std::endl;
        initialized_ = true;
        return;
    }

    // Cargar iconos personalizados para la toolbar
    // Ajusta la ruta relativa según dónde esté tu ejecutable (normalmente build/Release/)
    // loadIconTexture("line", "../../assets/icons/draw_line.png");
    // loadIconTexture("circle", "../../assets/icons/draw_circle.png");
    // loadIconTexture("arc", "../../assets/icons/draw_arc.png");
    // loadIconTexture("text", "../../assets/icons/draw_text.png");

    // Cargar iconos con los nombres que TÚ estás usando (según tu log)
    loadIconTexture("line", (iconsDir / "draw_line.png").string());
    loadIconTexture("circle", (iconsDir / "draw_circle.png").string());
    loadIconTexture("arc", (iconsDir / "draw_arc.png").string());
    loadIconTexture("text", (iconsDir / "draw_text.png").string());

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
        ImGui::End();
        return;
    }

    size_t count = engine.selectedEntities.size();
    
    // Cabecera de selección
    ImGui::Text("Selección:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%zu entidad(es)", count);
    
    if (count > 1) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(Mostrando datos de la 1ª entidad)");
    }
    ImGui::Separator();

    Entity* firstEntity = engine.selectedEntities[0];

    // =========================================================================
    // 1. GRUPO: CAPA (Siempre visible y accesible)
    // =========================================================================
    ImGui::Text("Capa:");
    std::string layersStr;
    for (const auto& pair : engine.doc.layers) {
        layersStr += pair.first + "\0";
    }
    layersStr += "\0";

    const char* preview = firstEntity->layerName.c_str();
    if (ImGui::BeginCombo("##LayerCombo", preview)) {
        for (const auto& pair : engine.doc.layers) {
            bool is_selected = (firstEntity->layerName == pair.first);
            if (ImGui::Selectable(pair.first.c_str(), is_selected)) {
                for (Entity* ent : engine.selectedEntities) {
                    ent->layerName = pair.first;
                }
                engine.doc.isModified = true;
                engine.statusMessage = "Capa cambiada a: " + pair.first;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::Spacing();

    // =========================================================================
    // 2. GRUPO: GEOMETRÍA (Colapsable para ahorrar espacio)
    // =========================================================================
    if (ImGui::CollapsingHeader("Geometría", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(); // Sangría para dar sensación de grupo
        
        // --- LÍNEA ---
        if (auto* line = dynamic_cast<Line*>(firstEntity)) {
            ImGui::InputDouble("P1 X", &line->p1.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("P1 Y", &line->p1.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("P2 X", &line->p2.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("P2 Y", &line->p2.y, 0.1, 1.0, "%.4f");
            double length = std::hypot(line->p2.x - line->p1.x, line->p2.y - line->p1.y);
            ImGui::Text("Longitud: %.4f", length);
            engine.doc.isModified = true;
        }
        // --- CÍRCULO ---
        else if (auto* circle = dynamic_cast<Circle*>(firstEntity)) {
            ImGui::InputDouble("Centro X", &circle->center.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Centro Y", &circle->center.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Radio", &circle->radius, 0.1, 1.0, "%.4f");
            if (circle->radius < 0.0) circle->radius = 0.0;
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
            bool isClosed = pline->closed;
            if (ImGui::Checkbox("Polilínea Cerrada", &isClosed)) {
                pline->closed = isClosed;
                engine.doc.isModified = true;
            }
        }
        // --- POLÍGONO ---
        else if (auto* poly = dynamic_cast<Polygon*>(firstEntity)) {
            bool geometryChanged = false;
            ImGui::InputDouble("Centro X", &poly->center.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Centro Y", &poly->center.y, 0.1, 1.0, "%.4f");
            if (ImGui::InputInt("Número de Lados", &poly->sides)) {
                if (poly->sides < 3) poly->sides = 3;
                geometryChanged = true;
            }
            if (ImGui::InputDouble("Radio", &poly->radius, 0.1, 1.0, "%.4f")) {
                if (poly->radius < 0.0) poly->radius = 0.0;
                geometryChanged = true;
            }
            if (geometryChanged) {
                poly->points.clear();
                double angleStep = 2.0 * 3.14159265359 / poly->sides;
                for (int i = 0; i < poly->sides; ++i) {
                    double angle = i * angleStep;
                    poly->points.push_back({
                        poly->center.x + poly->radius * std::cos(angle),
                        poly->center.y + poly->radius * std::sin(angle)
                    });
                }
                engine.doc.isModified = true;
            }
        }
        // --- ELIPSE ---
        else if (auto* ellipse = dynamic_cast<Ellipse*>(firstEntity)) {
            ImGui::InputDouble("Centro X", &ellipse->center.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Centro Y", &ellipse->center.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Radio Mayor", &ellipse->majorRadius, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Radio Menor", &ellipse->minorRadius, 0.1, 1.0, "%.4f");
            double rotDeg = ellipse->rotationAngle * 180.0 / 3.14159265359;
            if (ImGui::InputDouble("Rotación (°)", &rotDeg, 1.0, 5.0, "%.2f")) {
                ellipse->rotationAngle = rotDeg * 3.14159265359 / 180.0;
                engine.doc.isModified = true;
            }
        }
        // --- DIMENSIÓN (COTA) ---
        else if (auto* dim = dynamic_cast<Dimension*>(firstEntity)) {
            ImGui::Text("P1 (Inicio):"); ImGui::SameLine(); ImGui::InputDouble("X", &dim->p1.x, 0.1, 1.0, "%.4f"); ImGui::SameLine(); ImGui::InputDouble("Y", &dim->p1.y, 0.1, 1.0, "%.4f");
            ImGui::Text("P2 (Fin):");   ImGui::SameLine(); ImGui::InputDouble("X", &dim->p2.x, 0.1, 1.0, "%.4f"); ImGui::SameLine(); ImGui::InputDouble("Y", &dim->p2.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Valor de Cota", &dim->value, 0.1, 1.0, "%.4f");
            engine.doc.isModified = true;
        }
        // --- BLOQUE ---
        else if (auto* block = dynamic_cast<BlockInsert*>(firstEntity)) {
            std::string blkName = block->definition ? block->definition->name : "Desconocido";
            ImGui::Text("Bloque: %s", blkName.c_str());
            ImGui::InputDouble("Inserción X", &block->insertPoint.x, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Inserción Y", &block->insertPoint.y, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Escala", &block->blockScale, 0.1, 1.0, "%.4f");
            if (block->blockScale < 0.001) block->blockScale = 0.001;
            ImGui::InputDouble("Rotación (°)", &block->blockRotation, 1.0, 5.0, "%.2f");
            engine.doc.isModified = true;
        }
        // --- IMAGEN ---
        else if (auto* img = dynamic_cast<ImageEntity*>(firstEntity)) {
            char pathBuf[512];
            strncpy(pathBuf, img->imagePath.c_str(), sizeof(pathBuf) - 1);
            pathBuf[sizeof(pathBuf) - 1] = '\0';
            if (ImGui::InputText("Ruta", pathBuf, sizeof(pathBuf))) {
                img->imagePath = pathBuf;
                engine.doc.isModified = true;
            }
            ImGui::InputDouble("Ancho", &img->width, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Alto", &img->height, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Rotación (°)", &img->rotation, 1.0, 5.0, "%.2f");
            engine.doc.isModified = true;
        }
        // --- TEXTO ---
        else if (auto* txt = dynamic_cast<Text*>(firstEntity)) {
            char contentBuf[512];
            strncpy(contentBuf, txt->content.c_str(), sizeof(contentBuf) - 1);
            contentBuf[sizeof(contentBuf) - 1] = '\0';
            if (ImGui::InputText("Contenido", contentBuf, sizeof(contentBuf))) {
                txt->content = contentBuf;
                engine.doc.isModified = true;
            }
            ImGui::InputDouble("Altura", &txt->height, 0.1, 1.0, "%.4f");
            double rotDeg = txt->rotation * 180.0 / 3.14159265359;
            if (ImGui::InputDouble("Rotación (°)", &rotDeg, 1.0, 5.0, "%.2f")) {
                txt->rotation = rotDeg * 3.14159265359 / 180.0;
                engine.doc.isModified = true;
            }
        }
        // --- HATCH ---
        else if (auto* hatch = dynamic_cast<Hatch*>(firstEntity)) {
            const char* patterns[] = { "Sólido", "Diagonal", "Cruz" };
            int currentPattern = static_cast<int>(hatch->pattern);
            if (ImGui::Combo("Patrón", &currentPattern, patterns, IM_ARRAYSIZE(patterns))) {
                hatch->pattern = static_cast<HatchPattern>(currentPattern);
                engine.doc.isModified = true;
            }
            ImGui::InputDouble("Escala", &hatch->patternScale, 0.1, 1.0, "%.4f");
            ImGui::InputDouble("Ángulo (°)", &hatch->angle, 1.0, 5.0, "%.2f");
            ImGui::InputDouble("Espaciado", &hatch->spacing, 0.1, 1.0, "%.4f");
            engine.doc.isModified = true;
        }
        // --- DESCONOCIDO ---
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Entidad no editable.");
        }
        
        ImGui::Unindent(); // Quitar sangría
    }

    ImGui::End();
}

void ImGuiManager::addRecentFile(const std::string& path) {
    // Evitar duplicados
    auto it = std::find(recentFiles_.begin(), recentFiles_.end(), path);
    if (it != recentFiles_.end()) {
        recentFiles_.erase(it);
    }
    // Añadir al principio
    recentFiles_.insert(recentFiles_.begin(), path);
    
    // Mantener solo los últimos 5
    if (recentFiles_.size() > 5) {
        recentFiles_.pop_back();
    }
}

void ImGuiManager::drawMainMenu(Engine& engine, bool& showAxes) {
    if (ImGui::BeginMainMenuBar()) {
        
        // --- ARCHIVO ---
        if (ImGui::BeginMenu("Archivo")) {
            if (ImGui::MenuItem("Nuevo", "Ctrl+N")) {
                if (onNew) onNew();
            }
            if (ImGui::MenuItem("Abrir...", "Ctrl+O")) {
                if (onOpen) onOpen();
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Guardar", "Ctrl+S")) {
                if (onSave) onSave();
            }
            if (ImGui::MenuItem("Guardar como...", "Ctrl+Shift+S")) {
                if (onSaveAs) onSaveAs();
            }
            
            ImGui::Separator();
            
            // Submenú de Archivos Recientes
            if (!recentFiles_.empty() && ImGui::BeginMenu("Archivos Recientes")) {
                for (const auto& file : recentFiles_) {
                    // Mostrar solo el nombre del archivo, no la ruta completa
                    std::string fileName = std::filesystem::path(file).filename().string();
                    if (ImGui::MenuItem(fileName.c_str())) {
                        engine.statusMessage = "Abriendo: " + file;
                        // Aquí llamaremos a la lógica de carga
                    }
                }
                ImGui::EndMenu();
            } else {
                ImGui::MenuItem("Archivos Recientes", nullptr, false, false); // Deshabilitado si está vacío
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Salir", "Alt+F4")) {
                if (onExit) onExit();
            }
            
            ImGui::EndMenu();
        }

        // --- EDITAR ---
        if (ImGui::BeginMenu("Editar")) {
            if (ImGui::MenuItem("Deshacer", "Ctrl+Z")) {
                engine.undo();
            }
            if (ImGui::MenuItem("Rehacer", "Ctrl+Y")) {
                engine.redo();
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Borrar", "Del")) {
                engine.deleteSelected();
            }
            ImGui::EndMenu();
        }

        // --- VER ---
        if (ImGui::BeginMenu("Ver")) {
            if (ImGui::MenuItem("Ejes", nullptr, &showAxes)) {
                // El checkbox se actualiza automáticamente por la referencia &showAxes
                engine.statusMessage = showAxes ? "Ejes activados" : "Ejes desactivados";
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Gestor de Capas", "Ctrl+Shift+L", &layerPanelVisible_)) {}
            if (ImGui::MenuItem("Panel de Propiedades", "Ctrl+L", &propertiesPanelVisible_)) {}
            ImGui::EndMenu();
        }

        // --- DIBUJAR ---
        if (ImGui::BeginMenu("Dibujar")) {
            // Usamos engine.processInput para activar los comandos de dibujo existentes
            if (ImGui::MenuItem("Línea", "L")) engine.processInput("L");
            if (ImGui::MenuItem("Polilínea", "PL")) engine.processInput("PL");
            if (ImGui::MenuItem("Triángulo", "TRI")) engine.processInput("TRI");
            ImGui::Separator();
            if (ImGui::MenuItem("Círculo", "C")) engine.processInput("C");
            if (ImGui::MenuItem("Arco", "A")) engine.processInput("A");
            if (ImGui::MenuItem("Elipse", "EL")) engine.processInput("EL");
            ImGui::Separator();
            if (ImGui::MenuItem("Polígono", "POL")) engine.processInput("POL");
            if (ImGui::MenuItem("Texto", "T")) engine.processInput("T");
            ImGui::EndMenu();
        }
        // =========================================================================
        // --- ACOTAR ---
        // =========================================================================
        if (ImGui::BeginMenu("Acotar")) {
            if (ImGui::MenuItem("Cota Alineada", "DIM")) engine.processInput("DIM");
            if (ImGui::MenuItem("Cota Horizontal", "DIMH")) engine.processInput("DIMH");
            if (ImGui::MenuItem("Cota Vertical", "DIMV")) engine.processInput("DIMV");
            ImGui::Separator();
            if (ImGui::MenuItem("Cota de Radio", "DIMR")) engine.processInput("DIMR");
            if (ImGui::MenuItem("Cota de Diámetro", "DIMDIA")) engine.processInput("DIMDIA");
            ImGui::EndMenu();
        }
        // =========================================================================
        // --- MODIFICAR ---
        // =========================================================================
        if (ImGui::BeginMenu("Modificar")) {
            if (ImGui::MenuItem("Mover", "M")) engine.processInput("M");
            if (ImGui::MenuItem("Copiar", "CO")) engine.processInput("CO");
            if (ImGui::MenuItem("Rotar", "RO")) engine.processInput("RO");
            if (ImGui::MenuItem("Escala", "SC")) engine.processInput("SC");
            if (ImGui::MenuItem("Simetría", "SI")) engine.processInput("SI");
            if (ImGui::MenuItem("Estirar", "S")) engine.processInput("S");
            ImGui::Separator();
            if (ImGui::MenuItem("Recortar", "TR")) engine.processInput("TR");
            if (ImGui::MenuItem("Alargar", "EX")) engine.processInput("EX");
            if (ImGui::MenuItem("Empalme", "F")) engine.processInput("F");
            if (ImGui::MenuItem("Chaflán", "CHA")) engine.processInput("CHA");
            if (ImGui::MenuItem("Desplazar (Offset)", "OF")) engine.processInput("OF");
            ImGui::Separator();
            if (ImGui::MenuItem("Matriz (Array)", "ARR")) engine.processInput("ARR");
            if (ImGui::MenuItem("Descomponer (Explode)", "X")) engine.processInput("X");
            ImGui::EndMenu();
        }
        // =========================================================================
        // --- BLOQUES ---
        // =========================================================================
        if (ImGui::BeginMenu("Bloques")) {
            if (ImGui::MenuItem("Crear Bloque", "BLOCK")) engine.processInput("BLOCK");
            if (ImGui::MenuItem("Insertar Bloque", "INSERT")) engine.processInput("INSERT");
            ImGui::EndMenu();
        }

        // =========================================================================
        // --- HERRAMIENTAS ---
        // =========================================================================
        if (ImGui::BeginMenu("Herramientas")) {
            if (ImGui::MenuItem("Medir Distancia", "DIST")) engine.processInput("DIST");
            ImGui::EndMenu();
        }
        // --- AYUDA ---
        if (ImGui::BeginMenu("Ayuda")) {
            if (ImGui::MenuItem("Acerca de CAD+")) {
                engine.statusMessage = "CAD+ v0.5 - Desarrollado con SFML e ImGui";
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void ImGuiManager::drawToolbar(Engine& engine) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoScrollWithMouse;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + ImGui::GetFrameHeight()));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, 45.0f));

    ImGui::Begin("Toolbar", nullptr, toolbarFlags);

    // Helper para botón con icono PNG
    auto toolIconButton = [&](const std::string& iconName, const char* tooltip, 
                              const char* command, ImVec4 color) {
        auto it = iconTextures_.find(iconName);
        if (it == iconTextures_.end()) {
            // Fallback: si no se cargó la textura, muestra un botón de texto
            if (ImGui::Button(command, ImVec2(60, 32))) {
                engine.processInput(command);
            }
            ImGui::SameLine();
            return;
        }

        sf::Texture& tex = it->second;
        ImVec2 iconSize(tex.getSize().x, tex.getSize().y);

        // Aplicar color al botón (teñirá el icono blanco)
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
            color.x + 0.1f > 1.0f ? 1.0f : color.x + 0.1f,
            color.y + 0.1f > 1.0f ? 1.0f : color.y + 0.1f,
            color.z + 0.1f > 1.0f ? 1.0f : color.z + 0.1f,
            color.w));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
            color.x - 0.1f < 0.0f ? 0.0f : color.x - 0.1f,
            color.y - 0.1f < 0.0f ? 0.0f : color.y - 0.1f,
            color.z - 0.1f < 0.0f ? 0.0f : color.z - 0.1f,
            color.w));

        // CLAVE: ImageButton con la textura de SFML
        if (ImGui::ImageButton((ImTextureID)(intptr_t)tex.getNativeHandle(), iconSize)) {
            engine.processInput(command);
        }

        ImGui::PopStyleColor(3);

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s (%s)", tooltip, command);
        }
        ImGui::SameLine();
    };

    // =========================================================================
    // SECCIÓN: DIBUJO (Azul) - CON ICONOS
    // =========================================================================
    ImVec4 colorDraw(0.2f, 0.4f, 0.8f, 1.0f);
    toolIconButton("line", "Dibujar Línea", "L", colorDraw);
    toolIconButton("circle", "Dibujar Círculo", "C", colorDraw);
    toolIconButton("arc", "Dibujar Arco", "A", colorDraw);
    toolIconButton("text", "Insertar Texto", "T", colorDraw);

    // Separador
    ImGui::Dummy(ImVec2(2, 28));
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // =========================================================================
    // SECCIÓN: MODIFICACIÓN (Naranja) - Aún con texto (hasta que tengas iconos)
    // =========================================================================
    ImVec4 colorMod(0.8f, 0.5f, 0.2f, 1.0f);
    auto toolButton = [&](const char* label, const char* tooltip, const char* command, ImVec4 color) {
        ImGui::PushStyleColor(ImGuiCol_Button, color);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
            color.x + 0.1f > 1.0f ? 1.0f : color.x + 0.1f,
            color.y + 0.1f > 1.0f ? 1.0f : color.y + 0.1f,
            color.z + 0.1f > 1.0f ? 1.0f : color.z + 0.1f, color.w));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
            color.x - 0.1f < 0.0f ? 0.0f : color.x - 0.1f,
            color.y - 0.1f < 0.0f ? 0.0f : color.y - 0.1f,
            color.z - 0.1f < 0.0f ? 0.0f : color.z - 0.1f, color.w));
        if (ImGui::Button(label, ImVec2(60, 28))) {
            engine.processInput(command);
        }
        ImGui::PopStyleColor(3);
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s (%s)", tooltip, command);
        ImGui::SameLine();
    };

    toolButton("Mover", "Mover Entidades", "M", colorMod);
    toolButton("Copiar", "Copiar Entidades", "CO", colorMod);
    toolButton("Recortar", "Recortar Entidades", "TR", colorMod);
    toolButton("Empalme", "Empalme (Fillet)", "F", colorMod);
    toolButton("Desplazar", "Desplazar (Offset)", "OF", colorMod);

    ImGui::Dummy(ImVec2(2, 28));
    ImGui::SameLine();
    ImGui::Separator();
    ImGui::SameLine();

    // =========================================================================
    // SECCIÓN: UTILIDADES (Verde)
    // =========================================================================
    ImVec4 colorUtil(0.2f, 0.6f, 0.4f, 1.0f);
    if (ImGui::Button("Capas", ImVec2(60, 28))) toggleLayerPanel();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Gestor de Capas");
    ImGui::SameLine();
    if (ImGui::Button("Props", ImVec2(60, 28))) togglePropertiesPanel();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Panel de Propiedades");

    ImGui::End();
    ImGui::PopStyleVar(2);
}

bool ImGuiManager::loadIconTexture(const std::string& name, const std::string& filePath) {
    sf::Texture tex;
    if (tex.loadFromFile(filePath)) {
        tex.setSmooth(true); // Suavizado para que no se pixelen al escalar
        iconTextures_[name] = tex;
        return true;
    }
    std::cerr << "[ERROR] No se pudo cargar el icono: " << filePath << std::endl;
    return false;
}

} // namespace cad