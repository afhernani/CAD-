#include "cad/commands/modify/offset_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    OffsetCommand::OffsetCommand() {
        statusMessage_ = "DESPLAZAR | Distancia (número) o dos puntos:";
    }

    void OffsetCommand::execute(const std::string& input, Engine& engine) {
        // 1. Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Intentar parsear como número (distancia)
        // Permitimos introducir la distancia por teclado en cualquiera de las dos fases de distancia
        if (step_ == Step::WaitingDistance || step_ == Step::WaitingDistancePoint2) {
            try {
                double value = std::stod(input);
                if (value >= 0) {
                    distance_ = value;
                    hasDistance_ = true;
                    step_ = Step::WaitingEntity;
                    statusMessage_ = "DESPLAZAR | Seleccionar entidad a desplazar:";
                    return; // Salimos temprano porque ya se resolvió
                }
            } catch (...) {
                // Si no es un número válido, ignoramos y dejamos que el mensaje de error final lo maneje
            }
        }
        
        statusMessage_ = "DESPLAZAR | Formato no válido. Usa un número o coordenadas x,y.";
    }

    void OffsetCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingDistance) {
            // Primer punto para calcular distancia
            firstPoint_ = point;
            step_ = Step::WaitingDistancePoint2; // <<< CORRECCIÓN CLAVE: Cambiamos el estado
            statusMessage_ = "DESPLAZAR | Segundo punto para definir distancia:";
        }
        else if (step_ == Step::WaitingDistancePoint2) {
            // Segundo punto: calcular distancia
            double dx = point.x - firstPoint_.x;
            double dy = point.y - firstPoint_.y;
            distance_ = std::sqrt(dx * dx + dy * dy);
            hasDistance_ = true;
            step_ = Step::WaitingEntity;
            statusMessage_ = "DESPLAZAR | Seleccionar entidad a desplazar:";
        }
        else if (step_ == Step::WaitingEntity) {
            // Buscar entidad cercana al punto
            double tolerance = 10.0 / engine.viewScale;
            Entity* found = nullptr;
            
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) ||
                        dynamic_cast<Circle*>(entity.get()) ||
                        dynamic_cast<Arc*>(entity.get())) {
                        found = entity.get();
                        break;
                    }
                }
            }
            
            if (found) {
                selectedEntity_ = found;
                step_ = Step::WaitingSide;
                statusMessage_ = "DESPLAZAR | Indicar lado del desplazamiento:";
            } else {
                statusMessage_ = "DESPLAZAR | No se encontró entidad válida. Intenta de nuevo:";
            }
        }
        else if (step_ == Step::WaitingSide) {
            // Crear entidad desplazada
            createOffsetEntity(selectedEntity_, point, engine);
            
            // Resetear para permitir múltiples desplazamientos
            selectedEntity_ = nullptr;
            step_ = Step::WaitingEntity;
            statusMessage_ = "DESPLAZAR | Seleccionar otra entidad o ESC para terminar:";
        }
    }

    void OffsetCommand::createOffsetEntity(Entity* entity, const Point2D& sidePoint, Engine& engine) {
        engine.saveState();
        
        if (auto* line = dynamic_cast<Line*>(entity)) {
            double dx = line->p2.x - line->p1.x;
            double dy = line->p2.y - line->p1.y;
            double len = std::sqrt(dx * dx + dy * dy);
            
            if (len > 0) {
                double nx = -dy / len;
                double ny = dx / len;
                
                double vx = sidePoint.x - line->p1.x;
                double vy = sidePoint.y - line->p1.y;
                double side = vx * nx + vy * ny;
                double sign = (side >= 0) ? 1.0 : -1.0;
                
                auto newLine = std::make_unique<Line>();
                newLine->p1 = {line->p1.x + nx * distance_ * sign, line->p1.y + ny * distance_ * sign};
                newLine->p2 = {line->p2.x + nx * distance_ * sign, line->p2.y + ny * distance_ * sign};
                newLine->layerName = engine.doc.currentLayerName;
                engine.doc.addEntity(std::move(newLine));
            }
        }
        else if (auto* circle = dynamic_cast<Circle*>(entity)) {
            auto newCircle = std::make_unique<Circle>();
            newCircle->center = circle->center;
            newCircle->radius = circle->radius + distance_;
            if (newCircle->radius < 0) newCircle->radius = std::abs(newCircle->radius);
            newCircle->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newCircle));
        }
        else if (auto* arc = dynamic_cast<Arc*>(entity)) {
            auto newArc = std::make_unique<Arc>();
            newArc->center = arc->center;
            newArc->radius = arc->radius + distance_;
            newArc->startAngle = arc->startAngle;
            newArc->endAngle = arc->endAngle;
            if (newArc->radius < 0) newArc->radius = std::abs(newArc->radius);
            newArc->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newArc));
        }
    }

    void OffsetCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string OffsetCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool OffsetCommand::isComplete() const {
        return finished_;
    }

    void OffsetCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        
        // Lambda para convertir coordenadas. 
        // NOTA: Si ya quitaste el parámetro CANVAS_HEIGHT de tu View::worldToScreen, 
        // cámbialo a: return view.worldToScreen(x, y);
        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. RESALTAR ENTIDAD BAJO EL CURSOR (cuando esperamos seleccionar)
        if (step_ == Step::WaitingEntity && hasDistance_) {
            double tolerance = 10.0 / view.getScale();
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(mouseWorldPos, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) ||
                        dynamic_cast<Circle*>(entity.get()) ||
                        dynamic_cast<Arc*>(entity.get())) {
                        // Dibujar la entidad en verde brillante para indicar que se puede seleccionar
                        entity->draw(window, w2s, sf::Color(0, 255, 0, 150), view.getScale());
                        break;
                    }
                }
            }
        }

        // 2. DIBUJAR PREVIEW DEL DESPLAZAMIENTO (cuando ya tenemos entidad seleccionada)
        if (hasDistance_ && selectedEntity_) {
            sf::Color previewColor(255, 255, 0, 150); // Amarillo translúcido
            Entity* entity = selectedEntity_;
            double distance = distance_;

            if (auto* line = dynamic_cast<Line*>(entity)) {
                double dx = line->p2.x - line->p1.x;
                double dy = line->p2.y - line->p1.y;
                double len = std::sqrt(dx * dx + dy * dy);
                if (len > 0) {
                    double nx = -dy / len;
                    double ny = dx / len;
                    
                    // >>> CORRECCIÓN CLAVE: Calcular el lado basado en la posición del ratón <<<
                    double vx = mouseWorldPos.x - line->p1.x;
                    double vy = mouseWorldPos.y - line->p1.y;
                    double side = vx * nx + vy * ny;
                    double sign = (side >= 0) ? 1.0 : -1.0;
                    
                    sf::Vertex linePreview[] = { 
                        sf::Vertex(w2s(line->p1.x + nx * distance * sign, line->p1.y + ny * distance * sign), previewColor),
                        sf::Vertex(w2s(line->p2.x + nx * distance * sign, line->p2.y + ny * distance * sign), previewColor) 
                    };
                    window.draw(linePreview, 2, sf::Lines);
                }
            }
            else if (auto* circle = dynamic_cast<Circle*>(entity)) {
                double newRadius = circle->radius + distance;
                if (newRadius < 0) newRadius = std::abs(newRadius);
                
                sf::CircleShape circlePreview(static_cast<float>(newRadius * view.getScale()));
                circlePreview.setFillColor(sf::Color::Transparent);
                circlePreview.setOutlineColor(previewColor);
                circlePreview.setOutlineThickness(1.5f);
                circlePreview.setOrigin(static_cast<float>(newRadius * view.getScale()),
                                       static_cast<float>(newRadius * view.getScale()));
                circlePreview.setPosition(w2s(circle->center.x, circle->center.y));
                window.draw(circlePreview);
            }
            else if (auto* arc = dynamic_cast<Arc*>(entity)) {
                double newRadius = arc->radius + distance;
                if (newRadius < 0) newRadius = std::abs(newRadius);
                
                const int numPoints = 64;
                sf::VertexArray va(sf::LineStrip, numPoints);
                double startRad = arc->startAngle * PI / 180.0;
                double endRad = arc->endAngle * PI / 180.0;
                double step = (endRad - startRad) / (numPoints - 1);
                
                for (int i = 0; i < numPoints; ++i) {
                    double angle = startRad + i * step;
                    va[i].position = w2s(arc->center.x + newRadius * std::cos(angle), arc->center.y + newRadius * std::sin(angle));
                    va[i].color = previewColor;
                }
                window.draw(va);
            }
        }
    }

} // namespace cad