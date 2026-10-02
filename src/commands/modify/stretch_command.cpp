#include "cad/commands/modify/stretch_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/line.hpp"   // OBLIGATORIO para el preview
#include "cad/core/geometry/entities/circle.hpp" // Opcional, si soportas estirar círculos
#include "cad/render/view.hpp"                   // OBLIGATORIO
#include <SFML/Graphics.hpp>                     // OBLIGATORIO
#include "cad/core/constants.hpp"                // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>
#include <algorithm>

namespace cad {

    StretchCommand::StretchCommand() {
        statusMessage_ = "STRETCH | Selecciona entidades con ventana de cruce (primer punto):";
    }

    void StretchCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        statusMessage_ = "STRETCH | Formato no válido. Usa coordenadas x,y.";
    }

    void StretchCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingBasePoint) {
            basePoint_ = point;
            hasBasePoint_ = true;
            step_ = Step::WaitingDestPoint;
            statusMessage_ = "STRETCH | Punto destino:";
        }
        else if (step_ == Step::WaitingDestPoint) {
            destPoint_ = point;
            executeStretch(engine);
            finished_ = true;
        }
    }

    void StretchCommand::executeStretch(Engine& engine) {
        if (selectedEntities_.empty() || !hasWindow_ || !hasBasePoint_) return;
        
        engine.saveState();
        
        double dx = destPoint_.x - basePoint_.x;
        double dy = destPoint_.y - basePoint_.y;
        
        // Calcular límites de la ventana
        double minX = std::min(windowP1_.x, windowP2_.x);
        double maxX = std::max(windowP1_.x, windowP2_.x);
        double minY = std::min(windowP1_.y, windowP2_.y);
        double maxY = std::max(windowP1_.y, windowP2_.y);
        
        int movedVertices = 0;
        
        // Mover solo los vértices que están dentro de la ventana de cruce
        for (Entity* e : selectedEntities_) {
            auto grips = e->getGripPoints();
            for (size_t i = 0; i < grips.size(); ++i) {
                if (grips[i].x >= minX && grips[i].x <= maxX &&
                    grips[i].y >= minY && grips[i].y <= maxY) {
                    e->moveGrip(i, {grips[i].x + dx, grips[i].y + dy});
                    movedVertices++;
                }
            }
        }
        
        statusMessage_ = "STRETCH | " + std::to_string(movedVertices) + " vértices estirados.";
    }

    void StretchCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "STRETCH | Comando cancelado.";
    }

    std::string StretchCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool StretchCommand::isComplete() const {
        return finished_;
    }

    void StretchCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. FEEDBACK DURANTE LA SELECCIÓN DE LA VENTANA (Crossing Verde)
        if (step_ == Step::SelectingWindowP2 && hasWindowP1_) {
            sf::Color windowColor(0, 255, 0, 100);
            sf::Color outlineColor(0, 255, 0, 200);
            
            double x1 = windowP1_.x;
            double y1 = windowP1_.y;
            double x2 = mouseWorldPos.x; // Usamos el ratón como P2 dinámico
            double y2 = mouseWorldPos.y;

            double minX = std::min(x1, x2);
            double maxX = std::max(x1, x2);
            double minY = std::min(y1, y2);
            double maxY = std::max(y1, y2);

            sf::RectangleShape windowRect;
            windowRect.setSize(sf::Vector2f(
                static_cast<float>((maxX - minX) * view.getScale()),
                static_cast<float>((maxY - minY) * view.getScale())
            ));
            windowRect.setFillColor(windowColor);
            windowRect.setOutlineColor(outlineColor);
            windowRect.setOutlineThickness(1.0f);
            
            // SFML dibuja desde arriba-izquierda. En coordenadas de mundo, maxY es la parte "superior" (menor Y en pantalla)
            windowRect.setPosition(w2s(minX, maxY)); 
            window.draw(windowRect);
        }

        // 2. FEEDBACK DURANTE EL ESTIRAMIENTO (WaitingDestPoint)
        else if (step_ == Step::WaitingDestPoint && hasBasePoint_ && !selectedEntities_.empty()) {
            sf::Color originalColor(255, 165, 0, 180); // Naranja (originales)
            sf::Color ghostColor(0, 200, 255, 120);    // Cian (fantasmas estirados)
            
            double dx = mouseWorldPos.x - basePoint_.x;
            double dy = mouseWorldPos.y - basePoint_.y;

            // A) Dibujar entidades originales
            for (Entity* e : selectedEntities_) {
                e->draw(window, w2s, originalColor, view.getScale());
            }

            // B) Dibujar fantasmas estirados
            for (Entity* e : selectedEntities_) {
                auto ghost = e->clone();
                
                // Lógica de estiramiento para el preview: mover solo los puntos dentro de la ventana
                if (auto* line = dynamic_cast<Line*>(ghost.get())) {
                    double minX = std::min(windowP1_.x, windowP2_.x);
                    double maxX = std::max(windowP1_.x, windowP2_.x);
                    double minY = std::min(windowP1_.y, windowP2_.y);
                    double maxY = std::max(windowP1_.y, windowP2_.y);

                    auto isInside = [&](const Point2D& p) {
                        return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
                    };

                    if (isInside(line->p1)) {
                        line->p1.x += dx; line->p1.y += dy;
                    }
                    if (isInside(line->p2)) {
                        line->p2.x += dx; line->p2.y += dy;
                    }
                }
                // >>> AQUÍ PUEDES AÑADIR 'else if' para Arc, Circle, Polyline si tu motor los soporta en Stretch <<<
                
                ghost->draw(window, w2s, ghostColor, view.getScale());
            }
            
            // C) Línea guía desde el punto base al ratón
            sf::Vertex guideLine[] = {
                sf::Vertex(w2s(basePoint_.x, basePoint_.y), sf::Color(255, 255, 0, 200)),
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), sf::Color(255, 255, 0, 200))
            };
            window.draw(guideLine, 2, sf::Lines);
        }
    }

} // namespace cad