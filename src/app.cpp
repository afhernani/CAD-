#include "app.hpp"
#include <iostream>
#include <sstream>  // para std::ostringstream
#include <chrono>
#include <cmath>
#include <iomanip>
#include <windows.h>
#include <commdlg.h>

namespace cad {

    // Muestra el diálogo "Guardar como" de Windows
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
        
        if (GetSaveFileNameA(&ofn)) {
            return std::string(fileName);
        }
        return ""; // El usuario canceló
    }

    // Muestra el diálogo "Abrir" de Windows
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
        
        if (GetOpenFileNameA(&ofn)) {
            return std::string(fileName);
        }
        return ""; // El usuario canceló
    }


    App::App() {
        // Antes de window_.create()
        sf::ContextSettings settings;
        settings.majorVersion = 2;
        settings.minorVersion = 1;
        settings.antialiasingLevel = 0;

        std::cout << "Creando ventana..." << std::endl;
        window_.create(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "CAD+ v0.4", sf::Style::Close | sf::Style::Resize);
        // window_.setFramerateLimit(60);
        // SOLUCIÓN: Forzar posición explícita
        window_.setPosition(sf::Vector2i(100, 100));
        window_.setVerticalSyncEnabled(false);  // Desactivar vsync explícitamente
        // DIAGNÓSTICO:
        std::cout << "Ventana creada. Tamaño: " 
                << window_.getSize().x << "x" << window_.getSize().y << std::endl;
        std::cout << "isOpen: " << (window_.isOpen() ? "true" : "false") << std::endl;
        std::cout << "Posición: " 
                << window_.getPosition().x << "," << window_.getPosition().y << std::endl;
        // FIN DIAGNOSTICO

        if (!font_.loadFromFile("assets/arial.ttf") && !font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")) {
            std::cerr << "Error: No se pudo cargar la fuente.\n";
        }

        // Crear cursor transparente (16x16 píxeles, todo transparente)
        // Formato RGBA: 4 bytes por píxel = 1024 bytes
        const sf::Uint8 transparentPixels[1024] = {0}; // Todos ceros = totalmente transparente
        
        bool transparentOk = transparentCursor_.loadFromPixels(transparentPixels, {16, 16}, {0, 0});
        bool arrowOk = arrowCursor_.loadFromSystem(sf::Cursor::Arrow);
        cursorsLoaded_ = transparentOk && arrowOk;
        
        if (!cursorsLoaded_) {
            std::cerr << "Advertencia: No se pudieron cargar los cursores.\n";
        }
        
        viewScale_ = 1.0f;
        engine_.viewScale = 1.0;
        viewPanX_ = 50.0f;
        viewPanY_ = 50.0f;
        showAxes_ = true;  // Por defecto, mostrar ejes
        isSnapped_ = false;
    }

    void App::run() {
        std::cout << "Iniciando bucle principal..." << std::endl;
        while (window_.isOpen()) {
            handleEvents();
            render();
        }
        std::cout << "Programa terminado." << std::endl;
    }

    void App::handleEvents() {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.setMouseCursorVisible(true);  // Restaurar cursor al cerrar
                window_.close();
            }

            // --- CAMBIAR CURSOR SEGÚN LA ZONA ---
            if (event.type == sf::Event::MouseMoved) {
                int mx = event.mouseMove.x;
                int my = event.mouseMove.y;
                
                // Guardar posición de pantalla
                currentMouseScreenPos_ = {mx, my};

                // NUEVO: Lógica de arrastre del scroll de comandos
                if (isDraggingCommandScroll_) {
                    int deltaY = dragStartY_ - my;  // Invertido: arrastrar hacia arriba = scroll hacia abajo
                    int lineHeight = 20;
                    int linesMoved = deltaY / lineHeight;
                    
                    int totalLines = commandHistory_.size() + 1;
                    int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
                    int maxOffset = std::max(0, totalLines - maxLines);
                    
                    commandScrollOffset_ = std::max(0, std::min(maxOffset, dragStartOffset_ + linesMoved));
                }
                
                bool isInCanvas = (my >= static_cast<int>(MENU_HEIGHT + TOOLBAR_HEIGHT) && 
                                my < static_cast<int>(WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT));
                
                
                // Ocultar/mostrar cursor del sistema según la zona
                if (isInCanvas) {
                    window_.setMouseCursorVisible(false);  // Desactivar cursor completamente en el canvas
                } else {
                    window_.setMouseCursorVisible(true);   // Restaurar cursor fuera del canvas
                    if (cursorsLoaded_) {
                        window_.setMouseCursor(arrowCursor_);  // Asegurar flecha normal
                    }
                }
                
                if (isInCanvas) {
                    currentMouseWorldPos_ = screenToWorld(static_cast<float>(mx), static_cast<float>(my));
                    findSnap();
                    // Actualizar geometría si estamos arrastrando un grip
                    if (engine_.currentMode == Mode::GRIP_EDIT && engine_.activeGripEntity) {
                        // SOLUCIÓN: Conversión explícita a Point2D
                        Point2D targetPos = isSnapped_ ? snappedPoint_ : Point2D{currentMouseWorldPos_.x, currentMouseWorldPos_.y};
                        engine_.activeGripEntity->moveGrip(engine_.activeGripIndex, targetPos);
                    }
                }
            }

            // --- SCROLL EN VENTANA DE COMANDOS ---
            if (event.type == sf::Event::MouseWheelScrolled) {
                int my = event.mouseWheelScroll.y;
                // Si el ratón está sobre la ventana de comandos
                if (my >= WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT &&
                    my < WINDOW_HEIGHT - STATUS_HEIGHT) {
                    
                    int lineHeight = 20;
                    int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
                    int totalLines = commandHistory_.size() + 1;
                    int maxOffset = std::max(0, totalLines - maxLines);

                    if (event.mouseWheelScroll.delta > 0) {
                        commandScrollOffset_ = std::max(0, commandScrollOffset_ - 1);
                    } else {
                        // Limitar el offset al máximo para no pasar del inicio del historial
                        commandScrollOffset_ = std::min(maxOffset, commandScrollOffset_ + 1);
                    }
                }
            }

            // --- ZOOM CENTRADO EN EL CURSOR (solo en canvas) ---
            if (event.type == sf::Event::MouseWheelScrolled) {
                int mx = event.mouseWheelScroll.x;
                int my = event.mouseWheelScroll.y;
                
                // Comprobar si el ratón está sobre el área de dibujo (Canvas)
                if (my >= MENU_HEIGHT + TOOLBAR_HEIGHT && 
                    my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    
                    // 1. Guardar la posición del mundo bajo el ratón ANTES del zoom
                    sf::Vector2f tempPos = screenToWorld(static_cast<float>(mx), static_cast<float>(my));
                    Point2D worldPosBeforeZoom = {tempPos.x, tempPos.y};

                    // 2. Aplicar el Zoom
                    float zoomFactor = 1.1f;
                    if (event.mouseWheelScroll.delta > 0) {
                        viewScale_ *= zoomFactor; // Zoom In
                    } else if (event.mouseWheelScroll.delta < 0) {
                        viewScale_ /= zoomFactor; // Zoom Out
                    }
                    
                    // Limitar el zoom para que no se rompa
                    if (viewScale_ < 0.01f) viewScale_ = 0.01f;
                    if (viewScale_ > 100.0f) viewScale_ = 100.0f;
                    
                    // Sincronizar con el engine (para TRIM/EXTEND)
                    engine_.viewScale = viewScale_;

                    // 3. Calcular dónde estaría ese punto del mundo AHORA con el nuevo zoom
                    sf::Vector2f screenPosAfterZoom = worldToScreen(worldPosBeforeZoom.x, worldPosBeforeZoom.y);

                    // 4. Ajustar la cámara (Pan) para compensar el desplazamiento
                    // La diferencia entre donde está el ratón y donde quedó el punto es lo que debemos mover
                    float dx = static_cast<float>(mx) - screenPosAfterZoom.x;
                    float dy = static_cast<float>(my) - screenPosAfterZoom.y;

                    viewPanX_ += dx / viewScale_;
                    viewPanY_ -= dy / viewScale_; // Restamos porque en SFML Y crece hacia abajo, pero en CAD hacia arriba
                }
            }

            // --- PAN (Clic Derecho) ---
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
                int my = event.mouseButton.y;
                if (my > MENU_HEIGHT + TOOLBAR_HEIGHT && 
                    my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    isPanning_ = true;
                    panStartMouse_ = {static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)};
                    panStartViewX_ = viewPanX_;
                    panStartViewY_ = viewPanY_;
                }
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Right) {
                isPanning_ = false;
            }
            if (event.type == sf::Event::MouseMoved && isPanning_) {
                float dx = static_cast<float>(event.mouseMove.x) - panStartMouse_.x;
                float dy = static_cast<float>(event.mouseMove.y) - panStartMouse_.y;
                viewPanX_ = panStartViewX_ + (dx / viewScale_);
                viewPanY_ = panStartViewY_ - (dy / viewScale_); 
            }

            // --- CLIC IZQUIERDO ---
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                isDraggingCommandScroll_=false;

                // NUEVO: Detectar clic en la ventana de comandos para arrastrar el scroll
                if (my >= WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT &&
                    my < WINDOW_HEIGHT - STATUS_HEIGHT) {
                    isDraggingCommandScroll_ = true;
                    dragStartY_ = my;
                    dragStartOffset_ = commandScrollOffset_;
                }

                // A) Clic en la Barra de Herramientas
                if (my >= MENU_HEIGHT && my < MENU_HEIGHT + TOOLBAR_HEIGHT) {
                    if (mx >= 10 && mx <= 50) {
                        engine_.processInput("L");
                    } else if (mx >= 60 && mx <= 100) {
                        engine_.processInput("C");
                    } else if (mx >= 170 && mx <= 210) {
                        showAxes_ = !showAxes_;
                        engine_.statusMessage = showAxes_ ? "Ejes activados" : "Ejes desactivados";
                    } else if (mx >= 220 && mx <= 260) {
                        engine_.processInput("Z");
                    }
                    // En la sección de clic en barra de menú
                    else if (mx >= 280 && mx <= 320) {
                        // Clic en botón de ayuda
                        std::string helpText = engine_.getHelpForTopic("");
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
                // B) Clic en el Canvas
                else if (my >= MENU_HEIGHT + TOOLBAR_HEIGHT && my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    Point2D worldPoint = {currentMouseWorldPos_.x, currentMouseWorldPos_.y};
                    double tolerance = 5.0 / viewScale_;

                    if (engine_.currentMode == Mode::GRIP_EDIT) {
                        // Confirmar edición de grip
                        engine_.currentMode = Mode::IDLE;
                        engine_.activeGripEntity = nullptr;
                        engine_.gripBackup.reset();
                        engine_.statusMessage = "Entidad modificada.";
                    } 
                    else if (engine_.currentMode == Mode::IDLE) {
                        // 1. Buscar si hemos clicado un grip de una entidad seleccionada
                        Entity* hitEntity = nullptr;
                        int hitIndex = -1;
                        
                        for (Entity* e : engine_.selectedEntities) {
                            auto grips = e->getGripPoints();
                            for (int i = 0; i < grips.size(); ++i) {
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
                            // Iniciar modo Grip Edit
                            engine_.currentMode = Mode::GRIP_EDIT;
                            engine_.activeGripEntity = hitEntity;
                            engine_.activeGripIndex = hitIndex;
                            engine_.gripBackup = hitEntity->clone(); // Guardar copia para ESC
                            engine_.statusMessage = "Arrastrando grip... (Clic para confirmar, ESC para cancelar)";
                        } else {
                            // 2. Si no es un grip, selección normal de entidad
                            engine_.selectEntity(worldPoint, tolerance);
                        }
                    }
                    // ... (el resto de modos como DRAW_LINE etc. los dejas igual que los tenías) ...
                    else if (engine_.currentMode != Mode::IDLE) {
                        Point2D targetPoint = isSnapped_ ? snappedPoint_ : worldPoint;
                        std::ostringstream ossCoord;
                        ossCoord << std::fixed << std::setprecision(6);
                        ossCoord << targetPoint.x << "," << targetPoint.y;
                        engine_.processInput(ossCoord.str());
                        inputBuffer_.clear();
                    }
                }
            }

            // --- ESCRITURA EN LÍNEA DE COMANDOS ---
            if (event.type == sf::Event::TextEntered && isTyping_) {
                if (event.text.unicode == 13) { // Enter
                    if (!inputBuffer_.empty()) {
                        // 1. Guardar en el historial (solo lo que escribe el usuario, limpio)
                        if (commandHistory_.empty() || commandHistory_.back() != inputBuffer_) {
                            commandHistory_.push_back(inputBuffer_);
                        }
                        
                        // Limitar historial a 100 elementos (manteniendo los más recientes)
                        if (commandHistory_.size() > 100) {
                            commandHistory_.erase(commandHistory_.begin());
                        }
                        
                        // 2. Resetear índices de navegación y autocompletado
                        historyIndex_ = commandHistory_.size();
                        autocompleteIndex_ = -1;
                        autocompleteBase_.clear();
                        
                        // 3. Detectar comandos especiales o enviar al engine
                        std::string upperInput(inputBuffer_);
                        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
                        
                        // >>> NUEVO: Interceptamos GUARDAR y CARGAR antes de que lleguen al engine <<<
                        if (upperInput == "GUARDAR" || upperInput == "SAVE") {
                            std::string path = showSaveFileDialog();
                            if (!path.empty()) {
                                engine_.doc.saveToFile(path);
                                engine_.statusMessage = "Dibujo guardado en: " + path;
                            } else {
                                engine_.statusMessage = "Guardado cancelado.";
                            }
                        }
                        else if (upperInput == "CARGAR" || upperInput == "LOAD") {
                            std::string path = showOpenFileDialog();
                            if (!path.empty()) {
                                engine_.doc.loadFromFile(path);
                                engine_.selectedEntities.clear();
                                engine_.currentMode = Mode::IDLE; // Reseteamos cualquier modo activo
                                engine_.statusMessage = "Dibujo cargado desde: " + path;
                            } else {
                                engine_.statusMessage = "Carga cancelada.";
                            }
                        }
                        else if (upperInput == "HELP" || upperInput == "AYUDA" || upperInput == "?" ||
                                upperInput.substr(0, 5) == "HELP " || upperInput.substr(0, 6) == "AYUDA ") {
                            
                            std::string topic = "";
                            size_t spacePos = inputBuffer_.find(' ');
                            if (spacePos != std::string::npos && spacePos + 1 < inputBuffer_.size()) {
                                topic = inputBuffer_.substr(spacePos + 1);
                            }
                            
                            // El engine devuelve el texto, nosotros lo imprimimos en la consola visual
                            std::string helpText = engine_.getHelpForTopic(topic);
                            std::string line;
                            for (char c : helpText) {
                                if (c == '\n') {
                                    if (!line.empty()) {
                                        commandHistory_.push_back("  [AYUDA] " + line);
                                    }
                                    line.clear();
                                } else {
                                    line += c;
                                }
                            }
                            if (!line.empty()) {
                                commandHistory_.push_back("  [AYUDA] " + line);
                            }
                            engine_.statusMessage = "Ayuda mostrada";
                        }
                        else {
                            // Comando normal: enviar al engine (Línea, Círculo, Mover, etc.)
                            engine_.processInput(inputBuffer_);
                        }
                    }
                    
                    // Limpiar buffer y resetear scroll
                    inputBuffer_.clear();
                    commandScrollOffset_ = 0;
                } 
                else if (event.text.unicode == 8) { // Backspace
                    if (!inputBuffer_.empty()) {
                        inputBuffer_.pop_back();
                        autocompleteIndex_ = -1;
                        autocompleteBase_.clear();
                    }
                } 
                else if (event.text.unicode >= 32 && event.text.unicode <= 126) {
                    // SOLO caracteres ASCII imprimibles
                    inputBuffer_ += static_cast<char>(event.text.unicode);
                    autocompleteIndex_ = -1;
                    autocompleteBase_.clear();
                }
            }

            // --- TECLA ESCAPE: Cancelar comando ---
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                if (engine_.currentMode == Mode::GRIP_EDIT && engine_.gripBackup) {
                    // Restaurar la entidad original
                    engine_.activeGripEntity->copyFrom(*engine_.gripBackup);
                    engine_.gripBackup.reset();
                    engine_.statusMessage = "Edición de grip cancelada.";
                }
                engine_.cancelCommand();
                inputBuffer_.clear();
            }
            // --- TECLA SUPRIMIR: Borrar seleccionados ---
            if (event.type == sf::Event::KeyPressed && 
                event.key.code == sf::Keyboard::Delete) {
                engine_.deleteSelected();
            }

            // --- TECLAS DE NAVEGACIÓN Y AUTOCOMPLETADO (KeyPressed) ---
            if (event.type == sf::Event::KeyPressed && isTyping_) {
                
                // Flecha ARRIBA: Historial anterior
                if (event.key.code == sf::Keyboard::Up) {
                    if (!commandHistory_.empty()) {
                        if (historyIndex_ > 0) {
                            historyIndex_--;
                        } else {
                            historyIndex_ = commandHistory_.size() - 1;
                        }
                        inputBuffer_ = commandHistory_[historyIndex_];
                        autocompleteIndex_ = -1;
                        autocompleteBase_.clear();
                    }
                }
                // Flecha ABAJO: Historial siguiente
                else if (event.key.code == sf::Keyboard::Down) {
                    if (!commandHistory_.empty()) {
                        if (historyIndex_ < commandHistory_.size() - 1) {
                            historyIndex_++;
                            inputBuffer_ = commandHistory_[historyIndex_];
                        } else {
                            historyIndex_ = commandHistory_.size();
                            inputBuffer_.clear(); // Volver a la línea vacía
                        }
                        autocompleteIndex_ = -1;
                        autocompleteBase_.clear();
                    }
                }
                // TAB: Autocompletar
                else if (event.key.code == sf::Keyboard::Tab) {
                    if (!inputBuffer_.empty()) {
                        // Si es la primera vez que se pulsa Tab, guardamos la base escrita por el usuario
                        if (autocompleteBase_.empty()) {
                            autocompleteBase_ = inputBuffer_;
                            std::transform(autocompleteBase_.begin(), autocompleteBase_.end(), autocompleteBase_.begin(), ::toupper);
                            autocompleteIndex_ = 0;
                        } else {
                            // Si ya teníamos una base, simplemente avanzamos el índice
                            autocompleteIndex_++;
                        }

                        auto allCmds = engine_.getAllCommands();
                        std::vector<std::string> matches;
                        
                        // Buscar coincidencias que empiecen por la base ORIGINAL del usuario
                        for (const auto& cmd : allCmds) {
                            if (cmd.find(autocompleteBase_) == 0) {
                                matches.push_back(cmd);
                            }
                        }

                        if (!matches.empty()) {
                            // Ciclar entre las coincidencias
                            if (autocompleteIndex_ >= matches.size()) {
                                autocompleteIndex_ = 0;
                            }
                            inputBuffer_ = matches[autocompleteIndex_];
                        } else {
                            autocompleteIndex_ = -1;
                            autocompleteBase_.clear();
                        }
                    }
                }
            }

            // --- ATAJOS DE TECLADO: DESHACER / REHACER ---
            if (event.type == sf::Event::KeyPressed) {
                // Ctrl + Z (Deshacer)
                if (event.key.code == sf::Keyboard::Z && event.key.control) {
                    engine_.undo();
                }
                // Ctrl + Y (Rehacer)
                else if (event.key.code == sf::Keyboard::Y && event.key.control) {
                    engine_.redo();
                }
            }
        }
    }

    sf::Vector2f App::worldToScreen(double wx, double wy) {
        float sx = (wx + viewPanX_) * viewScale_;
        float sy = (CANVAS_HEIGHT - (wy + viewPanY_) * viewScale_);
        return {sx, sy};
    }

    sf::Vector2f App::screenToWorld(float sx, float sy) {
        float wx = (sx / viewScale_) - viewPanX_;
        float wy = ((CANVAS_HEIGHT - sy) / viewScale_) - viewPanY_;
        return {wx, wy};
    }

    void App::render() {
        window_.clear(sf::Color(30, 30, 30));

        // Dibujar el área gráfica (canvas)
        drawGrid();
        if (showAxes_) {
            drawAxes();
        }
        drawEntities();
        drawDimensionTexts();
        drawCrosshair();

        // Dibujar la interfaz
        drawUI();
        drawToolbar();

        window_.display();
    }

    void App::drawGrid() {
        if (!engine_.gridEnabled) return;

        // 1. Calcular el espaciado dinámico según el zoom (SIN modificar gridBaseSize_)
        double currentGridSize = gridBaseSize_;
        double pixelSpacing = currentGridSize * viewScale_;
        
        // Ajustar el tamaño local para que la separación visual sea siempre cómoda (entre 30px y 150px)
        while (pixelSpacing < 30.0) { 
            currentGridSize *= 10.0; 
            pixelSpacing = currentGridSize * viewScale_; 
        }
        while (pixelSpacing > 150.0) { 
            currentGridSize /= 10.0; 
            pixelSpacing = currentGridSize * viewScale_; 
        }

        // 2. Calcular los límites visibles en coordenadas del mundo
        double left   = screenToWorld(0, 0).x;
        double right  = screenToWorld(WINDOW_WIDTH, 0).x;
        double top    = screenToWorld(0, 0).y;
        double bottom = screenToWorld(0, WINDOW_HEIGHT).y;

        if (left > right) std::swap(left, right);
        if (top > bottom) std::swap(top, bottom);

        // 3. Calcular el primer punto de la cuadrícula (alineado al origen 0,0)
        // Usamos currentGridSize, no gridBaseSize_
        double startX = std::floor(left / currentGridSize) * currentGridSize;
        double startY = std::floor(top / currentGridSize) * currentGridSize;

        // 4. Dibujar usando VertexArray
        sf::Color gridColor(50, 50, 50); 
        sf::VertexArray lines(sf::Lines);

        // Líneas verticales
        for (double x = startX; x <= right; x += currentGridSize) {
            sf::Vector2f topPos = worldToScreen(x, top);
            sf::Vector2f botPos = worldToScreen(x, bottom);
            lines.append(sf::Vertex(topPos, gridColor));
            lines.append(sf::Vertex(botPos, gridColor));
        }

        // Líneas horizontales
        for (double y = startY; y <= bottom; y += currentGridSize) {
            sf::Vector2f leftPos = worldToScreen(left, y);
            sf::Vector2f rightPos = worldToScreen(right, y);
            lines.append(sf::Vertex(leftPos, gridColor));
            lines.append(sf::Vertex(rightPos, gridColor));
        }

        window_.draw(lines);
    }

    void App::drawAxes() {
        // Eje X (Rojo) - Solo 50 unidades desde el origen
        sf::Vertex xAxis[] = {
            sf::Vertex(worldToScreen(0, 0), sf::Color::Red),
            sf::Vertex(worldToScreen(50, 0), sf::Color::Red)
        };
        window_.draw(xAxis, 2, sf::Lines);

        // Eje Y (Verde) - Solo 50 unidades desde el origen
        sf::Vertex yAxis[] = {
            sf::Vertex(worldToScreen(0, 0), sf::Color::Green),
            sf::Vertex(worldToScreen(0, 50), sf::Color::Green)
        };
        window_.draw(yAxis, 2, sf::Lines);
        
        // Origen (punto blanco)
        sf::CircleShape origin(3.f);
        origin.setFillColor(sf::Color::White);
        origin.setOrigin(3.f, 3.f);
        origin.setPosition(worldToScreen(0,0));
        window_.draw(origin);
    }

    void App::drawEntities() {
        auto w2s = [this](double x, double y) {
            return worldToScreen(x, y);
        };

        // Iterar sobre todas las entidades de forma polimórfica
        for (const auto& entity : engine_.doc.entities) {
            const Layer* layer = engine_.doc.getLayer(entity->layerName);
            if (!layer || !layer->visible || layer->frozen) continue;

            // Comprobar si está seleccionada
            bool isSelected = std::find(engine_.selectedEntities.begin(), 
                                        engine_.selectedEntities.end(), 
                                        entity.get()) != engine_.selectedEntities.end();

            // Si está seleccionada, usamos Color Cyan, si no, el color de la capa
            // sf::Color drawColor = isSelected ? sf::Color::Cyan : layer->color;
            // Usar siempre el color de la capa (los grips indicarán la selección)
            sf::Color drawColor = layer->color;
            
            // Llamada polimórfica: cada entidad sabe cómo dibujarse
            entity->draw(window_, w2s, drawColor, viewScale_);
        }
    
        // >>> DIBUJAR GRIPS PARA ENTIDADES SELECCIONADAS <<<
        drawGrips();

        // --- FEEDBACK VISUAL PARA COMANDO DIST ---
        if (engine_.currentMode == Mode::MEASURE_DIST && 
            engine_.statusMessage.find("segundo") != std::string::npos) {
            
            // Dibujar línea temporal desde tempPoint1 hasta el ratón
            sf::Vertex line[] = {
                sf::Vertex(worldToScreen(engine_.tempPoint1.x, engine_.tempPoint1.y), sf::Color::Yellow),
                sf::Vertex(worldToScreen(currentMouseWorldPos_.x, currentMouseWorldPos_.y), sf::Color::Yellow)
            };
            window_.draw(line, 2, sf::Lines);

            // Dibujar texto con la distancia en tiempo real
            double dx = currentMouseWorldPos_.x - engine_.tempPoint1.x;
            double dy = currentMouseWorldPos_.y - engine_.tempPoint1.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2);
            oss << "Dist: " << dist;
            sf::Text txt;
            txt.setFont(font_);
            txt.setString(toSfString(oss.str()));
            txt.setCharacterSize(14);

            txt.setFillColor(sf::Color::Yellow);
            // Posicionar el texto cerca del ratón (en pantalla)
            sf::Vector2f mouseScreen = worldToScreen(currentMouseWorldPos_.x, currentMouseWorldPos_.y);
            txt.setPosition(mouseScreen.x + 10.f, mouseScreen.y - 20.f);
            window_.draw(txt);
        }
        
        // >>> AÑADIR ESTO: Feedback visual de dibujo <<<
        drawDrawingFeedback();

        // Marcador de Object Snap con iconos diferentes
        if (isSnapped_) {
            sf::Vector2f screenPos = worldToScreen(snappedPoint_.x, snappedPoint_.y);
            const float size = 8.0f;
            
            if (currentSnapType_ == SnapType::INTERSECTION) {
                // Cruz para intersección
                sf::Vertex line1[] = {
                    sf::Vertex(sf::Vector2f(screenPos.x - size, screenPos.y), sf::Color::Yellow),
                    sf::Vertex(sf::Vector2f(screenPos.x + size, screenPos.y), sf::Color::Yellow)
                };
                sf::Vertex line2[] = {
                    sf::Vertex(sf::Vector2f(screenPos.x, screenPos.y - size), sf::Color::Yellow),
                    sf::Vertex(sf::Vector2f(screenPos.x, screenPos.y + size), sf::Color::Yellow)
                };
                window_.draw(line1, 2, sf::Lines);
                window_.draw(line2, 2, sf::Lines);
            } 
            else {
                // Cuadrado para extremos, centros y puntos medios (podrías diferenciarlos más si quieres)
                sf::RectangleShape marker(sf::Vector2f(size, size));
                marker.setFillColor(sf::Color::Transparent);
                marker.setOutlineColor(sf::Color::Yellow);
                marker.setOutlineThickness(1.5f);
                marker.setOrigin(size / 2.f, size / 2.f);
                marker.setPosition(screenPos);
                window_.draw(marker);
            }
        }
    }

    void App::drawUI() {
        // Barra de Menú (Arriba)
        sf::RectangleShape menu(sf::Vector2f(WINDOW_WIDTH, MENU_HEIGHT));
        menu.setFillColor(sf::Color(45, 45, 48));
        menu.setPosition(0, 0);
        window_.draw(menu);
        
        // Después:
        sf::Text menuTxt;
        menuTxt.setFont(font_);
        menuTxt.setString(toSfString("Archivo  Editar  Ver  Dibujar  Modificar  Ayuda"));
        menuTxt.setCharacterSize(14);

        menuTxt.setFillColor(sf::Color(220, 220, 220));
        menuTxt.setPosition(10, 8);
        window_.draw(menuTxt);

        // La ventana de comandos se dibuja en drawCommandWindow()
        drawCommandWindow();

        // Barra de Estado (Muy abajo)
        sf::RectangleShape statusBg(sf::Vector2f(WINDOW_WIDTH, STATUS_HEIGHT));
        statusBg.setFillColor(sf::Color(0, 122, 204));
        statusBg.setPosition(0, WINDOW_HEIGHT - STATUS_HEIGHT);
        window_.draw(statusBg);

        // Texto de estado con coordenadas en tiempo real
        std::ostringstream oss;
        // oss << std::fixed << std::setprecision(2);
        oss << engine_.statusMessage 
            << " | X: " << currentMouseWorldPos_.x 
            << ", Y: " << currentMouseWorldPos_.y 
            << " | Zoom: " << viewScale_ << "x";
        sf::Text statusTxt;
        statusTxt.setFont(font_);
        statusTxt.setString(toSfString(oss.str()));
        statusTxt.setCharacterSize(12);

        statusTxt.setFillColor(sf::Color::White);
        statusTxt.setPosition(10, WINDOW_HEIGHT - STATUS_HEIGHT + 6);
        window_.draw(statusTxt);
    }

    void App::drawToolbar() {
        // Barra de Herramientas (Debajo del menú)
        sf::RectangleShape toolbar(sf::Vector2f(WINDOW_WIDTH, TOOLBAR_HEIGHT));
        toolbar.setFillColor(sf::Color(45, 45, 48));
        toolbar.setPosition(0, MENU_HEIGHT);
        window_.draw(toolbar);

        // Línea separadora inferior
        sf::RectangleShape separator(sf::Vector2f(WINDOW_WIDTH, 1));
        separator.setFillColor(sf::Color(80, 80, 80));
        separator.setPosition(0, MENU_HEIGHT + TOOLBAR_HEIGHT - 1);
        window_.draw(separator);

        // --- Botón Línea (L) ---
        sf::RectangleShape btnLine(sf::Vector2f(40, 40));
        btnLine.setFillColor(sf::Color(70, 70, 75));
        btnLine.setOutlineColor(sf::Color(100, 100, 100));
        btnLine.setOutlineThickness(1.f);
        btnLine.setPosition(10, MENU_HEIGHT + 10);
        window_.draw(btnLine);

        // Icono de línea (diagonal blanca)
        sf::Vertex lineIcon[] = {
            sf::Vertex(sf::Vector2f(18, MENU_HEIGHT + 38), sf::Color::White),
            sf::Vertex(sf::Vector2f(42, MENU_HEIGHT + 14), sf::Color::White)
        };
        window_.draw(lineIcon, 2, sf::Lines);

        // --- Botón Círculo (C) ---
        sf::RectangleShape btnCircle(sf::Vector2f(40, 40));
        btnCircle.setFillColor(sf::Color(70, 70, 75));
        btnCircle.setOutlineColor(sf::Color(100, 100, 100));
        btnCircle.setOutlineThickness(1.f);
        btnCircle.setPosition(60, MENU_HEIGHT + 10);
        window_.draw(btnCircle);

        // Icono de círculo
        sf::CircleShape circleIcon(12.f);
        circleIcon.setFillColor(sf::Color::Transparent);
        circleIcon.setOutlineColor(sf::Color::White);
        circleIcon.setOutlineThickness(2.f);
        circleIcon.setPosition(72, MENU_HEIGHT + 22);
        window_.draw(circleIcon);

        // --- Botón Arco (A) ---
        sf::RectangleShape btnArc(sf::Vector2f(40, 40));
        btnArc.setFillColor(sf::Color(70, 70, 75));
        btnArc.setOutlineColor(sf::Color(100, 100, 100));
        btnArc.setOutlineThickness(1.f);
        btnArc.setPosition(110, MENU_HEIGHT + 10);
        window_.draw(btnArc);

        // Icono de arco (semicírculo)
        sf::CircleShape arcIcon(12.f);
        arcIcon.setFillColor(sf::Color::Transparent);
        arcIcon.setOutlineColor(sf::Color::White);
        arcIcon.setOutlineThickness(2.f);
        // Dibujamos solo la mitad superior usando un círculo con recorte visual
        // Para simplificar, usamos un círculo completo pero más pequeño
        arcIcon.setRadius(10.f);
        arcIcon.setPosition(122, MENU_HEIGHT + 24);
        window_.draw(arcIcon);
        // Línea base del arco
        sf::Vertex arcBase[] = {
            sf::Vertex(sf::Vector2f(122, MENU_HEIGHT + 34), sf::Color::White),
            sf::Vertex(sf::Vector2f(142, MENU_HEIGHT + 34), sf::Color::White)
        };
        window_.draw(arcBase, 2, sf::Lines);

        // --- Separador vertical ---
        sf::RectangleShape sep1(sf::Vector2f(1, 40));
        sep1.setFillColor(sf::Color(80, 80, 80));
        sep1.setPosition(160, MENU_HEIGHT + 10);
        window_.draw(sep1);

        // --- Botón Ejes ON/OFF ---
        sf::RectangleShape btnAxes(sf::Vector2f(40, 40));
        btnAxes.setFillColor(showAxes_ ? sf::Color(0, 100, 0) : sf::Color(70, 70, 75));
        btnAxes.setOutlineColor(sf::Color(100, 100, 100));
        btnAxes.setOutlineThickness(1.f);
        btnAxes.setPosition(170, MENU_HEIGHT + 10);
        window_.draw(btnAxes);

        // Icono de ejes (X rojo, Y verde)
        sf::Vertex axisX[] = {
            sf::Vertex(sf::Vector2f(178, MENU_HEIGHT + 30), sf::Color::Red),
            sf::Vertex(sf::Vector2f(202, MENU_HEIGHT + 30), sf::Color::Red)
        };
        window_.draw(axisX, 2, sf::Lines);
        
        sf::Vertex axisY[] = {
            sf::Vertex(sf::Vector2f(190, MENU_HEIGHT + 38), sf::Color::Green),
            sf::Vertex(sf::Vector2f(190, MENU_HEIGHT + 14), sf::Color::Green)
        };
        window_.draw(axisY, 2, sf::Lines);

        // --- Botón Borrar (papelera) ---
        sf::RectangleShape btnClear(sf::Vector2f(40, 40));
        btnClear.setFillColor(sf::Color(70, 70, 75));
        btnClear.setOutlineColor(sf::Color(100, 100, 100));
        btnClear.setOutlineThickness(1.f);
        btnClear.setPosition(220, MENU_HEIGHT + 10);
        window_.draw(btnClear);

        // Icono de papelera (rectángulo con tapa)
        sf::RectangleShape trashBody(sf::Vector2f(16, 14));
        trashBody.setFillColor(sf::Color::Transparent);
        trashBody.setOutlineColor(sf::Color::White);
        trashBody.setOutlineThickness(1.5f);
        trashBody.setPosition(228, MENU_HEIGHT + 22);
        window_.draw(trashBody);

        sf::RectangleShape trashLid(sf::Vector2f(20, 3));
        trashLid.setFillColor(sf::Color::White);
        trashLid.setPosition(226, MENU_HEIGHT + 19);
        window_.draw(trashLid);

        // Líneas verticales de la papelera
        sf::Vertex trashLine1[] = {
            sf::Vertex(sf::Vector2f(233, MENU_HEIGHT + 22), sf::Color::White),
            sf::Vertex(sf::Vector2f(233, MENU_HEIGHT + 36), sf::Color::White)
        };
        window_.draw(trashLine1, 2, sf::Lines);
        
        sf::Vertex trashLine2[] = {
            sf::Vertex(sf::Vector2f(238, MENU_HEIGHT + 22), sf::Color::White),
            sf::Vertex(sf::Vector2f(238, MENU_HEIGHT + 36), sf::Color::White)
        };
        window_.draw(trashLine2, 2, sf::Lines);

        // --- Separador vertical ---
        sf::RectangleShape sep2(sf::Vector2f(1, 40));
        sep2.setFillColor(sf::Color(80, 80, 80));
        sep2.setPosition(270, MENU_HEIGHT + 10);
        window_.draw(sep2);

        // --- Botón Ayuda (?) ---
        sf::RectangleShape btnHelp(sf::Vector2f(40, 40));
        btnHelp.setFillColor(sf::Color(70, 70, 75));
        btnHelp.setOutlineColor(sf::Color(100, 100, 100));
        btnHelp.setOutlineThickness(1.f);
        btnHelp.setPosition(280, MENU_HEIGHT + 10);
        window_.draw(btnHelp);

        // Icono de interrogación
        sf::Text helpIcon("?", font_, 24);
        helpIcon.setFillColor(sf::Color::White);
        // Centrar el texto dentro del botón de 40x40
        helpIcon.setPosition(288, MENU_HEIGHT + 14); 
        window_.draw(helpIcon);

    }

    void App::findSnap() {
        isSnapped_ = false;
        snappedPoint_ = {0.0, 0.0};
        currentSnapType_ = SnapType::NONE;

        double tolerance = 10.0 / viewScale_;
        double minDist = tolerance;

        // Conversión explícita de sf::Vector2f a Point2D
        Point2D mousePos = {currentMouseWorldPos_.x, currentMouseWorldPos_.y};

        // 1. Buscar snaps en puntos clave (Extremos, Centros, Puntos Medios)
        for (const auto& entity : engine_.doc.entities) {
            const Layer* layer = engine_.doc.getLayer(entity->layerName);
            if (!layer || !layer->visible) continue;

            auto snaps = entity->getSnapPoints();
            for (const auto& pt : snaps) {
                double dist = std::hypot(mousePos.x - pt.x, mousePos.y - pt.y);
                if (dist < minDist) {
                    minDist = dist;
                    snappedPoint_ = pt;
                    isSnapped_ = true;
                    currentSnapType_ = SnapType::ENDPOINT; // Por defecto
                }
            }
        }

        // 2. Buscar Intersecciones entre entidades cercanas
        for (size_t i = 0; i < engine_.doc.entities.size(); ++i) {
            for (size_t j = i + 1; j < engine_.doc.entities.size(); ++j) {
                auto& e1 = engine_.doc.entities[i];
                auto& e2 = engine_.doc.entities[j];
                
                // Solo calcular si ambas están cerca del ratón (optimización)
                if (e1->isNear(mousePos, tolerance * 2) && 
                    e2->isNear(mousePos, tolerance * 2)) {
                    
                    // Calcular intersección si ambas son líneas
                    if (auto l1 = dynamic_cast<Line*>(e1.get())) {
                        if (auto l2 = dynamic_cast<Line*>(e2.get())) {
                            // Usamos la función de geometry.hpp (por referencia)
                            auto inter = lineLineIntersection(l1->p1, l1->p2, l2->p1, l2->p2);
                            if (inter.intersects) {
                                double dist = std::hypot(mousePos.x - inter.point.x, 
                                                        mousePos.y - inter.point.y);
                                if (dist < minDist) {
                                    minDist = dist;
                                    snappedPoint_ = inter.point;
                                    isSnapped_ = true;
                                    currentSnapType_ = SnapType::INTERSECTION;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    void App::drawCommandWindow() {
        // Fondo de la ventana de comandos
        sf::RectangleShape cmdBg(sf::Vector2f(WINDOW_WIDTH, COMMAND_HEIGHT));
        cmdBg.setFillColor(sf::Color(60, 60, 60));
        cmdBg.setPosition(0, WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT);
        window_.draw(cmdBg);

        int lineHeight = 20;
        int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
        int startY = WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT + 5;

        int totalLines = commandHistory_.size() + 1;  // +1 para la línea actual
        int maxOffset = std::max(0, totalLines - maxLines);
        
        // Asegurar que el offset no exceda el máximo (por si se borran comandos)
        commandScrollOffset_ = std::min(commandScrollOffset_, maxOffset);

        int startIdx = std::max(0, totalLines - maxLines - commandScrollOffset_);
        int lineCount = 0;

        // Dibujar líneas del historial
        for (int i = startIdx; i < commandHistory_.size() && lineCount < maxLines - 1; ++i) {
            sf::Text histText;
            histText.setFont(font_);
            histText.setString(toSfString("> " + commandHistory_[i]));
            histText.setCharacterSize(12);

            histText.setFillColor(sf::Color(180, 180, 180));
            histText.setPosition(10, startY + lineCount * lineHeight);
            window_.draw(histText);
            lineCount++;
        }

        // Dibujar línea de comando actual (siempre en la parte inferior)
        std::string prompt = "Comando: " + inputBuffer_;
        if (isTyping_ && std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() % 1000 < 500) {
            prompt += "_";  // Cursor parpadeante
        }
        
        sf::Text cmdText;
        cmdText.setFont(font_);
        cmdText.setString(toSfString(prompt));
        cmdText.setCharacterSize(12);

        cmdText.setFillColor(sf::Color::White);
        cmdText.setPosition(10, startY + (maxLines - 1) * lineHeight);
        window_.draw(cmdText);

        // Indicador de scroll DINÁMICO (se mueve según el offset)
        if (totalLines > maxLines) {
            int maxOffset = std::max(0, totalLines - maxLines);
            float scrollRatio = (maxOffset > 0) ? (float)commandScrollOffset_ / maxOffset : 0.0f;
            float availableHeight = (maxLines - 1) * lineHeight;
            float indicatorHeight = 30.f;
            float maxIndicatorY = startY + availableHeight - indicatorHeight;
            float indicatorY = startY + scrollRatio * (maxIndicatorY - startY);
            
            sf::RectangleShape scrollIndicator(sf::Vector2f(5, indicatorHeight));
            scrollIndicator.setFillColor(sf::Color(100, 100, 100));
            scrollIndicator.setPosition(WINDOW_WIDTH - 10, indicatorY);
            window_.draw(scrollIndicator);
        }
    }

    void App::drawCrosshair() {
        // Verificar si el ratón está realmente en el canvas
        bool isInCanvas = (currentMouseScreenPos_.x >= 0 && 
                        currentMouseScreenPos_.x < static_cast<int>(WINDOW_WIDTH) &&
                        currentMouseScreenPos_.y >= static_cast<int>(MENU_HEIGHT + TOOLBAR_HEIGHT) && 
                        currentMouseScreenPos_.y < static_cast<int>(WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT));
        
        if (!isInCanvas) return;

        // Posición del ratón en pantalla
        sf::Vector2f screenPos = {
            static_cast<float>(currentMouseScreenPos_.x),
            static_cast<float>(currentMouseScreenPos_.y)
        };
        
        // Color según snap
        sf::Color crosshairColor = isSnapped_ ? sf::Color(255, 255, 0) : sf::Color(255, 255, 255);
        if (isSnapped_) {
            screenPos = worldToScreen(snappedPoint_.x, snappedPoint_.y);
        }

        // ============================================
        // CRUCETA ROBUSTA: líneas gruesas con borde negro
        // ============================================
        float crosshairSize = 25.0f;  // Más larga
        float thickness = 2.0f;       // Más gruesa
        
        // Función auxiliar para dibujar línea gruesa (como rectángulo)
        auto drawThickLine = [&](sf::Vector2f start, sf::Vector2f end, sf::Color color) {
            sf::Vector2f dir = end - start;
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            sf::Vector2f normal = {-dir.y / len, dir.x / len};
            
            sf::Vertex quad[] = {
                sf::Vertex(start + normal * thickness / 2.f, color),
                sf::Vertex(start - normal * thickness / 2.f, color),
                sf::Vertex(end - normal * thickness / 2.f, color),
                sf::Vertex(end + normal * thickness / 2.f, color)
            };
            window_.draw(quad, 4, sf::Quads);
        };

        // Línea horizontal (con hueco en el centro)
        drawThickLine(
            {screenPos.x - crosshairSize, screenPos.y},
            {screenPos.x - 8.f, screenPos.y},
            crosshairColor
        );
        drawThickLine(
            {screenPos.x + 8.f, screenPos.y},
            {screenPos.x + crosshairSize, screenPos.y},
            crosshairColor
        );
        
        // Línea vertical (con hueco en el centro)
        drawThickLine(
            {screenPos.x, screenPos.y - crosshairSize},
            {screenPos.x, screenPos.y - 8.f},
            crosshairColor
        );
        drawThickLine(
            {screenPos.x, screenPos.y + 8.f},
            {screenPos.x, screenPos.y + crosshairSize},
            crosshairColor
        );
        
        // Punto central más visible
        sf::CircleShape centerPoint(3.f);
        centerPoint.setFillColor(crosshairColor);
        centerPoint.setOrigin(3.f, 3.f);
        centerPoint.setPosition(screenPos);
        window_.draw(centerPoint);
        
        // Si hay snap, dibujar un círculo amarillo alrededor para destacar
        if (isSnapped_) {
            sf::CircleShape snapCircle(8.f);
            snapCircle.setFillColor(sf::Color::Transparent);
            snapCircle.setOutlineColor(sf::Color::Yellow);
            snapCircle.setOutlineThickness(2.f);
            snapCircle.setOrigin(8.f, 8.f);
            snapCircle.setPosition(screenPos);
            window_.draw(snapCircle);
        }
    }

    void App::drawDrawingFeedback() {
        // Color amarillo para el feedback visual
        sf::Color feedbackColor(255, 255, 0, 180); // Amarillo semitransparente
        
        auto w2s = [this](double x, double y) { return worldToScreen(x, y); };
        
        // --- LÍNEA ---
        if (engine_.currentMode == Mode::DRAW_LINE && 
            engine_.statusMessage.find("siguiente") != std::string::npos) {
            // Ya tenemos el primer punto, dibujar línea hasta el ratón
            sf::Vertex line[] = {
                sf::Vertex(w2s(engine_.tempPoint1.x, engine_.tempPoint1.y), feedbackColor),
                sf::Vertex(w2s(currentMouseWorldPos_.x, currentMouseWorldPos_.y), feedbackColor)
            };
            window_.draw(line, 2, sf::Lines);
        }
        
        // --- CÍRCULO ---
        else if (engine_.currentMode == Mode::DRAW_CIRCLE && 
                (engine_.statusMessage.find("Radio") != std::string::npos ||   // ← R mayúscula
                engine_.statusMessage.find("radio") != std::string::npos)) {  // ← r minúscula (por si acaso)
            
            double dx = currentMouseWorldPos_.x - engine_.tempPoint1.x;
            double dy = currentMouseWorldPos_.y - engine_.tempPoint1.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            const int numPoints = 64;
            sf::VertexArray va(sf::LineStrip, numPoints + 1);
            const double PI = 3.14159265358979323846;
            double angleStep = 2.0 * PI / numPoints;
            
            for (int i = 0; i <= numPoints; ++i) {
                double angle = i * angleStep;
                double px = engine_.tempPoint1.x + radius * std::cos(angle);
                double py = engine_.tempPoint1.y + radius * std::sin(angle);
                va[i].position = w2s(px, py);
                va[i].color = feedbackColor;
            }
            window_.draw(va);
        }
        
        // --- ARCO ---
        else if (engine_.currentMode == Mode::DRAW_ARC && 
                engine_.statusMessage.find("radio") != std::string::npos) {
            // Similar al círculo, mostramos el círculo completo como guía
            double dx = currentMouseWorldPos_.x - engine_.tempPoint1.x;
            double dy = currentMouseWorldPos_.y - engine_.tempPoint1.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            sf::CircleShape circle(static_cast<float>(radius * viewScale_));
            circle.setFillColor(sf::Color::Transparent);
            circle.setOutlineColor(feedbackColor);
            circle.setOutlineThickness(1.5f);
            circle.setOrigin(static_cast<float>(radius * viewScale_), 
                            static_cast<float>(radius * viewScale_));
            circle.setPosition(w2s(engine_.tempPoint1.x, engine_.tempPoint1.y));
            window_.draw(circle);
        }
        
        // --- POLILÍNEA ---
        else if (engine_.currentMode == Mode::DRAW_POLYLINE && 
                !engine_.tempPolylinePoints.empty()) {
            // Dibujar línea desde el último punto de la polilínea hasta el ratón
            const auto& lastPt = engine_.tempPolylinePoints.back();
            sf::Vertex line[] = {
                sf::Vertex(w2s(lastPt.x, lastPt.y), feedbackColor),
                sf::Vertex(w2s(currentMouseWorldPos_.x, currentMouseWorldPos_.y), feedbackColor)
            };
            window_.draw(line, 2, sf::Lines);
        }
        
        // --- POLÍGONO ---
        else if (engine_.currentMode == Mode::DRAW_POLYGON && 
                engine_.statusMessage.find("radio") != std::string::npos) {
            // Ya tenemos el centro y los lados, dibujar polígono con radio hasta el ratón
            double dx = currentMouseWorldPos_.x - engine_.tempPolygonCenter.x;
            double dy = currentMouseWorldPos_.y - engine_.tempPolygonCenter.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            int sides = engine_.tempPolygonSides > 0 ? engine_.tempPolygonSides : 6;
            sf::VertexArray va(sf::LineStrip, sides + 1);

            // SOLUCIÓN: Usar constante local en lugar de std::numbers::pi
            const double PI = 3.14159265358979323846;
            double angleStep = 2.0 * PI / sides;
            
            for (int i = 0; i <= sides; ++i) {
                double angle = i * angleStep - PI / 2.0;
                double px = engine_.tempPolygonCenter.x + radius * std::cos(angle);
                double py = engine_.tempPolygonCenter.y + radius * std::sin(angle);
                va[i].position = w2s(px, py);
                va[i].color = feedbackColor;
            }
            window_.draw(va);
        }
        // --- ELIPSE ---
        else if (engine_.currentMode == Mode::DRAW_ELLIPSE) {
            const int numPoints = 64;
            const double PI = 3.14159265358979323846;
            double angleStep = 2.0 * PI / numPoints;
            
            // PASO 1: Esperando centro - dibujar un pequeño punto
            if (engine_.statusMessage.find("centro") != std::string::npos || 
                engine_.statusMessage.find("Centro") != std::string::npos) {
                sf::CircleShape dot(3.0f);
                dot.setFillColor(feedbackColor);
                dot.setOrigin(3.0f, 3.0f);
                dot.setPosition(worldToScreen(currentMouseWorldPos_.x, currentMouseWorldPos_.y));
                window_.draw(dot);
            }
            // PASO 2: Esperando eje mayor - dibujar línea desde centro hasta ratón
            else if (engine_.statusMessage.find("eje mayor") != std::string::npos ||
                    engine_.statusMessage.find("Eje mayor") != std::string::npos) {
                sf::Vertex line[] = {
                    sf::Vertex(worldToScreen(engine_.tempPoint1.x, engine_.tempPoint1.y), feedbackColor),
                    sf::Vertex(worldToScreen(currentMouseWorldPos_.x, currentMouseWorldPos_.y), feedbackColor)
                };
                window_.draw(line, 2, sf::Lines);
            }
            // PASO 3: Esperando eje menor - dibujar elipse completa
            else {
                // Calcular eje mayor (ya definido)
                double dx = engine_.tempPoint2.x - engine_.tempPoint1.x;
                double dy = engine_.tempPoint2.y - engine_.tempPoint1.y;
                double majorRadius = std::sqrt(dx * dx + dy * dy);
                double rotationAngle = std::atan2(dy, dx);
                
                // Calcular eje menor (del ratón)
                double distToMouse = std::sqrt(
                    std::pow(currentMouseWorldPos_.x - engine_.tempPoint1.x, 2) +
                    std::pow(currentMouseWorldPos_.y - engine_.tempPoint1.y, 2)
                );
                double minorRadius = distToMouse;
                
                // Dibujar elipse
                sf::VertexArray va(sf::LineStrip, numPoints + 1);
                for (int i = 0; i <= numPoints; ++i) {
                    double angle = i * angleStep;
                    double rotatedAngle = angle + rotationAngle;
                    double px = engine_.tempPoint1.x + majorRadius * std::cos(rotatedAngle);
                    double py = engine_.tempPoint1.y + minorRadius * std::sin(rotatedAngle);
                    va[i].position = worldToScreen(px, py);
                    va[i].color = feedbackColor;
                }
                window_.draw(va);
            }
        }
        // --- COTA (DIMENSION) ---
        else if (engine_.currentMode == Mode::DRAW_DIMENSION) {
            Point2D mousePos = {currentMouseWorldPos_.x, currentMouseWorldPos_.y};
            
            // PASO 2: Esperando el segundo punto (dibujar línea desde P1 hasta el ratón)
            if (engine_.statusMessage.find("Segundo") != std::string::npos) {
                sf::Vertex line[] = {
                    sf::Vertex(worldToScreen(engine_.tempDimP1.x, engine_.tempDimP1.y), feedbackColor),
                    sf::Vertex(worldToScreen(mousePos.x, mousePos.y), feedbackColor)
                };
                window_.draw(line, 2, sf::Lines);
            }
            // PASO 3: Esperando la ubicación (dibujar la cota fantasma completa)
            else if (engine_.statusMessage.find("Ubicación") != std::string::npos || 
                    engine_.statusMessage.find("ubicación") != std::string::npos) {
                
                // Determinar si será horizontal o vertical (misma lógica que al crearla)
                double cx = (engine_.tempDimP1.x + engine_.tempDimP2.x) / 2.0;
                double cy = (engine_.tempDimP1.y + engine_.tempDimP2.y) / 2.0;
                bool isHoriz = std::abs(mousePos.y - cy) > std::abs(mousePos.x - cx);
                
                // Calcular puntos de la cota fantasma
                Point2D ext1, ext2, lineStart, lineEnd;
                if (isHoriz) {
                    double y = mousePos.y;
                    ext1 = {engine_.tempDimP1.x, y}; ext2 = {engine_.tempDimP2.x, y};
                    lineStart = {engine_.tempDimP1.x, y}; lineEnd = {engine_.tempDimP2.x, y};
                } else {
                    double x = mousePos.x;
                    ext1 = {x, engine_.tempDimP1.y}; ext2 = {x, engine_.tempDimP2.y};
                    lineStart = {x, engine_.tempDimP1.y}; lineEnd = {x, engine_.tempDimP2.y};
                }
                
                sf::Color extColor(255, 255, 0, 100); // Amarillo más transparente para extensiones
                
                // Líneas de extensión
                sf::Vertex extLine1[] = { 
                    sf::Vertex(worldToScreen(engine_.tempDimP1.x, engine_.tempDimP1.y), extColor), 
                    sf::Vertex(worldToScreen(ext1.x, ext1.y), extColor) 
                };
                sf::Vertex extLine2[] = { 
                    sf::Vertex(worldToScreen(engine_.tempDimP2.x, engine_.tempDimP2.y), extColor), 
                    sf::Vertex(worldToScreen(ext2.x, ext2.y), extColor) 
                };
                window_.draw(extLine1, 2, sf::Lines);
                window_.draw(extLine2, 2, sf::Lines);
                
                // Línea de cota
                sf::Vertex dimLine[] = { 
                    sf::Vertex(worldToScreen(lineStart.x, lineStart.y), feedbackColor), 
                    sf::Vertex(worldToScreen(lineEnd.x, lineEnd.y), feedbackColor) 
                };
                window_.draw(dimLine, 2, sf::Lines);
            }
        }
    }

    void App::drawGrips() {
        const float gripSize = 6.0f;
        sf::Color gripColor(0, 100, 255); // Azul
        sf::Color activeGripColor(255, 0, 0); // Rojo para el grip activo

        for (Entity* entity : engine_.selectedEntities) {
            auto grips = entity->getGripPoints();
            for (int i = 0; i < grips.size(); ++i) {
                sf::Vector2f screenPos = worldToScreen(grips[i].x, grips[i].y);
                
                // Si es el grip que estamos arrastrando, lo pintamos rojo y más grande
                bool isActive = (engine_.currentMode == Mode::GRIP_EDIT && 
                                engine_.activeGripEntity == entity && 
                                engine_.activeGripIndex == i);
                
                float size = isActive ? gripSize * 1.5f : gripSize;
                sf::Color color = isActive ? activeGripColor : gripColor;

                sf::RectangleShape grip(sf::Vector2f(size, size));
                grip.setFillColor(color);
                grip.setOrigin(size / 2.f, size / 2.f);
                grip.setPosition(screenPos);
                window_.draw(grip);
            }
        }
    }

    void App::drawDimensionTexts() {
        // IMPORTANTE: Cambia 'font_' por el nombre de la variable de fuente que uses en tu App
        // Si no tienes ninguna, el texto no se dibujará, pero no dará error.
        
        for (const auto& entity : engine_.doc.entities) {
            if (auto* dim = dynamic_cast<Dimension*>(entity.get())) {
                // Formatear el valor a 2 decimales
                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << dim->value;
                
                // Calcular la posición del texto (centro de la línea de cota)
                double textX = dim->isHorizontal ? (dim->p1.x + dim->p2.x) / 2.0 : dim->location.x;
                double textY = dim->isHorizontal ? dim->location.y : (dim->p1.y + dim->p2.y) / 2.0;
                
                sf::Vector2f screenPos = worldToScreen(textX, textY);

                // Dibujar el texto (Asegúrate de que 'font_' existe en tu clase App)
                sf::Text text;
                text.setFont(font_); // <--- DESCOMENTA Y USA TU FUENTE REAL
                text.setString(oss.str());
                text.setCharacterSize(12);
                text.setFillColor(sf::Color::White);
                text.setOrigin(text.getLocalBounds().width / 2.f, text.getLocalBounds().height / 2.f);
                text.setPosition(screenPos);
                
                window_.draw(text);
            }
        }
    }

} // namespace cad