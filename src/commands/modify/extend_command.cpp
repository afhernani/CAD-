#include "cad/commands/modify/extend_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include <sstream>
#include <cmath>
#include <limits>
#include <algorithm>

namespace cad {

    ExtendCommand::ExtendCommand() {
        step_ = Step::SelectingBoundaries;
        selectingBoundaries_ = true;
        finished_ = false;
        boundaries_.clear();
        statusMessage_ = "EXTEND | Seleccionar bordes (Enter para terminar):";
    }

    void ExtendCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        if (input.empty()) {
            if (step_ == Step::SelectingBoundaries) {
                if (boundaries_.empty()) {
                    statusMessage_ = "EXTEND | No hay bordes. Comando cancelado.";
                    finished_ = true;
                } else {
                    step_ = Step::SelectingEntitiesToExtend;
                    selectingBoundaries_ = false;
                    statusMessage_ = "EXTEND | Seleccionar entidades a alargar:";
                }
            }
            else if (step_ == Step::SelectingEntitiesToExtend) {
                finished_ = true;
                statusMessage_ = "EXTEND | Comando terminado.";
            }
            return;
        }

        statusMessage_ = "EXTEND | Formato no válido.";
    }

    void ExtendCommand::onPoint(const Point2D& point, Engine& engine) {
        double tolerance = 10.0 / engine.viewScale;

        if (step_ == Step::SelectingBoundaries) {
            Entity* found = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get())) {
                        found = entity.get();
                        break;
                    }
                }
            }
            if (found) {
                if (std::find(boundaries_.begin(), boundaries_.end(), found) == boundaries_.end()) {
                    boundaries_.push_back(found);
                }
                statusMessage_ = "EXTEND | Borde añadido (" + 
                            std::to_string(boundaries_.size()) + "). Enter para terminar:";
            } else {
                statusMessage_ = "EXTEND | No se encontró entidad. Intenta de nuevo:";
            }
        }
        else if (step_ == Step::SelectingEntitiesToExtend) {
            Entity* toExtend = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get())) {
                        toExtend = entity.get();
                        break;
                    }
                }
            }
            if (toExtend) {
                extendEntity(toExtend, point, engine);
            } else {
                statusMessage_ = "EXTEND | No se encontró entidad a alargar.";
            }
        }
    }

    void ExtendCommand::extendEntity(Entity* entity, const Point2D& clickPoint, Engine& engine) {
        auto* line = dynamic_cast<Line*>(entity);
        if (!line) return;

        Point2D closestBorder;
        double minDist = std::numeric_limits<double>::max();

        for (Entity* boundary : boundaries_) {
            auto* boundaryLine = dynamic_cast<Line*>(boundary);
            if (!boundaryLine) continue;

            auto inter = lineLineIntersection(line->p1, line->p2,
                                            boundaryLine->p1, boundaryLine->p2);
            if (inter.intersects) {
                double dist = std::hypot(inter.point.x - clickPoint.x,
                                        inter.point.y - clickPoint.y);
                if (dist < minDist) {
                    minDist = dist;
                    closestBorder = inter.point;
                }
            }
        }

        if (minDist < std::numeric_limits<double>::max()) {
            engine.saveState();
            double distToP1 = std::hypot(clickPoint.x - line->p1.x, clickPoint.y - line->p1.y);
            double distToP2 = std::hypot(clickPoint.x - line->p2.x, clickPoint.y - line->p2.y);
            if (distToP1 < distToP2) {
                line->p1 = closestBorder;
            } else {
                line->p2 = closestBorder;
            }
            statusMessage_ = "EXTEND | Entidad alargada. Seleccionar otra o Enter para terminar:";
        } else {
            statusMessage_ = "EXTEND | No hay intersección con los bordes.";
        }
    }

    void ExtendCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "EXTEND | Comando cancelado.";
    }

    std::string ExtendCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool ExtendCommand::isComplete() const {
        return finished_;
    }

} // namespace cad