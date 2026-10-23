#include "cad/commands/modify/stretch_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace cad {

    StretchCommand::StretchCommand() {
        statusMessage_ = "STRETCH | Selecciona entidades con ventana de cruce (primer punto):";
    }

    // ========================================================================
    // Lógica real para seleccionar entidades dentro de la ventana
    // ========================================================================
    void StretchCommand::performWindowSelection(Engine& engine) {
        selectedEntities_.clear();
        
        double minX = std::min(windowP1_.x, windowP2_.x);
        double maxX = std::max(windowP1_.x, windowP2_.x);
        double minY = std::min(windowP1_.y, windowP2_.y);
        double maxY = std::max(windowP1_.y, windowP2_.y);

        auto isInside = [&](const Point2D& p) {
            return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
        };

        for (auto& entity : engine.doc.entities) {
            auto grips = entity->getGripPoints();
            for (const auto& grip : grips) {
                if (isInside(grip)) {
                    selectedEntities_.push_back(entity.get());
                    break; 
                }
            }
        }
    }

    void StretchCommand::execute(const std::string& input, Engine& engine) {
        if (input.empty()) {
            if (step_ == Step::WaitingDestPoint) {
                finished_ = true;
                statusMessage_ = "STRETCH | Comando terminado.";
                return;
            }
        }

        std::istringstream iss(input);
        double x, y; char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }
        statusMessage_ = "STRETCH | Formato no válido. Usa clic o Enter.";
    }

    // ✅ AQUÍ ESTÁ LA CLAVE: onPoint ahora gestiona TODOS los pasos
    void StretchCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::SelectingWindowP1) {
            windowP1_ = point;
            hasWindowP1_ = true;
            step_ = Step::SelectingWindowP2;
            statusMessage_ = "STRETCH | Segundo punto de la ventana:";
        }
        else if (step_ == Step::SelectingWindowP2) {
            windowP2_ = point;
            hasWindow_ = true;
            
            performWindowSelection(engine);
            
            if (selectedEntities_.empty()) {
                statusMessage_ = "STRETCH | Ninguna entidad en la ventana. Intenta de nuevo:";
                windowP1_ = {0,0}; windowP2_ = {0,0}; hasWindowP1_ = false; hasWindow_ = false;
                step_ = Step::SelectingWindowP1;
            } else {
                step_ = Step::WaitingBasePoint;
                statusMessage_ = "STRETCH | " + std::to_string(selectedEntities_.size()) + " entidades. Designa punto base:";
            }
        }
        else if (step_ == Step::WaitingBasePoint) {
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
        
        double minX = std::min(windowP1_.x, windowP2_.x);
        double maxX = std::max(windowP1_.x, windowP2_.x);
        double minY = std::min(windowP1_.y, windowP2_.y);
        double maxY = std::max(windowP1_.y, windowP2_.y);
        
        auto isInside = [&](const Point2D& p) {
            return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
        };

        int movedVertices = 0;
        for (Entity* e : selectedEntities_) {
            auto grips = e->getGripPoints();
            for (size_t i = 0; i < grips.size(); ++i) {
                if (isInside(grips[i])) {
                    e->moveGrip(static_cast<int>(i), {grips[i].x + dx, grips[i].y + dy});
                    movedVertices++;
                }
            }
        }
        
        engine.doc.isModified = true;
        statusMessage_ = "STRETCH | " + std::to_string(movedVertices) + " vértices estirados.";
    }

    void StretchCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "STRETCH | Comando cancelado.";
    }

    std::string StretchCommand::getStatusMessage() const { return statusMessage_; }
    bool StretchCommand::isComplete() const { return finished_; }

    // ✅ DIBUJO DEL RECTÁNGULO Y FANTASMAS
    void StretchCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // 1. RECTÁNGULO VERDE DE SELECCIÓN
        if (step_ == Step::SelectingWindowP2 && hasWindowP1_) {
            sf::Color windowColor(0, 255, 0, 50);
            sf::Color outlineColor(0, 255, 0, 200);
            
            double minX = std::min(windowP1_.x, mouseWorldPos.x);
            double maxX = std::max(windowP1_.x, mouseWorldPos.x);
            double minY = std::min(windowP1_.y, mouseWorldPos.y);
            double maxY = std::max(windowP1_.y, mouseWorldPos.y);

            sf::RectangleShape windowRect;
            windowRect.setSize(sf::Vector2f(
                static_cast<float>((maxX - minX) * view.getScale()),
                static_cast<float>((maxY - minY) * view.getScale())
            ));
            windowRect.setFillColor(windowColor);
            windowRect.setOutlineColor(outlineColor);
            windowRect.setOutlineThickness(1.0f);
            
            // En SFML, Y crece hacia abajo. maxY en mundo es la parte superior en pantalla.
            windowRect.setPosition(w2s(minX, maxY)); 
            window.draw(windowRect);
        }

        // 2. FANTASMAS CIAN DURANTE EL ESTIRAMIENTO
        else if (step_ == Step::WaitingDestPoint && hasBasePoint_ && !selectedEntities_.empty()) {
            sf::Color originalColor(255, 165, 0, 180); 
            sf::Color ghostColor(0, 200, 255, 150);    
            
            double dx = mouseWorldPos.x - basePoint_.x;
            double dy = mouseWorldPos.y - basePoint_.y;

            for (Entity* e : selectedEntities_) {
                e->draw(window, w2s, originalColor, view.getScale());
            }

            for (Entity* e : selectedEntities_) {
                auto ghost = e->clone();
                
                double minX = std::min(windowP1_.x, windowP2_.x);
                double maxX = std::max(windowP1_.x, windowP2_.x);
                double minY = std::min(windowP1_.y, windowP2_.y);
                double maxY = std::max(windowP1_.y, windowP2_.y);

                auto isInside = [&](const Point2D& p) {
                    return p.x >= minX && p.x <= maxX && p.y >= minY && p.y <= maxY;
                };

                if (auto* line = dynamic_cast<Line*>(ghost.get())) {
                    if (isInside(line->p1)) { line->p1.x += dx; line->p1.y += dy; }
                    if (isInside(line->p2)) { line->p2.x += dx; line->p2.y += dy; }
                }
                else if (auto* circle = dynamic_cast<Circle*>(ghost.get())) {
                    if (isInside(circle->center)) { 
                        circle->center.x += dx; circle->center.y += dy; 
                    }
                }
                
                ghost->draw(window, w2s, ghostColor, view.getScale());
            }
            
            // Línea guía amarilla
            sf::Vertex guideLine[] = {
                sf::Vertex(w2s(basePoint_.x, basePoint_.y), sf::Color(255, 255, 0, 200)),
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), sf::Color(255, 255, 0, 200))
            };
            window.draw(guideLine, 2, sf::Lines);
        }
    }

} // namespace cad