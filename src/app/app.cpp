#include "cad/app/app.hpp"

#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/ellipse.hpp"
#include "cad/core/geometry/entities/dimension.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/persistence/file_manager.hpp"

// >>> AÑADIR ESTOS INCLUDES ESPECÍFICOS QUE app.cpp NECESITA PARA LOS dynamic_cast <<<
#include "cad/commands/modify/stretch_command.hpp"
#include "cad/commands/block/block_create_command.hpp"
#include "cad/commands/block/block_insert_command.hpp"
#include "cad/commands/draw/dimension_command.hpp" // Por si también lo usas aquí

#include <filesystem>
#include <iostream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <windows.h>
#include <commdlg.h>

namespace cad {

    // --- Diálogos de archivo ---
    std::string showSaveFileDialog() {
        OPENFILENAMEA ofn;
        char fileName[MAX_PATH] = "";
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = "Archivos JSON\0*.json\0Todos los archivos\0*.*\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "json";
        if (GetSaveFileNameA(&ofn)) return std::string(fileName);
        return "";
    }

    std::string showOpenFileDialog() {
        OPENFILENAMEA ofn;
        char fileName[MAX_PATH] = "";
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFilter = "Archivos JSON\0*.json\0Todos los archivos\0*.*\0";
        ofn.lpstrFile = fileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        if (GetOpenFileNameA(&ofn)) return std::string(fileName);
        return "";
    }

    // --- Diálogo de Selección de Color de Windows ---
    sf::Color showColorDialog(sf::Color initialColor) {
        CHOOSECOLORA cc;
        static COLORREF customColors[16]; // Array para colores personalizados
        
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = NULL; // Puedes pasar el HWND de tu ventana si lo tienes a mano
        cc.lpCustColors = customColors;
        
        // Convertir sf::Color (RGB) a COLORREF (BGR) de Windows
        cc.rgbResult = RGB(initialColor.r, initialColor.g, initialColor.b);
        cc.Flags = CC_FULLOPEN | CC_RGBINIT;

        if (ChooseColorA(&cc)) {
            // El usuario eligió un color. Convertir de BGR (Windows) a RGB (SFML)
            COLORREF chosenColor = cc.rgbResult;
            unsigned char r = GetRValue(chosenColor);
            unsigned char g = GetGValue(chosenColor);
            unsigned char b = GetBValue(chosenColor);
            
            return sf::Color(r, g, b);
        }
        
        // El usuario canceló el diálogo
        return initialColor; 
    }

    // --- Constructor ---
    App::App() : ui_(font_) {
        std::cout << "Iniciando aplicación..." << std::endl;

        // 0. Detectar directorio del ejecutable
        std::filesystem::path exePath = std::filesystem::current_path();
        std::filesystem::path assetsPath;
        
        // Si estamos en build/, los assets están en ../assets/
        // Si estamos en raíz/, los assets están en assets/
        if (exePath.filename().string() == "build") {
            assetsPath = exePath.parent_path() / "assets";
        } else {
            assetsPath = exePath / "assets";
        }
        
        std::string fontPath = (assetsPath / "arial.ttf").string();
        std::cout << "[App] Buscando fuente en: " << fontPath << std::endl;

        // 1. Cargar configuración (con fallback)
        bool configLoaded = config_.loadFromFile("config.json");
        if (!configLoaded) {
            configLoaded = config_.loadFromFile("../config.json");
        }

        // 2. Configurar y crear la ventana
        sf::ContextSettings settings;
        settings.majorVersion = 2;
        settings.minorVersion = 1;
        settings.antialiasingLevel = 0;

        std::cout << "Creando ventana: " << config_.window.width << "x" << config_.window.height << std::endl;
        window_.create(
            sf::VideoMode(config_.window.width, config_.window.height),
            config_.window.title,
            sf::Style::Close | sf::Style::Resize,
            settings
        );
        window_.setPosition(sf::Vector2i(100, 100));
        window_.setVerticalSyncEnabled(false);
        
        float realCanvasHeight = config_.window.height - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT;
        view_.setCanvasHeight(realCanvasHeight);
        
        // 3. Cargar fuente (usando la ruta calculada)
        if (!font_.loadFromFile(fontPath)) {
            // Fallback a ruta absoluta de sistema (solo Linux)
            if (!font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
                std::cerr << "Error crítico: No se pudo cargar la fuente desde: " << fontPath << std::endl;
            }
        } else {
            std::cout << "[App] Fuente cargada correctamente." << std::endl;
        }

        // 4. Cargar cursores
        const sf::Uint8 transparentPixels[1024] = {0};
        bool transparentOk = transparentCursor_.loadFromPixels(transparentPixels, {16, 16}, {0, 0});
        bool arrowOk = arrowCursor_.loadFromSystem(sf::Cursor::Arrow);
        cursorsLoaded_ = transparentOk && arrowOk;

        // 5. Inicializar estado
        view_.setScale(1.0f);
        view_.setPan({50.0f, 50.0f});
        engine_.viewScale = 1.0;

        showAxes_ = true;
        showEntityListPanel_ = false;
        entityListScrollOffset_ = 0;
        entityListText_ = "";
    }

