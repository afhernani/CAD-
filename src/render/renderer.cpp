#include "cad/render/renderer.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/ellipse.hpp"
#include "cad/core/geometry/entities/dimension.hpp"
#include "cad/core/geometry/entities/text.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/core/geometry/intersections.hpp"
#include <sstream>
#include <cmath>
#include <iomanip>

namespace cad {

    // Helper local para convertir UTF-8 a sf::String
    namespace {
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
    }

    void Renderer::render(sf::RenderWindow& window, const View& view, Engine& engine, 
                        const sf::Vector2i& mouseScreenPos, const Point2D& mouseWorldPos,
                        sf::Font& font, bool isSnapped, const Point2D& snappedPoint, bool showAxes, SnapType SnapType) const {
        drawGrid(window, view, engine);
        if (showAxes) drawAxes(window, view);
        drawEntities(window, view, engine);
        drawTexts(window, view, engine, font);
        drawGrips(window, view, engine);
        drawDimensionTexts(window, view, engine, font);
        drawCrosshair(window, view, mouseScreenPos, mouseWorldPos, isSnapped, snappedPoint, SnapType);
        drawDrawingFeedback(window, view, engine, mouseWorldPos, font);
        
    }

    void Renderer::drawGrid(sf::RenderWindow& window, const View& view, const Engine& engine) const {
        if (!engine.gridEnabled) return;
        
        double currentGridSize = 10.0;
        double pixelSpacing = currentGridSize * view.getScale();
        
        while (pixelSpacing < 30.0) { currentGridSize *= 10.0; pixelSpacing = currentGridSize * view.getScale(); }
        while (pixelSpacing > 150.0) { currentGridSize /= 10.0; pixelSpacing = currentGridSize * view.getScale(); }

        Point2D tl = view.screenToWorld(0, 0);
        Point2D br = view.screenToWorld(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
        
        double left = std::min(tl.x, br.x), right = std::max(tl.x, br.x);
        double top = std::max(tl.y, br.y), bottom = std::min(tl.y, br.y);

        double startX = std::floor(left / currentGridSize) * currentGridSize;
        double startY = std::floor(top / currentGridSize) * currentGridSize;

        sf::Color gridColor(50, 50, 50);
        sf::VertexArray lines(sf::Lines);

        for (double x = startX; x <= right; x += currentGridSize) {
            sf::Vector2f topPos = view.worldToScreen(x, top);
            sf::Vector2f botPos = view.worldToScreen(x, bottom);
            lines.append(sf::Vertex(topPos, gridColor));
            lines.append(sf::Vertex(botPos, gridColor));
        }
        for (double y = startY; y <= bottom; y += currentGridSize) {
            sf::Vector2f leftPos = view.worldToScreen(left, y);
            sf::Vector2f rightPos = view.worldToScreen(right, y);
            lines.append(sf::Vertex(leftPos, gridColor));
            lines.append(sf::Vertex(rightPos, gridColor));
        }
        window.draw(lines);
    }

    void Renderer::drawAxes(sf::RenderWindow& window, const View& view) const {
        sf::Vertex xAxis[] = {
            sf::Vertex(view.worldToScreen(0, 0), sf::Color::Red),
            sf::Vertex(view.worldToScreen(50, 0), sf::Color::Red)
        };
        window.draw(xAxis, 2, sf::Lines);

        sf::Vertex yAxis[] = {
            sf::Vertex(view.worldToScreen(0, 0), sf::Color::Green),
            sf::Vertex(view.worldToScreen(0, 50), sf::Color::Green)
        };
        window.draw(yAxis, 2, sf::Lines);

        sf::CircleShape origin(3.f);
        origin.setFillColor(sf::Color::White);
        origin.setOrigin(3.f, 3.f);
        origin.setPosition(view.worldToScreen(0, 0));
        window.draw(origin);
    }

    void Renderer::drawEntities(sf::RenderWindow& window, const View& view, Engine& engine) const {
        // 1. Obtener los límites visibles del mundo (Viewport Culling)
        Rect viewportBounds = view.getVisibleWorldBounds();

        // 2. Crear la lambda de conversión localmente (como en el resto del código)
        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        for (const auto& entity : engine.doc.entities) {
            // >>> FILTRO DE CAPA <<<
            const Layer* layer = engine.doc.getLayer(entity->layerName);
            if (!layer || !layer->visible || layer->frozen) continue;

            // >>> VIEWPORT CULLING: La magia ocurre aquí <<<
            if (!entity->getBoundingBox().intersects(viewportBounds)) {
                continue; // ¡Saltamos el draw() completamente!
            }

            // Si llegamos aquí, la entidad es visible. La dibujamos.
            sf::Color color = layer ? layer->color : sf::Color::White;
            
            // Usamos la lambda w2s que acabamos de crear
            entity->draw(window, w2s, color, view.getScale());
        }
    }

    void Renderer::drawGrips(sf::RenderWindow& window, const View& view, const Engine& engine) const {
        const float gripSize = 6.0f;
        sf::Color gripColor(0, 100, 255);
        sf::Color activeGripColor(255, 0, 0);

        for (Entity* entity : engine.selectedEntities) {
            // >>> FILTRO AÑADIDO <<<
            const Layer* layer = engine.doc.getLayer(entity->layerName);
            if (!layer || !layer->visible || layer->frozen) continue;

            auto grips = entity->getGripPoints();
            for (int i = 0; i < grips.size(); ++i) {
                sf::Vector2f screenPos = view.worldToScreen(grips[i].x, grips[i].y);
                bool isActive = (engine.currentMode == Mode::GRIP_EDIT &&
                                engine.activeGripEntity == entity &&
                                engine.activeGripIndex == i);
                float size = isActive ? gripSize * 1.5f : gripSize;
                sf::Color color = isActive ? activeGripColor : gripColor;

                sf::RectangleShape grip(sf::Vector2f(size, size));
                grip.setFillColor(color);
                grip.setOrigin(size / 2.f, size / 2.f);
                grip.setPosition(screenPos);
                window.draw(grip);
            }
        }
    }

    void Renderer::drawDimensionTexts(sf::RenderWindow& window, const View& view, const Engine& engine, sf::Font& font) const {
        const double PI = 3.14159265358979323846;
        for (const auto& entity : engine.doc.entities) {
            if (auto* dim = dynamic_cast<Dimension*>(entity.get())) {

                // >>> FILTRO DE VISIBILIDAD DE CAPA PARA COTAS <<<
                const Layer* layer = engine.doc.getLayer(dim->layerName);
                if (!layer || !layer->visible || layer->frozen || layer->locked) {
                    continue; // Si la capa no es visible o está congelada, no dibujamos el texto
                }

                std::ostringstream oss;
                oss << std::fixed << std::setprecision(2) << dim->value;
                double textX, textY;
                float rotation = 0.0f;

                if (dim->type == DimType::ALIGNED || dim->isAligned) {
                    double dx = dim->p2.x - dim->p1.x;
                    double dy = dim->p2.y - dim->p1.y;
                    double len = std::sqrt(dx*dx + dy*dy);
                    if (len == 0) continue;
                    double nx = -dy / len; double ny = dx / len;
                    double vx = dim->location.x - dim->p1.x; double vy = dim->location.y - dim->p1.y;
                    double offset = vx * nx + vy * ny;
                    textX = (dim->p1.x + dim->p2.x) / 2.0 + nx * offset;
                    textY = (dim->p1.y + dim->p2.y) / 2.0 + ny * offset;
                    rotation = std::atan2(dy, dx) * 180.0f / (float)PI;
                    if (rotation > 90.0f && rotation < 270.0f) rotation -= 180.0f;
                }
                else if (dim->type == DimType::ANGULAR) {
                    double arcRadius = std::hypot(dim->p3.x - dim->location.x, dim->p3.y - dim->location.y);
                    if (arcRadius < 1.0) arcRadius = 10.0;
                    double angle1 = std::atan2(dim->p1.y - dim->location.y, dim->p1.x - dim->location.x);
                    double angle2 = std::atan2(dim->p2.y - dim->location.y, dim->p2.x - dim->location.x);
                    double diff = angle2 - angle1;
                    while (diff < 0) diff += 2 * PI;
                    while (diff >= 2 * PI) diff -= 2 * PI;
                    double midAngle = angle1 + diff / 2.0;
                    double textRadius = arcRadius * 1.3;
                    textX = dim->location.x + textRadius * std::cos(midAngle);
                    textY = dim->location.y + textRadius * std::sin(midAngle);
                }
                else if (dim->type == DimType::RADIUS || dim->type == DimType::DIAMETER) {
                    textX = (dim->p1.x + dim->p2.x) / 2.0;
                    textY = (dim->p1.y + dim->p2.y) / 2.0;
                }
                else {
                    textX = dim->isHorizontal ? (dim->p1.x + dim->p2.x) / 2.0 : dim->location.x;
                    textY = dim->isHorizontal ? dim->location.y : (dim->p1.y + dim->p2.y) / 2.0;
                }

                sf::Vector2f screenPos = view.worldToScreen(textX, textY);
                sf::Text text;
                text.setFont(font);
                text.setString(toSfString(oss.str()));
                text.setCharacterSize(12);
                text.setFillColor(sf::Color::White);
                text.setRotation(rotation);

                sf::FloatRect bounds = text.getLocalBounds();
                float padding = 4.0f;
                sf::RectangleShape bg(sf::Vector2f(bounds.width + padding * 2, bounds.height + padding));
                bg.setFillColor(sf::Color(30, 30, 30));
                bg.setOrigin(bounds.width / 2.f + padding, bounds.height / 2.f);
                bg.setPosition(screenPos);
                bg.setRotation(rotation);
                window.draw(bg);

                text.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
                text.setPosition(screenPos);
                window.draw(text);
            }
        }
    }

    void Renderer::drawCrosshair(sf::RenderWindow& window, const View& view, const sf::Vector2i& mouseScreenPos, 
                            const Point2D& mouseWorldPos, bool isSnapped, const Point2D& snappedPoint, 
                            cad::SnapType snapType) const {
    
        bool isInCanvas = (mouseScreenPos.x >= 0 && mouseScreenPos.x < static_cast<int>(WINDOW_WIDTH) &&
                        mouseScreenPos.y >= static_cast<int>(MENU_HEIGHT + TOOLBAR_HEIGHT) &&
                        mouseScreenPos.y < static_cast<int>(WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT));
        if (!isInCanvas) return;

        sf::Vector2f screenPos = isSnapped ? view.worldToScreen(snappedPoint.x, snappedPoint.y) 
                                        : sf::Vector2f(static_cast<float>(mouseScreenPos.x), static_cast<float>(mouseScreenPos.y));

        // ============================================================
        // 1. DIBUJAR LA CRUCETA (siempre visible, estilo AutoCAD)
        // ============================================================
        float crosshairSize = 15.0f;
        float gap = 8.0f; // Espacio entre el centro y el inicio de las líneas
        sf::Color crossColor(220, 220, 220); // Gris muy claro, casi blanco
        float thickness = 1.0f;

        auto drawThickLine = [&](sf::Vector2f start, sf::Vector2f end, sf::Color color) {
            sf::Vector2f dir = end - start;
            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len < 0.001f) return;
            sf::Vector2f normal = {-dir.y / len, dir.x / len};
            sf::Vertex quad[] = {
                sf::Vertex(start + normal * thickness / 2.f, color),
                sf::Vertex(start - normal * thickness / 2.f, color),
                sf::Vertex(end - normal * thickness / 2.f, color),
                sf::Vertex(end + normal * thickness / 2.f, color)
            };
            window.draw(quad, 4, sf::Quads);
        };

        // Líneas de la cruceta (con hueco en el centro)
        drawThickLine({screenPos.x - crosshairSize, screenPos.y}, {screenPos.x - gap, screenPos.y}, crossColor);
        drawThickLine({screenPos.x + gap, screenPos.y}, {screenPos.x + crosshairSize, screenPos.y}, crossColor);
        drawThickLine({screenPos.x, screenPos.y - crosshairSize}, {screenPos.x, screenPos.y - gap}, crossColor);
        drawThickLine({screenPos.x, screenPos.y + gap}, {screenPos.x, screenPos.y + crosshairSize}, crossColor);

        // ============================================================
        // 2. DIBUJAR EL CENTRO (cuadrado pequeño o icono de snap brillante)
        // ============================================================
        if (isSnapped) {
            // Icono de SNAP: más grande, brillante y con relleno semi-transparente
            sf::Color snapColor(255, 255, 0); // Amarillo puro y brillante
            float size = 7.0f; // Tamaño del icono

            if (snapType == cad::SnapType::CENTER) {
                // Círculo para Centro - con relleno amarillo tenue y borde brillante
                sf::CircleShape circle(size);
                circle.setOrigin(size, size);
                circle.setPosition(screenPos);
                circle.setFillColor(sf::Color(255, 255, 0, 60)); // Relleno tenue
                circle.setOutlineColor(snapColor);
                circle.setOutlineThickness(2.0f);
                window.draw(circle);
            } 
            else if (snapType == cad::SnapType::MIDPOINT) {
                // Triángulo para Punto Medio
                sf::ConvexShape triangle(3);
                triangle.setPoint(0, sf::Vector2f(0, -size));
                triangle.setPoint(1, sf::Vector2f(size * 0.866f, size * 0.5f));
                triangle.setPoint(2, sf::Vector2f(-size * 0.866f, size * 0.5f));
                triangle.setOrigin(0, 0);
                triangle.setPosition(screenPos);
                triangle.setFillColor(sf::Color(255, 255, 0, 60));
                triangle.setOutlineColor(snapColor);
                triangle.setOutlineThickness(2.0f);
                window.draw(triangle);
            } 
            else if (snapType == cad::SnapType::INTERSECTION) {
                // X para Intersección - más gruesa y brillante
                float s = size * 0.85f;
                sf::Vertex line1[] = {
                    sf::Vertex(sf::Vector2f(screenPos.x - s, screenPos.y - s), snapColor),
                    sf::Vertex(sf::Vector2f(screenPos.x + s, screenPos.y + s), snapColor)
                };
                sf::Vertex line2[] = {
                    sf::Vertex(sf::Vector2f(screenPos.x - s, screenPos.y + s), snapColor),
                    sf::Vertex(sf::Vector2f(screenPos.x + s, screenPos.y - s), snapColor)
                };
                // Dibujar la X con líneas gruesas usando quads
                auto drawThickX = [&](sf::Vector2f start, sf::Vector2f end, sf::Color color) {
                    sf::Vector2f dir = end - start;
                    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                    sf::Vector2f normal = {-dir.y / len, dir.x / len};
                    float t = 2.0f; // Grosor de la X
                    sf::Vertex quad[] = {
                        sf::Vertex(start + normal * t / 2.f, color),
                        sf::Vertex(start - normal * t / 2.f, color),
                        sf::Vertex(end - normal * t / 2.f, color),
                        sf::Vertex(end + normal * t / 2.f, color)
                    };
                    window.draw(quad, 4, sf::Quads);
                };
                drawThickX({screenPos.x - s, screenPos.y - s}, {screenPos.x + s, screenPos.y + s}, snapColor);
                drawThickX({screenPos.x - s, screenPos.y + s}, {screenPos.x + s, screenPos.y - s}, snapColor);
            } 
            else {
                // Cuadrado para Extremo (Default) - con relleno y borde brillante
                sf::RectangleShape square(sf::Vector2f(size * 2, size * 2));
                square.setOrigin(size, size);
                square.setPosition(screenPos);
                square.setFillColor(sf::Color(255, 255, 0, 60)); // Relleno tenue
                square.setOutlineColor(snapColor);
                square.setOutlineThickness(2.0f);
                window.draw(square);
            }
        } 
        else {
            // Sin snap: pequeño cuadrado en el centro de la cruceta (estilo AutoCAD)
            float boxSize = 3.0f;
            sf::RectangleShape centerBox(sf::Vector2f(boxSize * 2, boxSize * 2));
            centerBox.setOrigin(boxSize, boxSize);
            centerBox.setPosition(screenPos);
            centerBox.setFillColor(sf::Color::Transparent);
            centerBox.setOutlineColor(crossColor);
            centerBox.setOutlineThickness(1.0f);
            window.draw(centerBox);
        }
    }

