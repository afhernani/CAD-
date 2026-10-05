#include "cad/commands/draw/hatch_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include <SFML/Graphics.hpp>
#include "cad/core/constants.hpp"
#include <cmath>
#include <sstream>
#include <algorithm>

namespace cad {

    HatchCommand::HatchCommand() {
        statusMessage_ = "SOMBREADO | Seleccionar objeto cerrado (haz clic SOBRE la línea del borde):";
    }

    void HatchCommand::execute(const std::string& input, Engine& engine) {
        std::string upper = input;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        // Cancelar en cualquier momento
        if (upper == "C" || upper == "S" || upper == "CANCELAR" || upper == "EXIT") {
            finished_ = true;
            statusMessage_ = "SOMBREADO | Cancelado.";
            return;
        }

        // Paso 2: Definir Ángulo
        if (step_ == Step::DefiningAngle) {
            if (input.empty()) {
                angle_ = 0.0;
            } else {
                try { 
                    angle_ = std::stod(input); 
                } catch (...) { 
                    statusMessage_ = "SOMBREADO | Ángulo no válido. Usa un número (grados)."; 
                    return; 
                }
            }
            step_ = Step::DefiningSpacing;
            statusMessage_ = "SOMBREADO | Especificar distancia entre líneas <1.0>:";
            return;
        }

        // Paso 3: Definir Espaciado y Crear
        if (step_ == Step::DefiningSpacing) {
            if (input.empty()) {
                spacing_ = 1.0;
            } else {
                try { 
                    spacing_ = std::stod(input); 
                } catch (...) { 
                    statusMessage_ = "SOMBREADO | Distancia no válida. Usa un número."; 
                    return; 
                }
            }

            // Crear la entidad final
            auto hatch = std::make_unique<Hatch>();
            hatch->points = hatchPoints_;
            hatch->pattern = HatchPattern::DIAGONAL;
            hatch->angle = angle_;
            hatch->spacing = spacing_;
            hatch->layerName = engine.doc.currentLayerName;
            
            engine.saveState();
            engine.doc.addEntity(std::move(hatch));
            
            statusMessage_ = "SOMBREADO | Objeto sombreado correctamente.";
            step_ = Step::Finished;
            finished_ = true;
            return;
        }
    }

    std::vector<Point2D> HatchCommand::extractPointsFromEntity(Entity* entity) const {
        std::vector<Point2D> pts;
        if (auto* poly = dynamic_cast<Polyline*>(entity)) {
            if (poly->closed && poly->points.size() >= 3) pts = poly->points;
        }
        else if (auto* polygon = dynamic_cast<Polygon*>(entity)) {
            int sides = polygon->sides;
            double angleStep = 2 * 3.14159265358979323846 / sides;
            double offset = polygon->rotationOffset * 3.14159265358979323846 / 180.0;
            for (int i = 0; i < sides; ++i) {
                double angle = i * angleStep + offset;
                pts.push_back({polygon->center.x + polygon->radius * std::cos(angle),
                               polygon->center.y + polygon->radius * std::sin(angle)});
            }
        }
        else if (auto* circle = dynamic_cast<Circle*>(entity)) {
            int segments = 32;
            double angleStep = 2 * 3.14159265358979323846 / segments;
            for (int i = 0; i < segments; ++i) {
                double angle = i * angleStep;
                pts.push_back({circle->center.x + circle->radius * std::cos(angle),
                               circle->center.y + circle->radius * std::sin(angle)});
            }
        }
        return pts;
    }

    void HatchCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ != Step::SelectingObject) return;

        double tolerance = 15.0 / engine.viewScale;
        Entity* targetEntity = nullptr;

        for (auto& entity : engine.doc.entities) {
            if (dynamic_cast<Hatch*>(entity.get())) continue;

            if (entity->isNear(point, tolerance)) {
                if (auto* poly = dynamic_cast<Polyline*>(entity.get())) {
                    if (poly->closed && poly->points.size() >= 3) {
                        targetEntity = entity.get();
                        break;
                    }
                }
                else if (dynamic_cast<Polygon*>(entity.get()) || dynamic_cast<Circle*>(entity.get())) {
                    targetEntity = entity.get();
                    break;
                }
            }
        }

        if (targetEntity) {
            hatchPoints_ = extractPointsFromEntity(targetEntity);
            if (!hatchPoints_.empty()) {
                selectedEntity_ = targetEntity;
                step_ = Step::DefiningAngle;
                statusMessage_ = "SOMBREADO | Objeto seleccionado. Especificar ángulo de rayado <0>:";
            } else {
                statusMessage_ = "SOMBREADO | El objeto no es válido para sombrear.";
            }
        } else {
            statusMessage_ = "SOMBREADO | No se ha seleccionado ningún objeto cerrado. Intenta de nuevo (o Esc para salir):";
        }
    }

    void HatchCommand::onCancel() { 
        finished_ = true; 
        statusMessage_ = "SOMBREADO | Cancelado."; 
    }
    
    std::string HatchCommand::getStatusMessage() const { 
        return statusMessage_; 
    }
    
    bool HatchCommand::isComplete() const { 
        return finished_; 
    }

    void HatchCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                    const Point2D& mouseWorldPos, sf::Font& font) const {
        // Feedback 1: Resaltar el objeto seleccionado en amarillo
        if (selectedEntity_ != nullptr && step_ != Step::Finished) {
            sf::Color highlightColor(255, 255, 0, 200); // Amarillo brillante
            
            if (auto* circle = dynamic_cast<Circle*>(selectedEntity_)) {
                sf::CircleShape shape(circle->radius * view.getScale());
                shape.setOrigin(circle->radius * view.getScale(), circle->radius * view.getScale());
                shape.setPosition(view.worldToScreen(circle->center.x, circle->center.y));
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(highlightColor);
                shape.setOutlineThickness(3.0f);
                window.draw(shape);
            }
            else if (auto* polygon = dynamic_cast<Polygon*>(selectedEntity_)) {
                sf::ConvexShape shape;
                shape.setPointCount(polygon->sides);
                for (int i = 0; i < polygon->sides; ++i) {
                    double angle = i * 2 * 3.14159265358979323846 / polygon->sides + 
                                   polygon->rotationOffset * 3.14159265358979323846 / 180.0;
                    shape.setPoint(i, view.worldToScreen(
                        polygon->center.x + polygon->radius * std::cos(angle),
                        polygon->center.y + polygon->radius * std::sin(angle)
                    ));
                }
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(highlightColor);
                shape.setOutlineThickness(3.0f);
                window.draw(shape);
            }
            else if (auto* poly = dynamic_cast<Polyline*>(selectedEntity_)) {
                sf::VertexArray lines(sf::LineStrip, poly->points.size() + 1);
                for (size_t i = 0; i < poly->points.size(); ++i) {
                    lines[i].position = view.worldToScreen(poly->points[i].x, poly->points[i].y);
                    lines[i].color = highlightColor;
                }
                lines[poly->points.size()] = lines[0];
                window.draw(lines);
            }
        }

        // Feedback 2: Preview del sombreado en tiempo real
        if ((step_ == Step::DefiningAngle || step_ == Step::DefiningSpacing) && !hatchPoints_.empty()) {
            Hatch tempHatch;
            tempHatch.points = hatchPoints_;
            tempHatch.pattern = HatchPattern::DIAGONAL;
            tempHatch.angle = angle_;
            tempHatch.spacing = spacing_;
            
            sf::Color previewColor(255, 165, 0, 180); // Naranja semitransparente
            auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };
            tempHatch.draw(window, w2s, previewColor, view.getScale());
        }
    }

} // namespace cad