    void App::run() {
        std::cout << "Iniciando bucle principal..." << std::endl;
        while (window_.isOpen()) {
            handleEvents();
            render();
        }
        std::cout << "Programa terminado." << std::endl;
    }

    // --- Handle Events ---
    void App::handleEvents() {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.setMouseCursorVisible(true);
                window_.close();
            }

            // --- MOVIMIENTO DEL RATÓN ---
            if (event.type == sf::Event::MouseMoved) {
                int mx = event.mouseMove.x;
                int my = event.mouseMove.y;
                currentMouseScreenPos_ = {mx, my};

                if (isDraggingCommandScroll_) {
                    int deltaY = dragStartY_ - my;
                    int lineHeight = 20;
                    int linesMoved = deltaY / lineHeight;
                    int totalLines = commandHistory_.size() + 1;
                    int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
                    int maxOffset = std::max(0, totalLines - maxLines);
                    commandScrollOffset_ = std::max(0, std::min(maxOffset, dragStartOffset_ + linesMoved));
                }

                bool isInCanvas = (my >= static_cast<int>(MENU_HEIGHT + TOOLBAR_HEIGHT) &&
                                my < static_cast<int>(WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT));

                if (isInCanvas) {
                    window_.setMouseCursorVisible(false);
                    currentMouseWorldPos_ = view_.screenToWorld(
                        static_cast<float>(mx), static_cast<float>(my));
                    lastSnapResult_ = snapEngine_.findSnap(
                        engine_.doc.entities, currentMouseWorldPos_, view_.getScale());

                    if (engine_.currentMode == Mode::GRIP_EDIT && engine_.activeGripEntity) {
                        Point2D targetPos = lastSnapResult_.active ?
                            lastSnapResult_.point : currentMouseWorldPos_;
                        engine_.activeGripEntity->moveGrip(engine_.activeGripIndex, targetPos);
                    }
                } else {
                    window_.setMouseCursorVisible(true);
                    if (cursorsLoaded_) window_.setMouseCursor(arrowCursor_);
                }
            }

            // --- SCROLL ---
            if (event.type == sf::Event::MouseWheelScrolled) {
                int mx = event.mouseWheelScroll.x;
                int my = event.mouseWheelScroll.y;

                if (showEntityListPanel_) {
                    const int panelWidth = 350;
                    const int panelX = WINDOW_WIDTH - panelWidth;
                    const int panelY = MENU_HEIGHT + TOOLBAR_HEIGHT;
                    const int panelHeight = WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT;
                    if (mx >= panelX && mx < WINDOW_WIDTH && my >= panelY && my < panelY + panelHeight) {
                        if (event.mouseWheelScroll.delta > 0)
                            entityListScrollOffset_ = std::max(0, entityListScrollOffset_ - 1);
                        else
                            entityListScrollOffset_++;
                        return;
                    }
                }

                if (my >= WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT &&
                    my < WINDOW_HEIGHT - STATUS_HEIGHT) {
                    int lineHeight = 20;
                    int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
                    int totalLines = commandHistory_.size() + 1;
                    int maxOffset = std::max(0, totalLines - maxLines);
                    if (event.mouseWheelScroll.delta > 0)
                        commandScrollOffset_ = std::max(0, commandScrollOffset_ - 1);
                    else
                        commandScrollOffset_ = std::min(maxOffset, commandScrollOffset_ + 1);
                }

                if (my >= MENU_HEIGHT + TOOLBAR_HEIGHT &&
                    my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    float zoomFactor = (event.mouseWheelScroll.delta > 0) ? 1.1f : (1.0f / 1.1f);
                    view_.zoom(zoomFactor, {static_cast<float>(mx), static_cast<float>(my)});
                    engine_.viewScale = view_.getScale();
                }
            }