    void Renderer::drawDrawingFeedback(sf::RenderWindow& window, const View& view, Engine& engine, 
                                    const Point2D& mouseWorldPos, sf::Font& font) const {
        if (engine.activeCommand_) {
            // >>> ACTUALIZADO: Ahora pasamos 'engine' como tercer argumento
            engine.activeCommand_->drawFeedback(window, view, engine, mouseWorldPos, font);
        }
    }

    void Renderer::drawSelectionRect(sf::RenderWindow& window, const View& view,
                                    const Point2D& startPoint, const Point2D& endPoint) const {
        auto startScreen = view.worldToScreen(startPoint.x, startPoint.y);
        auto endScreen = view.worldToScreen(endPoint.x, endPoint.y);
        
        sf::RectangleShape selectionRect;
        selectionRect.setPosition(std::min(startScreen.x, endScreen.x), 
                                std::min(startScreen.y, endScreen.y));
        selectionRect.setSize({std::abs(endScreen.x - startScreen.x), 
                            std::abs(endScreen.y - startScreen.y)});
        selectionRect.setOutlineThickness(1.0f);
        
        // Determinar color según dirección del arrastre
        if (startPoint.x < endPoint.x) {
            // WINDOW: Azul (izquierda → derecha)
            selectionRect.setFillColor(sf::Color(0, 100, 255, 40));   // Azul translúcido
            selectionRect.setOutlineColor(sf::Color::Blue);
        } else {
            // CROSSING: Verde (derecha → izquierda)
            selectionRect.setFillColor(sf::Color(0, 200, 100, 40));   // Verde translúcido
            selectionRect.setOutlineColor(sf::Color::Green);
        }
        
        window.draw(selectionRect);
    }

    void Renderer::drawTexts(sf::RenderWindow& window, const View& view, const Engine& engine, sf::Font& font) const {
        for (const auto& entity : engine.doc.entities) {
            if (auto* text = dynamic_cast<Text*>(entity.get())) {
                const Layer* layer = engine.doc.getLayer(text->layerName);
                if (!layer || !layer->visible || layer->frozen) continue;
                
                sf::Color drawColor = layer->color;
                if (layer->locked) drawColor.a = 100;

                sf::Vector2f screenPos = view.worldToScreen(text->position.x, text->position.y);
                sf::Text sfText;
                sfText.setFont(font);
                sfText.setString(toSfString(text->content));
                sfText.setCharacterSize(static_cast<unsigned int>(text->height * view.getScale()));
                sfText.setFillColor(drawColor);
                sfText.setRotation(-text->rotation * 180.0f / 3.14159f);
                sfText.setOrigin(0, text->height * view.getScale());
                sfText.setPosition(screenPos);
                window.draw(sfText);
            }
        }
    }

} // namespace cad