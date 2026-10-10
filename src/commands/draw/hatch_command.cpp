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
#include <iostream>

namespace cad {

    namespace{
        constexpr double PI = 3.14159265358979323846;
    }

    HatchCommand::HatchCommand() {
        statusMessage_ = "SOMBREADO | Seleccionar borde exterior (clic SOBRE la línea):";
        finished_ = false;
        step_ = Step::SelectingOuter;
    }

    void HatchCommand::execute(const std::string& input, Engine& engine) {
        std::string upper = input;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (step_ == Step::SelectingOuter || step_ == Step::SelectingIslands) {
            std::istringstream iss(input);
            double x, y;
            char comma;
            if (iss >> x >> comma >> y && comma == ',') {
                onPoint({x, y}, engine);
                return;
            }
        }

        if (upper == "C" || upper == "S" || upper == "CANCELAR" || upper == "EXIT") {
            finished_ = true;
            statusMessage_ = "SOMBREADO | Cancelado.";
            return;
        }

        // En SelectingIslands, ENTER termina la selección de islas
        if (step_ == Step::SelectingIslands && input.empty()) {
            if (selectedEntities_.empty()) {
                statusMessage_ = "SOMBREADO | No hay borde exterior. Comando cancelado.";
                finished_ = true;
            } else {
                step_ = Step::DefiningAngle;
                statusMessage_ = "SOMBREADO | Especificar ángulo de rayado <0>:";
            }
            return;
        }

        if (step_ == Step::SelectingOuter) {
            statusMessage_ = "SOMBREADO | Haz clic SOBRE el borde exterior de la entidad cerrada.";
            return;
        }

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
            hatch->id = Entity::generateId();
            hatch->pattern = HatchPattern::DIAGONAL;
            hatch->angle = angle_;
            hatch->spacing = spacing_;
            hatch->layerName = engine.doc.currentLayerName;
            
            if (!selectedEntities_.empty()) {
                // Primera entidad = borde exterior
                Entity* outer = selectedEntities_[0];
                hatch->boundaryId = outer->id;
                hatch->points = extractPointsFromEntity(outer);
                
                // Resto = islas
                for (size_t i = 1; i < selectedEntities_.size(); ++i) {
                    Entity* island = selectedEntities_[i];
                    hatch->islandIds.push_back(island->id);
                    hatch->islands.push_back(extractPointsFromEntity(island));
                }
            }

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
            if (polygon->points.size() >= 3) pts = polygon->points;
        }
        else if (auto* circle = dynamic_cast<Circle*>(entity)) {
            int segments = 32;
            double angleStep = 2 * PI / segments;
            for (int i = 0; i < segments; ++i) {
                double angle = i * angleStep;
                pts.push_back({circle->center.x + circle->radius * std::cos(angle),
                               circle->center.y + circle->radius * std::sin(angle)});
            }
        }
        return pts;
    }

    void HatchCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ != Step::SelectingOuter && step_ != Step::SelectingIslands) return;

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
                else if (dynamic_cast<Polygon*>(entity.get())) {
                    targetEntity = entity.get();
                    break;
                }
                else if (dynamic_cast<Circle*>(entity.get())) {
                    targetEntity = entity.get();
                    break;
                }
            }
        }

        if (targetEntity) {
            if (step_ == Step::SelectingOuter) {
                selectedEntities_.push_back(targetEntity);
                step_ = Step::SelectingIslands;
                statusMessage_ = "SOMBREADO | Selecciona islas (agujeros) o pulsa ENTER para terminar:";
            }
            else if (step_ == Step::SelectingIslands) {
                // Evitar duplicados y evitar que sea la misma que el exterior
                bool alreadySelected = false;
                for (Entity* e : selectedEntities_) {
                    if (e == targetEntity) {
                        alreadySelected = true;
                        break;
                    }
                }
                if (!alreadySelected) {
                    selectedEntities_.push_back(targetEntity);
                    statusMessage_ = "SOMBREADO | Isla añadida. Selecciona más o pulsa ENTER para terminar:";
                } else {
                    statusMessage_ = "SOMBREADO | Esa entidad ya está seleccionada.";
                }
            }
        } else {
            statusMessage_ = "SOMBREADO | No se encontró entidad válida. Intenta de nuevo:";
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
        if (selectedEntities_.empty()) return;

        sf::Color outerColor(0, 255, 0, 200); // Verde para borde exterior
        sf::Color islandColor(255, 165, 0, 200); // Naranja para islas

        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // Dibujar borde exterior
        if (!selectedEntities_.empty()) {
            Entity* outer = selectedEntities_[0];
            if (auto* poly = dynamic_cast<Polyline*>(outer)) {
                sf::VertexArray lines(sf::LineStrip, poly->points.size() + 1);
                for (size_t i = 0; i < poly->points.size(); ++i) {
                    lines[i].position = w2s(poly->points[i].x, poly->points[i].y);
                    lines[i].color = outerColor;
                }
                lines[poly->points.size()] = lines[0];
                window.draw(lines);
            }
            else if (auto* polygon = dynamic_cast<Polygon*>(outer)) {
                sf::ConvexShape shape;
                shape.setPointCount(polygon->points.size());
                for (size_t i = 0; i < polygon->points.size(); ++i) {
                    shape.setPoint(i, w2s(polygon->points[i].x, polygon->points[i].y));
                }
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(outerColor);
                shape.setOutlineThickness(3.0f);
                window.draw(shape);
            }
            else if (auto* circle = dynamic_cast<Circle*>(outer)) {
                sf::CircleShape shape(circle->radius * view.getScale());
                shape.setOrigin(circle->radius * view.getScale(), circle->radius * view.getScale());
                shape.setPosition(w2s(circle->center.x, circle->center.y));
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(outerColor);
                shape.setOutlineThickness(3.0f);
                window.draw(shape);
            }
        }

        // Dibujar islas
        for (size_t i = 1; i < selectedEntities_.size(); ++i) {
            Entity* island = selectedEntities_[i];
            if (auto* poly = dynamic_cast<Polyline*>(island)) {
                sf::VertexArray lines(sf::LineStrip, poly->points.size() + 1);
                for (size_t j = 0; j < poly->points.size(); ++j) {
                    lines[j].position = w2s(poly->points[j].x, poly->points[j].y);
                    lines[j].color = islandColor;
                }
                lines[poly->points.size()] = lines[0];
                window.draw(lines);
            }
            else if (auto* polygon = dynamic_cast<Polygon*>(island)) {
                sf::ConvexShape shape;
                shape.setPointCount(polygon->points.size());
                for (size_t j = 0; j < polygon->points.size(); ++j) {
                    shape.setPoint(j, w2s(polygon->points[j].x, polygon->points[j].y));
                }
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(islandColor);
                shape.setOutlineThickness(3.0f);
                window.draw(shape);
            }
            else if (auto* circle = dynamic_cast<Circle*>(island)) {
                sf::CircleShape shape(circle->radius * view.getScale());
                shape.setOrigin(circle->radius * view.getScale(), circle->radius * view.getScale());
                shape.setPosition(w2s(circle->center.x, circle->center.y));
                shape.setFillColor(sf::Color::Transparent);
                shape.setOutlineColor(islandColor);
                shape.setOutlineThickness(3.0f);
                window.draw(shape);
            }
        }

        // Preview del sombreado
        if ((step_ == Step::DefiningAngle || step_ == Step::DefiningSpacing) && !selectedEntities_.empty()) {
            Hatch tempHatch;
            tempHatch.points = extractPointsFromEntity(selectedEntities_[0]);
            tempHatch.pattern = HatchPattern::DIAGONAL;
            tempHatch.angle = angle_;
            tempHatch.spacing = spacing_;
            
            for (size_t i = 1; i < selectedEntities_.size(); ++i) {
                tempHatch.islands.push_back(extractPointsFromEntity(selectedEntities_[i]));
            }
            
            sf::Color previewColor(255, 165, 0, 180);
            tempHatch.draw(window, w2s, previewColor, view.getScale());
        }
    }

} // namespace cad