            // --- PAN (Clic Derecho) ---
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
                int my = event.mouseButton.y;
                if (my > MENU_HEIGHT + TOOLBAR_HEIGHT &&
                    my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    isPanning_ = true;
                    panStartMouse_ = {static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)};
                }
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Right) {
                isPanning_ = false;
            }

            // --- SELECCIÓN POR VENTANA (Clic Izquierdo liberado) ---
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                if (isSelectingByWindow_) {
                    selectionEndPoint_ = currentMouseWorldPos_;
                    
                    auto startScreen = view_.worldToScreen(selectionStartPoint_.x, selectionStartPoint_.y);
                    auto endScreen = view_.worldToScreen(selectionEndPoint_.x, selectionEndPoint_.y);
                    double dragDistance = std::hypot(endScreen.x - startScreen.x, endScreen.y - startScreen.y);

                    if (dragDistance > 5.0) {
                        bool shiftPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || 
                                            sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
                        if (!shiftPressed) {
                            engine_.selectedEntities.clear();
                        }
                        engine_.performWindowSelection(selectionStartPoint_, selectionEndPoint_, true);
                    } else {
                        double tolerance = 5.0 / view_.getScale();
                        Entity* clickedEntity = nullptr;
                        for (auto& entity : engine_.doc.entities) {
                            // >>> FILTRO DE SEGURIDAD: No seleccionar entidades de capas bloqueadas/congeladas <<<
                            const Layer* layer = engine_.doc.getLayer(entity->layerName);
                            if (!layer || !layer->visible || layer->frozen || layer->locked) continue;
                            
                            if (entity->isNear(selectionStartPoint_, tolerance)) {
                                clickedEntity = entity.get();
                                break;
                            }
                        }

                        if (clickedEntity) {
                            auto it = std::find(engine_.selectedEntities.begin(), 
                                                engine_.selectedEntities.end(), 
                                                clickedEntity);
                            if (it != engine_.selectedEntities.end()) {
                                engine_.selectedEntities.erase(it);
                            } else {
                                engine_.selectedEntities.push_back(clickedEntity);
                            }
                        } else {
                            engine_.selectedEntities.clear();
                        }
                    }
                    isSelectingByWindow_ = false;
                }
            }

            // --- PAN en movimiento ---
            if (event.type == sf::Event::MouseMoved && isPanning_) {
                float dx = static_cast<float>(event.mouseMove.x) - panStartMouse_.x;
                float dy = static_cast<float>(event.mouseMove.y) - panStartMouse_.y;
                view_.pan({dx, dy});
                panStartMouse_ = {static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y)};
            }

            // --- Actualizar punto final de selección por ventana ---
            if (event.type == sf::Event::MouseMoved && isSelectingByWindow_) {
                selectionEndPoint_ = currentMouseWorldPos_;
            }

            // --- CLIC IZQUIERDO ---
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;
                isDraggingCommandScroll_ = false;

                // Scroll de consola
                if (my >= WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT &&
                    my < WINDOW_HEIGHT - STATUS_HEIGHT) {
                    isDraggingCommandScroll_ = true;
                    dragStartY_ = my;
                    dragStartOffset_ = commandScrollOffset_;
                }
                // Barra de herramientas
                else if (my >= MENU_HEIGHT && my < MENU_HEIGHT + TOOLBAR_HEIGHT) {
                    handleToolbarClick(mx, my);
                }
                // Canvas
                else if (my >= MENU_HEIGHT + TOOLBAR_HEIGHT &&
                        my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    handleCanvasClick(currentMouseWorldPos_);
                }
            }

            // --- ESCRITURA EN LÍNEA DE COMANDOS ---
            if (event.type == sf::Event::TextEntered && isTyping_) {
                if (event.text.unicode == 13) {
                    processTextInput();
                }
                else if (event.text.unicode == 8) {
                    if (!inputBuffer_.empty()) {
                        inputBuffer_.pop_back();
                        autocompleteIndex_ = -1;
                        autocompleteBase_.clear();
                    }
                }
                else if (event.text.unicode >= 32 && event.text.unicode <= 126) {
                    inputBuffer_ += static_cast<char>(event.text.unicode);
                    autocompleteIndex_ = -1;
                    autocompleteBase_.clear();
                }
            }

            // --- TECLA ESCAPE ---
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                if (isSelectingByWindow_) {
                    isSelectingByWindow_ = false;
                }
                if (engine_.currentMode == Mode::GRIP_EDIT && engine_.gripBackup) {
                    engine_.activeGripEntity->copyFrom(*engine_.gripBackup);
                    engine_.gripBackup.reset();
                    engine_.statusMessage = "Edición de grip cancelada.";
                }
                engine_.cancelCommand();
                inputBuffer_.clear();
            }

            // --- TECLA SUPRIMIR ---
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Delete) {
                engine_.deleteSelected();
            }

            // --- TECLA ENTER EN EL CANVAS ---
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Return && !isTyping_) {
                if (engine_.activeCommand_ && !engine_.activeCommand_->isComplete()) {
                    engine_.processInput("");
                }
            }

            // --- TECLAS DE NAVEGACIÓN ---
            if (event.type == sf::Event::KeyPressed && isTyping_) {
                handleKeyboardNavigation(event);
            }

            // --- DESHACER / REHACER ---
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Z && event.key.control) engine_.undo();
                else if (event.key.code == sf::Keyboard::Y && event.key.control) engine_.redo();
            }
        }
    }

    // --- Render ---
    void App::render() {
        window_.clear(sf::Color(30, 30, 30));

        renderer_.render(window_, view_, engine_, currentMouseScreenPos_, currentMouseWorldPos_,
                        font_, lastSnapResult_.active, lastSnapResult_.point, showAxes_);

        // >>> NUEVO: Dibujar rectángulo de selección por ventana <<<
        if (isSelectingByWindow_) {
            renderer_.drawSelectionRect(window_, view_, selectionStartPoint_, selectionEndPoint_);
        }
    
        ui_.draw(window_, engine_, inputBuffer_, commandHistory_, commandScrollOffset_,
                isTyping_, showEntityListPanel_, entityListText_, entityListScrollOffset_,
                showAxes_, view_.getScale(), currentMouseWorldPos_);

        window_.display();
    }

    void App::updateEntityList() {
        entityListText_ = engine_.getEntityList();
        entityListScrollOffset_ = 0;
    }

    void App::handleToolbarClick(int mx, int my) {
        // Botón Línea
        if (mx >= 10 && mx <= 50) {
            engine_.processInput("L");
        }
        // Botón Círculo
        else if (mx >= 60 && mx <= 100) {
            engine_.processInput("C");
        }
        // Botón Ejes
        else if (mx >= 170 && mx <= 210) {
            showAxes_ = !showAxes_;
            engine_.statusMessage = showAxes_ ? "Ejes activados" : "Ejes desactivados";
        }
        // Botón Borrar
        else if (mx >= 220 && mx <= 260) {
            engine_.processInput("Z");
        }
        // Botón Ayuda
        else if (mx >= 280 && mx <= 320) {
            int maxChars = config_.window.width / CHARS_PER_PIXEL_FACTOR;
            std::string helpText = engine_.getHelpForTopic("", maxChars);
            commandHistory_.push_back("HELP");
            
            std::string line;
            for (char c : helpText) {
                if (c == '\n') {
                    if (!line.empty()) commandHistory_.push_back(line);
                    line.clear();
                } else {
                    line += c;
                }
            }
            if (!line.empty()) commandHistory_.push_back(line);
            engine_.statusMessage = "Ayuda mostrada";
        }
    }

    void App::handleCanvasClick(const Point2D& worldPoint) {
        double tolerance = 5.0 / view_.getScale();

        // 1. Modo GRIP_EDIT: terminar edición
        if (engine_.currentMode == Mode::GRIP_EDIT) {
            engine_.currentMode = Mode::IDLE;
            engine_.activeGripEntity = nullptr;
            engine_.gripBackup.reset();
            engine_.statusMessage = "Entidad modificada.";
            return;
        }

        // 2. Modo IDLE: buscar grips o iniciar selección por ventana
        if (engine_.currentMode == Mode::IDLE) {
            Entity* hitEntity = nullptr;
            int hitIndex = -1;
            
            for (Entity* e : engine_.selectedEntities) {
                // >>> FILTRO DE SEGURIDAD: No permitir grips en capas bloqueadas/congeladas <<<
                const Layer* layer = engine_.doc.getLayer(e->layerName);
                if (!layer || !layer->visible || layer->frozen || layer->locked) continue;

                auto grips = e->getGripPoints();
                for (int i = 0; i < static_cast<int>(grips.size()); ++i) {
                    double dist = std::hypot(worldPoint.x - grips[i].x, worldPoint.y - grips[i].y);
                    if (dist <= tolerance) {
                        hitEntity = e;
                        hitIndex = i;
                        break;
                    }
                }
                if (hitEntity) break;
            }

            if (hitEntity) {
                engine_.currentMode = Mode::GRIP_EDIT;
                engine_.activeGripEntity = hitEntity;
                engine_.activeGripIndex = hitIndex;
                engine_.gripBackup = hitEntity->clone();
                engine_.statusMessage = "Arrastrando grip...";
            } else {
                // Iniciar selección por ventana
                selectionStartPoint_ = worldPoint;
                selectionEndPoint_ = worldPoint;
                isSelectingByWindow_ = true;
                
                if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && 
                    !sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                    engine_.selectedEntities.clear();
                }
            }
            return;
        }

        // 3. Comandos activos con lógica especial
        // >>> STRETCH <<<
        if (engine_.currentMode == Mode::STRETCH && engine_.activeCommand_) {
            if (auto* stretchCmd = dynamic_cast<StretchCommand*>(engine_.activeCommand_.get())) {
                if (stretchCmd->getStep() == StretchCommand::Step::SelectingWindowP1) {
                    stretchCmd->setWindowP1(worldPoint);
                    engine_.statusMessage = "STRETCH | Esquina opuesta de la ventana:";
                    return;
                }
                else if (stretchCmd->getStep() == StretchCommand::Step::SelectingWindowP2) {
                    stretchCmd->setWindowP2(worldPoint);
                    double minX = std::min(stretchCmd->getWindowP1().x, stretchCmd->getWindowP2().x);
                    double maxX = std::max(stretchCmd->getWindowP1().x, stretchCmd->getWindowP2().x);
                    double minY = std::min(stretchCmd->getWindowP1().y, stretchCmd->getWindowP2().y);
                    double maxY = std::max(stretchCmd->getWindowP1().y, stretchCmd->getWindowP2().y);
                    
                    std::vector<Entity*> found;
                    for (auto& entity : engine_.doc.entities) {
                        auto grips = entity->getGripPoints();
                        for (const auto& grip : grips) {
                            if (grip.x >= minX && grip.x <= maxX && grip.y >= minY && grip.y <= maxY) {
                                found.push_back(entity.get());
                                break;
                            }
                        }
                    }
                    
                    if (!found.empty()) {
                        stretchCmd->setSelectedEntities(found);
                        stretchCmd->advanceToBasePoint();
                        engine_.statusMessage = "STRETCH | Punto base:";
                    } else {
                        engine_.statusMessage = "STRETCH | No hay entidades. Intenta de nuevo:";
                        stretchCmd->resetWindow();
                    }
                    return;
                }
            }
        }

        // >>> BLOCK CREATE <<<
        if (engine_.currentMode == Mode::BLOCK_CREATE && engine_.activeCommand_) {
            if (auto* blockCmd = dynamic_cast<BlockCreateCommand*>(engine_.activeCommand_.get())) {
                if (blockCmd->getStep() == BlockCreateCommand::Step::WaitingBasePoint) {
                    std::ostringstream ossCoord;
                    ossCoord << std::fixed << std::setprecision(6) << worldPoint.x << "," << worldPoint.y;
                    engine_.processInput(ossCoord.str());
                    inputBuffer_.clear();
                    return;
                }
                else if (blockCmd->getStep() == BlockCreateCommand::Step::SelectingEntities) {
                    double minDist = 10.0 / view_.getScale();
                    Entity* found = nullptr;
                    for (auto& entity : engine_.doc.entities) {
                        if (dynamic_cast<BlockInsert*>(entity.get())) continue;
                        if (entity->isNear(worldPoint, minDist)) {
                            found = entity.get();
                            break;
                        }
                    }
                    if (found) {
                        blockCmd->addSelectedEntity(found);
                        engine_.statusMessage = "BLOQUE | " +
                            std::to_string(blockCmd->getSelectedEntities().size()) +
                            " entidades. Enter para terminar:";
                    }
                    return;
                }
            }
        }

        // >>> BLOCK INSERT <<<
        if (engine_.currentMode == Mode::BLOCK_INSERT && engine_.activeCommand_) {
            if (auto* insertCmd = dynamic_cast<BlockInsertCommand*>(engine_.activeCommand_.get())) {
                if (insertCmd->getStep() == BlockInsertCommand::Step::WaitingInsertPoint) {
                    std::ostringstream ossCoord;
                    ossCoord << std::fixed << std::setprecision(6) << worldPoint.x << "," << worldPoint.y;
                    engine_.processInput(ossCoord.str());
                    inputBuffer_.clear();
                    return;
                }
            }
        }

        // 4. Envío genérico de coordenadas al comando activo
        Point2D targetPoint = lastSnapResult_.active ? lastSnapResult_.point : worldPoint;
        std::ostringstream ossCoord;
        ossCoord << std::fixed << std::setprecision(6);
        ossCoord << targetPoint.x << "," << targetPoint.y;
        engine_.processInput(ossCoord.str());
        inputBuffer_.clear();
    }

    void App::processTextInput() {
        if (!inputBuffer_.empty()) {
            if (commandHistory_.empty() || commandHistory_.back() != inputBuffer_) {
                commandHistory_.push_back(inputBuffer_);
            }
            if (commandHistory_.size() > 100) {
                commandHistory_.erase(commandHistory_.begin());
            }
        }
        historyIndex_ = commandHistory_.size();
        autocompleteIndex_ = -1;
        autocompleteBase_.clear();

        if (!inputBuffer_.empty()) {
            std::string upperInput(inputBuffer_);
            std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);

            // >>> GUARDAR <<<
            if (upperInput == "GUARDAR" || upperInput == "SAVE") {
                std::string path = showSaveFileDialog();
                if (!path.empty()) {
                    std::string ext = FileManager::getFileExtension(path);
                    if (ext != ".json" && ext != ".JSON") {
                        path += ".json";
                    }
                    if (FileManager::saveDocument(engine_.doc, path)) {
                        engine_.statusMessage = "Dibujo guardado en: " + path;
                    } else {
                        engine_.statusMessage = "Error al guardar el dibujo.";
                    }
                } else {
                    engine_.statusMessage = "Guardado cancelado.";
                }
            }
            // >>> CARGAR <<<
            else if (upperInput == "CARGAR" || upperInput == "LOAD") {
                std::string path = showOpenFileDialog();
                if (!path.empty()) {
                    engine_.selectedEntities.clear();
                    engine_.currentMode = Mode::IDLE;
                    engine_.cancelCommand();

                    if (FileManager::loadDocument(engine_.doc, path)) {
                        engine_.statusMessage = "Dibujo cargado desde: " + path;
                    } else {
                        engine_.statusMessage = "Error al cargar el dibujo.";
                    }
                } else {
                    engine_.statusMessage = "Carga cancelada.";
                }
            }
            // >>> AYUDA <<<
            else if (upperInput == "HELP" || upperInput == "AYUDA" || upperInput == "?" ||
                    upperInput.substr(0, 5) == "HELP " || upperInput.substr(0, 6) == "AYUDA ") {
                std::string topic = "";
                size_t spacePos = inputBuffer_.find(' ');
                if (spacePos != std::string::npos && spacePos + 1 < inputBuffer_.size()) {
                    topic = inputBuffer_.substr(spacePos + 1);
                }
                int maxChars = config_.window.width / CHARS_PER_PIXEL_FACTOR;
                std::string helpText = engine_.getHelpForTopic(topic, maxChars);
                
                std::string line;
                for (char c : helpText) {
                    if (c == '\n') {
                        if (!line.empty()) commandHistory_.push_back("  [AYUDA] " + line);
                        line.clear();
                    } else {
                        line += c;
                    }
                }
                if (!line.empty()) commandHistory_.push_back("  [AYUDA] " + line);
                engine_.statusMessage = "Ayuda mostrada";
            }
            // >>> CAMBIAR COLOR DE CAPA (Diálogo Windows) <<<
            else if (upperInput.find("CAPA COLOR ") == 0 || upperInput.find("LAYER COLOR ") == 0) {
                // 1. Extraer el nombre de la capa (quitando el prefijo)
                size_t prefixLen = (upperInput.find("CAPA COLOR ") == 0) ? 11 : 12;
                std::string layerName = upperInput.substr(prefixLen);
                
                // 2. Limpiar espacios en blanco sobrantes
                layerName.erase(0, layerName.find_first_not_of(' '));
                layerName.erase(layerName.find_last_not_of(' ') + 1);

                // 3. Validar y ejecutar
                if (!layerName.empty() && engine_.doc.layers.find(layerName) != engine_.doc.layers.end()) {
                    const Layer* currentLayer = engine_.doc.getLayer(layerName);
                    sf::Color startColor = currentLayer ? currentLayer->color : sf::Color::White;
                    
                    // Abrir el diálogo de color de Windows
                    sf::Color newColor = showColorDialog(startColor);
                    
                    // Actualizar el documento (¡Asegúrate de haber añadido setLayerColor en Document!)
                    engine_.doc.setLayerColor(layerName, newColor);
                    engine_.statusMessage = "Color de la capa '" + layerName + "' actualizado.";
                } else {
                    engine_.statusMessage = "Error: Capa no encontrada. Uso: CAPA COLOR <nombre>";
                }
            }
            // >>> COMANDO NORMAL <<<
            else {
                engine_.processInput(inputBuffer_);
            }
        }
        else {
            // Enter vacío: terminar comando activo
            if (engine_.activeCommand_ && !engine_.activeCommand_->isComplete()) {
                engine_.processInput("");
            }
        }
        inputBuffer_.clear();
        commandScrollOffset_ = 0;
    }

    void App::handleKeyboardNavigation(const sf::Event& event) {
        // Ctrl+L: Toggle panel de lista de entidades
        if (event.key.code == sf::Keyboard::L && event.key.control) {
            showEntityListPanel_ = !showEntityListPanel_;
            if (showEntityListPanel_) updateEntityList();
            engine_.statusMessage = showEntityListPanel_ ? "Panel activado" : "Panel desactivado";
        }
        // Flecha arriba: historial
        else if (event.key.code == sf::Keyboard::Up) {
            if (!commandHistory_.empty()) {
                if (historyIndex_ > 0) historyIndex_--;
                else historyIndex_ = commandHistory_.size() - 1;
                inputBuffer_ = commandHistory_[historyIndex_];
                autocompleteIndex_ = -1;
                autocompleteBase_.clear();
            }
        }
        // Flecha abajo: historial
        else if (event.key.code == sf::Keyboard::Down) {
            if (!commandHistory_.empty()) {
                if (historyIndex_ < commandHistory_.size() - 1) {
                    historyIndex_++;
                    inputBuffer_ = commandHistory_[historyIndex_];
                } else {
                    historyIndex_ = commandHistory_.size();
                    inputBuffer_.clear();
                }
                autocompleteIndex_ = -1;
                autocompleteBase_.clear();
            }
        }
        // Tab: autocompletado
        else if (event.key.code == sf::Keyboard::Tab) {
            if (!inputBuffer_.empty()) {
                if (autocompleteBase_.empty()) {
                    autocompleteBase_ = inputBuffer_;
                    std::transform(autocompleteBase_.begin(), autocompleteBase_.end(),
                                autocompleteBase_.begin(), ::toupper);
                    autocompleteIndex_ = 0;
                } else {
                    autocompleteIndex_++;
                }
                auto allCmds = engine_.getAllCommands();
                std::vector<std::string> matches;
                for (const auto& cmd : allCmds) {
                    if (cmd.find(autocompleteBase_) == 0) matches.push_back(cmd);
                }
                if (!matches.empty()) {
                    if (autocompleteIndex_ >= static_cast<int>(matches.size())) autocompleteIndex_ = 0;
                    inputBuffer_ = matches[autocompleteIndex_];
                } else {
                    autocompleteIndex_ = -1;
                    autocompleteBase_.clear();
                }
            }
        }
    }

    // Ejemplo dentro de tu App, cuando el usuario selecciona una capa y pulsa "Color":
    void App::changeLayerColor(const std::string& layerName) {
        // 1. Obtener el color actual de la capa
        const Layer* currentLayer = engine_.doc.getLayer(layerName);
        sf::Color startColor = currentLayer ? currentLayer->color : sf::Color::White;

        // 2. Abrir el diálogo de Windows
        sf::Color newColor = showColorDialog(startColor);

        // 3. Si el color cambió (o simplemente para confirmar), actualizar el Engine
        // (Puedes añadir un método setLayerColor en Engine o llamar directamente al doc)
        engine_.doc.setLayerColor(layerName, newColor);
        engine_.statusMessage = "Color de la capa '" + layerName + "' actualizado.";
    }

} // namespace cad