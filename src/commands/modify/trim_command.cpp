#include "cad/commands/modify/trim_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/render/view.hpp"                 // OBLIGATORIO
#include <SFML/Graphics.hpp>                   // OBLIGATORIO
#include "cad/core/constants.hpp"              // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>
#include <limits>
#include <algorithm>

namespace cad {

    TrimCommand::TrimCommand() {
        // --- INICIALIZACIÓN EXPLÍCITA ---
        step_ = Step::SelectingBoundaries;
        selectingBoundaries_ = true;
        finished_ = false;
        boundaries_.clear();
        statusMessage_ = "TRIM | Seleccionar cortes (Enter para terminar):";
    }

    void TrimCommand::execute(const std::string& input, Engine& engine) {
        // 1. Primero intentar parsear como coordenada "x,y" (clic del ratón)
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Si el input está vacío (Enter), cambiar de fase
        if (input.empty()) {
            if (step_ == Step::SelectingBoundaries) {
                if (boundaries_.empty()) {
                    statusMessage_ = "TRIM | No hay cortes seleccionados. Comando cancelado.";
                    finished_ = true;
                } else {
                    step_ = Step::SelectingEntitiesToTrim;
                    selectingBoundaries_ = false;
                    statusMessage_ = "TRIM | Seleccionar entidades a recortar (clic sobre ellas):";
                }
            }
            else if (step_ == Step::SelectingEntitiesToTrim) {
                // Enter en fase de recortar = terminar comando
                finished_ = true;
                statusMessage_ = "TRIM | Comando terminado.";
            }
            return;
        }

        statusMessage_ = "TRIM | Formato no válido. Usa clic o Enter.";
    }

    void TrimCommand::onPoint(const Point2D& point, Engine& engine) {
        double tolerance = 10.0 / engine.viewScale;

        if (step_ == Step::SelectingBoundaries) {
            // Buscar entidad cercana para añadir como corte
            Entity* found = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    // Solo permitir líneas como cortes (por ahora)
                    if (dynamic_cast<Line*>(entity.get())) {
                        found = entity.get();
                        break;
                    }
                }
            }

            if (found) {
                // Evitar duplicados
                if (std::find(boundaries_.begin(), boundaries_.end(), found) == boundaries_.end()) {
                    boundaries_.push_back(found);
                }
                statusMessage_ = "TRIM | Corte añadido (" + 
                            std::to_string(boundaries_.size()) + 
                            " cortes). Enter para terminar:";
            } else {
                statusMessage_ = "TRIM | No se encontró entidad válida. Intenta de nuevo:";
            }
        }
        else if (step_ == Step::SelectingEntitiesToTrim) {
            // Buscar entidad a recortar
            Entity* toTrim = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get())) {
                        toTrim = entity.get();
                        break;
                    }
                }
            }

            if (toTrim) {
                trimEntity(toTrim, point, engine);
            } else {
                statusMessage_ = "TRIM | No se encontró entidad a recortar. Intenta de nuevo:";
            }
        }
    }

    void TrimCommand::trimEntity(Entity* entity, const Point2D& clickPoint, Engine& engine) {
        auto* line = dynamic_cast<Line*>(entity);
        if (!line) return;

        // Encontrar la intersección con la línea de corte más cercana al punto de clic
        Point2D closestCut;
        double minDist = std::numeric_limits<double>::max();

        for (Entity* boundary : boundaries_) {
            auto* boundaryLine = dynamic_cast<Line*>(boundary);
            if (!boundaryLine) continue;

            auto inter = lineLineIntersection(line->p1, line->p2,
                                            boundaryLine->p1, boundaryLine->p2);
            if (inter.intersects && inter.param >= 0 && inter.param <= 1) {
                double dist = std::hypot(inter.point.x - clickPoint.x,
                                        inter.point.y - clickPoint.y);
                if (dist < minDist) {
                    minDist = dist;
                    closestCut = inter.point;
                }
            }
        }

        if (minDist < std::numeric_limits<double>::max()) {
            // Guardar estado para Undo
            engine.saveState();

            // Determinar qué extremo está más cerca del punto de clic
            double distToP1 = std::hypot(clickPoint.x - line->p1.x, clickPoint.y - line->p1.y);
            double distToP2 = std::hypot(clickPoint.x - line->p2.x, clickPoint.y - line->p2.y);
            
            // Mantener el extremo más cercano al clic, recortar el otro
            bool keepP1 = (distToP1 < distToP2);
            
            if (keepP1) {
                // Mantener p1, recortar desde closestCut hasta p2
                line->p2 = closestCut;
            } else {
                // Mantener p2, recortar desde p1 hasta closestCut
                line->p1 = closestCut;
            }
            
            statusMessage_ = "TRIM | Entidad recortada. Seleccionar otra o Enter para terminar:";
        } else {
            statusMessage_ = "TRIM | No hay intersección con los cortes. Intenta de nuevo:";
        }
    }

    void TrimCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "TRIM | Comando cancelado.";
    }

    std::string TrimCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool TrimCommand::isComplete() const {
        return finished_;
    }

    void TrimCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                   const Point2D& mouseWorldPos, sf::Font& font) const {
        if (boundaries_.empty()) return;

        sf::Color boundaryColor(0, 255, 0, 150); // Verde translúcido

        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // Dibujar todas las entidades seleccionadas como bordes de corte
        for (Entity* boundary : boundaries_) {
            if (auto* line = dynamic_cast<Line*>(boundary)) {
                sf::Vertex lineVerts[] = {
                    sf::Vertex(w2s(line->p1.x, line->p1.y), boundaryColor),
                    sf::Vertex(w2s(line->p2.x, line->p2.y), boundaryColor)
                };
                window.draw(lineVerts, 2, sf::Lines);
            }
            // Si en el futuro soportas Arcos o Polilíneas como bordes, 
            // podrías añadir aquí los bloques else if correspondientes.
        }
    }

} // namespace cad