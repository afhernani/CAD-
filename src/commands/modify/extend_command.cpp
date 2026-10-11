#include "cad/commands/modify/extend_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include <cmath>
#include <algorithm>
#include <limits>

namespace cad {
    namespace {
        constexpr double PI = 3.14159265358979323846;
        bool isAngleBetween(double angle, double start, double end) {
            auto norm = [](double a) { a = std::fmod(a, 2.0 * PI); if (a < 0) a += 2.0 * PI; return a; };
            angle = norm(angle); start = norm(start); end = norm(end);
            if (start <= end) return angle >= start && angle <= end;
            return angle >= start || angle <= end;
        }
    }

    ExtendCommand::ExtendCommand() {
        step_ = Step::SelectingBoundaries; selectingBoundaries_ = true; finished_ = false;
        statusMessage_ = "EXTEND | Seleccionar bordes (Enter para terminar):";
    }

    void ExtendCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input); double x, y; char comma;
        if (iss >> x >> comma >> y && comma == ',') { onPoint({x, y}, engine); return; }
        if (input.empty()) {
            if (step_ == Step::SelectingBoundaries) {
                if (boundaries_.empty()) { statusMessage_ = "EXTEND | No hay bordes. Cancelado."; finished_ = true; }
                else { step_ = Step::SelectingEntitiesToExtend; selectingBoundaries_ = false; statusMessage_ = "EXTEND | Seleccionar entidades a alargar:"; }
            } else { finished_ = true; statusMessage_ = "EXTEND | Comando terminado."; }
            return;
        }
        statusMessage_ = "EXTEND | Formato no válido.";
    }

    void ExtendCommand::onPoint(const Point2D& point, Engine& engine) {
        double tolerance = 10.0 / engine.viewScale;
        if (step_ == Step::SelectingBoundaries) {
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (std::find(boundaries_.begin(), boundaries_.end(), entity.get()) == boundaries_.end()) boundaries_.push_back(entity.get());
                    statusMessage_ = "EXTEND | Borde añadido. Enter para terminar:";
                    return;
                }
            }
        } else {
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) { extendEntity(entity.get(), point, engine); return; }
            }
        }
    }

    void ExtendCommand::extendEntity(Entity* entity, const Point2D& clickPoint, Engine& engine) {
        if (auto* line = dynamic_cast<Line*>(entity)) {
            Point2D closestBorder; double minDist = std::numeric_limits<double>::max();
            for (Entity* boundary : boundaries_) {
                if (auto* bLine = dynamic_cast<Line*>(boundary)) {
                    auto inter = lineLineIntersection(line->p1, line->p2, bLine->p1, bLine->p2);
                    if (inter.intersects) {
                        double dist = std::hypot(inter.point.x - clickPoint.x, inter.point.y - clickPoint.y);
                        if (dist < minDist) { minDist = dist; closestBorder = inter.point; }
                    }
                } else if (auto* bCircle = dynamic_cast<Circle*>(boundary)) {
                    auto pts = lineCircleIntersection(line->p1, line->p2, bCircle->center, bCircle->radius);
                    for (const auto& p : pts) {
                        double dist = std::hypot(p.x - clickPoint.x, p.y - clickPoint.y);
                        if (dist < minDist) { minDist = dist; closestBorder = p; }
                    }
                }
            }
            if (minDist < std::numeric_limits<double>::max()) {
                engine.saveState();
                if (std::hypot(clickPoint.x - line->p1.x, clickPoint.y - line->p1.y) < std::hypot(clickPoint.x - line->p2.x, clickPoint.y - line->p2.y)) line->p1 = closestBorder;
                else line->p2 = closestBorder;
                statusMessage_ = "EXTEND | Línea alargada.";
            } else { statusMessage_ = "EXTEND | Sin intersección."; }
        }
        else if (auto* arc = dynamic_cast<Arc*>(entity)) {
            std::vector<Point2D> allIntersections;
            for (Entity* boundary : boundaries_) {
                if (auto* bLine = dynamic_cast<Line*>(boundary)) {
                    auto pts = lineCircleIntersection(bLine->p1, bLine->p2, arc->center, arc->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                } else if (auto* bCircle = dynamic_cast<Circle*>(boundary)) {
                    auto pts = circleCircleIntersection(arc->center, arc->radius, bCircle->center, bCircle->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                }
            }
            if (!allIntersections.empty()) {
                Point2D closest = allIntersections[0]; double minDist = std::hypot(closest.x - clickPoint.x, closest.y - clickPoint.y);
                for (const auto& p : allIntersections) {
                    double dist = std::hypot(p.x - clickPoint.x, p.y - clickPoint.y);
                    if (dist < minDist) { minDist = dist; closest = p; }
                }
                double intAngle = std::atan2(closest.y - arc->center.y, closest.x - arc->center.x);
                double clickAngle = std::atan2(clickPoint.y - arc->center.y, clickPoint.x - arc->center.x);
                double startA = arc->startAngle * PI / 180.0;
                double endA = arc->endAngle * PI / 180.0;
                
                engine.saveState();
                // Alargar significa mover el extremo MÁS CERCANO al clickPoint hacia la intersección
                double distToStart = std::hypot(arc->center.x + arc->radius*std::cos(startA) - clickPoint.x, arc->center.y + arc->radius*std::sin(startA) - clickPoint.y);
                double distToEnd = std::hypot(arc->center.x + arc->radius*std::cos(endA) - clickPoint.x, arc->center.y + arc->radius*std::sin(endA) - clickPoint.y);
                
                if (distToStart < distToEnd) arc->startAngle = intAngle * 180.0 / PI;
                else arc->endAngle = intAngle * 180.0 / PI;
                statusMessage_ = "EXTEND | Arco alargado.";
            } else { statusMessage_ = "EXTEND | Sin intersección."; }
        }
        else {
            statusMessage_ = "EXTEND | Entidad no soportada para alargar.";
        }
    }

    void ExtendCommand::onCancel() { finished_ = true; statusMessage_ = "EXTEND | Cancelado."; }
    std::string ExtendCommand::getStatusMessage() const { return statusMessage_; }
    bool ExtendCommand::isComplete() const { return finished_; }

    void ExtendCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        if (boundaries_.empty()) return;
        sf::Color boundaryColor(0, 255, 0, 150);
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };
        for (Entity* boundary : boundaries_) {
            if (auto* line = dynamic_cast<Line*>(boundary)) {
                sf::Vertex lineVerts[] = { sf::Vertex(w2s(line->p1.x, line->p1.y), boundaryColor), sf::Vertex(w2s(line->p2.x, line->p2.y), boundaryColor) };
                window.draw(lineVerts, 2, sf::Lines);
            }
        }
    }
} // namespace cad