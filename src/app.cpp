#include "app.hpp"
#include <iostream>
#include <format>
#include <chrono>
#include <cmath>

namespace cad {

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
                }
            }

            // --- SCROLL EN VENTANA DE COMANDOS ---
            if (event.type == sf::Event::MouseWheelScrolled) {
                int mx = event.mouseWheelScroll.x;
                int my = event.mouseWheelScroll.y;
                
                // Si el ratón está sobre la ventana de comandos
                if (my >= WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT && 
                    my < WINDOW_HEIGHT - STATUS_HEIGHT) {
                    if (event.mouseWheelScroll.delta > 0) {
                        commandScrollOffset_ = std::max(0, commandScrollOffset_ - 1);
                    } else {
                        commandScrollOffset_++;
                    }
                }
            }

            // --- ZOOM (solo en canvas) ---
            if (event.type == sf::Event::MouseWheelScrolled) {
                int my = event.mouseWheelScroll.y;
                if (my >= MENU_HEIGHT + TOOLBAR_HEIGHT && 
                    my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    if (event.mouseWheelScroll.delta > 0) viewScale_ *= 1.1f; 
                    else if (event.mouseWheelScroll.delta < 0) viewScale_ /= 1.1f;
                    
                    if (viewScale_ < 0.01f) viewScale_ = 0.01f;
                    if (viewScale_ > 100.0f) viewScale_ = 100.0f;
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
                }
                // B) Clic en el Canvas
                else if (my >= MENU_HEIGHT + TOOLBAR_HEIGHT && my < WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT) {
                    if (engine_.currentMode != Mode::IDLE) {
                        Point2D targetPoint = isSnapped_ ? snappedPoint_ : Point2D{currentMouseWorldPos_.x, currentMouseWorldPos_.y};
                        std::string coord = std::format("{:.6f},{:.6f}", targetPoint.x, targetPoint.y);
                        engine_.processInput(coord);
                        inputBuffer_.clear(); 
                    }
                }
            }

            // --- ESCRITURA EN LÍNEA DE COMANDOS ---
            if (event.type == sf::Event::TextEntered && isTyping_) {
                if (event.text.unicode == 13) { // Enter
                    if (!inputBuffer_.empty()) {
                        // Guardar en historial antes de procesar
                        commandHistory_.push_back(inputBuffer_);
                        // Limitar historial a 100 comandos
                        if (commandHistory_.size() > 100) {
                            commandHistory_.erase(commandHistory_.begin());
                        }
                        
                        engine_.processInput(inputBuffer_);
                        inputBuffer_.clear();
                        commandScrollOffset_ = 0;  // Resetear scroll al escribir
                    }
                } else if (event.text.unicode == 8) { // Backspace
                    if (!inputBuffer_.empty()) inputBuffer_.pop_back();
                } else if (event.text.unicode < 128) {
                    inputBuffer_ += static_cast<char>(event.text.unicode);
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
        drawCrosshair();

        // Dibujar la interfaz
        drawUI();
        drawToolbar();

        window_.display();
    }

    void App::drawGrid() {
        sf::RectangleShape line;
        line.setFillColor(sf::Color(50, 50, 50));
        
        // Rejilla que cubre todo el canvas (1280 x CANVAS_HEIGHT)
        for (int i = 0; i <= WINDOW_WIDTH; i += 50) {
            line.setSize(sf::Vector2f(1, CANVAS_HEIGHT));
            line.setPosition(i, MENU_HEIGHT + TOOLBAR_HEIGHT);
            window_.draw(line);
        }
        for (int i = 0; i <= CANVAS_HEIGHT; i += 50) {
            line.setSize(sf::Vector2f(WINDOW_WIDTH, 1));
            line.setPosition(0, MENU_HEIGHT + TOOLBAR_HEIGHT + i);
            window_.draw(line);
        }
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
            
            // Llamada polimórfica: cada entidad sabe cómo dibujarse
            entity->draw(window_, w2s, layer->color, viewScale_);
        }
        
        // Marcador de Object Snap
        if (isSnapped_) {
            sf::Vector2f screenPos = worldToScreen(snappedPoint_.x, snappedPoint_.y);
            sf::RectangleShape marker(sf::Vector2f(8.f, 8.f));
            marker.setFillColor(sf::Color::Transparent);
            marker.setOutlineColor(sf::Color::Yellow);
            marker.setOutlineThickness(1.5f);
            marker.setOrigin(4.f, 4.f);
            marker.setPosition(screenPos);
            window_.draw(marker);
        }
    }

    void App::drawUI() {
        // Barra de Menú (Arriba)
        sf::RectangleShape menu(sf::Vector2f(WINDOW_WIDTH, MENU_HEIGHT));
        menu.setFillColor(sf::Color(45, 45, 48));
        menu.setPosition(0, 0);
        window_.draw(menu);
        
        sf::Text menuTxt("Archivo  Editar  Ver  Dibujar  Modificar", font_, 14);
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
        std::string statusStr = std::format("{} | X: {:.2f}, Y: {:.2f} | Zoom: {:.2f}x", 
            engine_.statusMessage, 
            currentMouseWorldPos_.x, 
            currentMouseWorldPos_.y, 
            viewScale_);
            
        sf::Text statusTxt(statusStr, font_, 12);
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
    }

    void App::findSnap() {
        isSnapped_ = false;
        
        float snapRadiusPixels = 10.0f;
        float snapRadiusWorld = snapRadiusPixels / viewScale_;
        float snapDistSq = snapRadiusWorld * snapRadiusWorld;

        // Recorremos todas las entidades polimórficamente
        for (const auto& entity : engine_.doc.entities) {
            // Intentamos convertir la entidad a una Línea
            if (const auto* line = dynamic_cast<const Line*>(entity.get())) {
                // Comprobar punto final 1 (p1)
                float dx1 = currentMouseWorldPos_.x - line->p1.x;
                float dy1 = currentMouseWorldPos_.y - line->p1.y;
                if ((dx1 * dx1 + dy1 * dy1) < snapDistSq) {
                    isSnapped_ = true;
                    snappedPoint_ = line->p1;
                    return;
                }

                // Comprobar punto final 2 (p2)
                float dx2 = currentMouseWorldPos_.x - line->p2.x;
                float dy2 = currentMouseWorldPos_.y - line->p2.y;
                if ((dx2 * dx2 + dy2 * dy2) < snapDistSq) {
                    isSnapped_ = true;
                    snappedPoint_ = line->p2;
                    return;
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

        // Calcular cuántas líneas caben (aproximadamente 20px por línea)
        int lineHeight = 20;
        int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
        int startY = WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT + 5;

        // Mostrar historial de comandos con scroll
        int totalLines = commandHistory_.size() + 1;  // +1 para la línea actual
        int startIdx = std::max(0, totalLines - maxLines - commandScrollOffset_);
        
        int lineCount = 0;
        
        // Dibujar líneas del historial
        for (int i = startIdx; i < commandHistory_.size() && lineCount < maxLines - 1; ++i) {
            sf::Text histText("> " + commandHistory_[i], font_, 12);
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
        
        sf::Text cmdText(prompt, font_, 12);
        cmdText.setFillColor(sf::Color::White);
        cmdText.setPosition(10, startY + (maxLines - 1) * lineHeight);
        window_.draw(cmdText);

        // Indicador de scroll si hay más contenido
        if (totalLines > maxLines) {
            sf::RectangleShape scrollIndicator(sf::Vector2f(5, 30));
            scrollIndicator.setFillColor(sf::Color(100, 100, 100));
            scrollIndicator.setPosition(WINDOW_WIDTH - 10, startY + 10);
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

} // namespace cad