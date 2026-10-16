#include "cad/commands/modify/trim_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/arc.hpp"      // ✅ AÑADIDO
#include "cad/core/geometry/entities/circle.hpp"   // ✅ AÑADIDO
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include <sstream>
#include <cmath>
#include <limits>
#include <algorithm>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
        
        // Helper para verificar si un ángulo está dentro del rango del arco
        bool isAngleBetween(double angle, double start, double end) {
            auto norm = [](double a) { 
                a = std::fmod(a, 2.0 * PI); 
                if (a < 0) a += 2.0 * PI; 
                return a; 
            };
            angle = norm(angle); start = norm(start); end = norm(end);
            if (start <= end) return angle >= start && angle <= end;
            return angle >= start || angle <= end;
        }
    }

    TrimCommand::TrimCommand() {
        step_ = Step::SelectingBoundaries;
        selectingBoundaries_ = true;
        finished_ = false;
        boundaries_.clear();
        statusMessage_ = "TRIM | Seleccionar cortes (Enter para terminar):";
    }

    void TrimCommand::execute(const std::string& input, Engine& engine) {
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
                    statusMessage_ = "TRIM | No hay cortes seleccionados. Comando cancelado.";
                    finished_ = true;
                } else {
                    step_ = Step::SelectingEntitiesToTrim;
                    selectingBoundaries_ = false;
                    statusMessage_ = "TRIM | Seleccionar entidades a recortar (clic sobre ellas):";
                }
            }
            else if (step_ == Step::SelectingEntitiesToTrim) {
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
            Entity* found = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    // Permitimos Líneas, Arcos y Círculos como bordes de corte
                    if (dynamic_cast<Line*>(entity.get()) || 
                        dynamic_cast<Arc*>(entity.get()) || 
                        dynamic_cast<Circle*>(entity.get())) {
                        found = entity.get();
                        break;
                    }
                }
            }

            if (found) {
                if (std::find(boundaries_.begin(), boundaries_.end(), found) == boundaries_.end()) {
                    boundaries_.push_back(found);
                }
                statusMessage_ = "TRIM | Corte añadido (" + std::to_string(boundaries_.size()) + "). Enter para terminar:";
            } else {
                statusMessage_ = "TRIM | No se encontró entidad válida. Intenta de nuevo:";
            }
        }
        else if (step_ == Step::SelectingEntitiesToTrim) {
            Entity* toTrim = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) || 
                        dynamic_cast<Arc*>(entity.get()) || 
                        dynamic_cast<Circle*>(entity.get())) {
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
        
        // ========================================================================
        // CASO 1: LÍNEA
        // ========================================================================
        if (auto* line = dynamic_cast<Line*>(entity)) {
            std::vector<IntersectionResult> validIntersections;
            for (Entity* boundary : boundaries_) {
                if (auto* bLine = dynamic_cast<Line*>(boundary)) {
                    // usamos interceccion de rectas infinitas para ser más tolerante
                    Point2D I;
                    // Asumimos que tienes getInfiniteLineIntersection disponible, si no, usa lineLineIntersection
                    // Aquí uso lineLineIntersection por compatibilidad con tu código base, pero idealmente usa la infinita.
                    auto res = lineLineIntersection(line->p1, line->p2, bLine->p1, bLine->p2);
                    if (res.intersects) validIntersections.push_back(res);
                } 
                else if (auto* bCircle = dynamic_cast<Circle*>(boundary)) {
                    auto pts = lineCircleIntersection(line->p1, line->p2, bCircle->center, bCircle->radius);
                    for (const auto& p : pts) {
                        double dx = line->p2.x - line->p1.x, dy = line->p2.y - line->p1.y;
                        double len2 = dx*dx + dy*dy;
                        if (len2 > 1e-10) {
                            double t = ((p.x - line->p1.x)*dx + (p.y - line->p1.y)*dy) / len2;
                            if (t >= 0.0 && t <= 1.0) validIntersections.push_back({true, p, t});
                        }
                    }
                } 
                else if (auto* bArc = dynamic_cast<Arc*>(boundary)) {
                    auto pts = lineCircleIntersection(line->p1, line->p2, bArc->center, bArc->radius);
                    for (const auto& p : pts) {
                        double dx = line->p2.x - line->p1.x, dy = line->p2.y - line->p1.y;
                        double len2 = dx*dx + dy*dy;
                        if (len2 > 1e-10) {
                            double t = ((p.x - line->p1.x)*dx + (p.y - line->p1.y)*dy) / len2;
                            if (t >= 0.0 && t <= 1.0) {
                                double angle = std::atan2(p.y - bArc->center.y, p.x - bArc->center.x);
                                if (isAngleBetween(angle, bArc->startAngle * PI / 180.0, bArc->endAngle * PI / 180.0)) {
                                    validIntersections.push_back({true, p, t});
                                }
                            }
                        }
                    }
                }
            }

            if (!validIntersections.empty()) {
                auto closest = validIntersections[0];
                double minDist = std::hypot(closest.point.x - clickPoint.x, closest.point.y - clickPoint.y);
                for (const auto& res : validIntersections) {
                    double dist = std::hypot(res.point.x - clickPoint.x, res.point.y - clickPoint.y);
                    if (dist < minDist) { minDist = dist; closest = res; }
                }
                engine.saveState();
                if (std::hypot(clickPoint.x - line->p1.x, clickPoint.y - line->p1.y) < std::hypot(clickPoint.x - line->p2.x, clickPoint.y - line->p2.y)) {
                    line->p1 = closest.point;
                } else {
                    line->p2 = closest.point;
                }
                engine.doc.isModified = true;
                statusMessage_ = "TRIM | Línea recortada.";
            } else {
                statusMessage_ = "TRIM | Sin intersecciones válidas.";
            }
        }
        
        // ========================================================================
        // CASO 2: ARCO
        // ========================================================================
        else if (auto* arc = dynamic_cast<Arc*>(entity)) {
            std::vector<Point2D> allIntersections;
            for (Entity* boundary : boundaries_) {
                if (auto* bLine = dynamic_cast<Line*>(boundary)) {
                    auto pts = lineCircleIntersection(bLine->p1, bLine->p2, arc->center, arc->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                } 
                else if (auto* bCircle = dynamic_cast<Circle*>(boundary)) {
                    auto pts = circleCircleIntersection(arc->center, arc->radius, bCircle->center, bCircle->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                } 
                else if (auto* bArc = dynamic_cast<Arc*>(boundary)) {
                    auto pts = circleCircleIntersection(arc->center, arc->radius, bArc->center, bArc->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                }
            }

            std::vector<Point2D> validIntersections;
            for (const auto& p : allIntersections) {
                double angle = std::atan2(p.y - arc->center.y, p.x - arc->center.x);
                if (isAngleBetween(angle, arc->startAngle * PI / 180.0, arc->endAngle * PI / 180.0)) {
                    validIntersections.push_back(p);
                }
            }

            if (!validIntersections.empty()) {
                Point2D closest = validIntersections[0];
                double minDist = std::hypot(closest.x - clickPoint.x, closest.y - clickPoint.y);
                for (const auto& p : validIntersections) {
                    double dist = std::hypot(p.x - clickPoint.x, p.y - clickPoint.y);
                    if (dist < minDist) { minDist = dist; closest = p; }
                }
                
                double intAngle = std::atan2(closest.y - arc->center.y, closest.x - arc->center.x);
                double clickAngle = std::atan2(clickPoint.y - arc->center.y, clickPoint.x - arc->center.x);
                double startA = arc->startAngle * PI / 180.0;
                double endA = arc->endAngle * PI / 180.0;
                
                engine.saveState();
                // Recortamos el extremo que esté en el mismo segmento que el punto de clic
                if (isAngleBetween(clickAngle, startA, intAngle)) {
                    arc->startAngle = intAngle * 180.0 / PI;
                } else {
                    arc->endAngle = intAngle * 180.0 / PI;
                }
                engine.doc.isModified = true;
                statusMessage_ = "TRIM | Arco recortado.";
            } else {
                statusMessage_ = "TRIM | Sin intersecciones válidas en el arco.";
            }
        }

        // ========================================================================
        // CASO 3: CÍRCULO (Se convierte en Arco)
        // ========================================================================
        else if (auto* circle = dynamic_cast<Circle*>(entity)) {
            std::vector<Point2D> allIntersections;
            for (Entity* boundary : boundaries_) {
                if (auto* bLine = dynamic_cast<Line*>(boundary)) {
                    auto pts = lineCircleIntersection(bLine->p1, bLine->p2, circle->center, circle->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                } 
                else if (auto* bCircle = dynamic_cast<Circle*>(boundary)) {
                    auto pts = circleCircleIntersection(circle->center, circle->radius, bCircle->center, bCircle->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                } 
                else if (auto* bArc = dynamic_cast<Arc*>(boundary)) {
                    auto pts = circleCircleIntersection(circle->center, circle->radius, bArc->center, bArc->radius);
                    allIntersections.insert(allIntersections.end(), pts.begin(), pts.end());
                }
            }

            // Eliminar duplicados (intersecciones muy cercanas)
            std::vector<Point2D> uniqueIntersections;
            for (const auto& p : allIntersections) {
                bool isUnique = true;
                for (const auto& up : uniqueIntersections) {
                    if (std::hypot(p.x - up.x, p.y - up.y) < 1e-5) { isUnique = false; break; }
                }
                if (isUnique) uniqueIntersections.push_back(p);
            }

            if (uniqueIntersections.size() >= 2) {
                // Ordenar intersecciones por ángulo
                std::vector<std::pair<double, Point2D>> anglePts;
                for (const auto& p : uniqueIntersections) {
                    anglePts.push_back({std::atan2(p.y - circle->center.y, p.x - circle->center.x), p});
                }
                std::sort(anglePts.begin(), anglePts.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

                double clickAngle = std::atan2(clickPoint.y - circle->center.y, clickPoint.x - circle->center.x);
                int segmentToRemove = -1;
                int n = anglePts.size();
                
                // Encontrar en qué segmento del círculo cayó el clic
                for (int i = 0; i < n; ++i) {
                    if (isAngleBetween(clickAngle, anglePts[i].first, anglePts[(i + 1) % n].first)) {
                        segmentToRemove = i; 
                        break;
                    }
                }

                if (segmentToRemove != -1) {
                    // El nuevo arco será todo el círculo EXCEPTO el segmento removido
                    double newStart = anglePts[(segmentToRemove + 1) % n].first;
                    double newEnd = anglePts[segmentToRemove].first;
                    
                    auto newArc = std::make_unique<Arc>();
                    newArc->center = circle->center;
                    newArc->radius = circle->radius;
                    newArc->startAngle = newStart * 180.0 / PI;
                    newArc->endAngle = newEnd * 180.0 / PI;
                    newArc->layerName = circle->layerName;
                    newArc->id = Entity::generateId();

                    engine.saveState();
                    auto& entities = engine.doc.entities;
                    
                    // 1. Eliminar el círculo original
                    entities.erase(std::remove_if(entities.begin(), entities.end(), 
                        [circle](const std::unique_ptr<Entity>& e) { return e.get() == circle; }), entities.end());
                    
                    // 2. Añadir el nuevo arco
                    entities.push_back(std::move(newArc));
                    
                    statusMessage_ = "TRIM | Círculo recortado y convertido en Arco.";
                    return;
                }
            }
            statusMessage_ = "TRIM | No se pudo recortar (se necesitan >= 2 intersecciones únicas).";
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
        // if (boundaries_.empty()) return;

        // sf::Color boundaryColor(0, 255, 0, 150); // Verde translúcido
        // auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // for (Entity* boundary : boundaries_) {
        //     if (auto* line = dynamic_cast<Line*>(boundary)) {
        //         sf::Vertex lineVerts[] = {
        //             sf::Vertex(w2s(line->p1.x, line->p1.y), boundaryColor),
        //             sf::Vertex(w2s(line->p2.x, line->p2.y), boundaryColor)
        //         };
        //         window.draw(lineVerts, 2, sf::Lines);
        //     }
        //     // Aquí podrías añadir el dibujo de Arcos/Círculos de corte si lo deseas en el futuro
        // }

        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // 1. Dibujar bordes de corte en VERDE (ya seleccionados)
        if (step_ == Step::SelectingBoundaries || step_ == Step::SelectingEntitiesToTrim) {
            sf::Color boundaryColor(0, 255, 0, 150);
            for (Entity* boundary : boundaries_) {
                if (auto* line = dynamic_cast<Line*>(boundary)) {
                    sf::Vertex lineVerts[] = {
                        sf::Vertex(w2s(line->p1.x, line->p1.y), boundaryColor),
                        sf::Vertex(w2s(line->p2.x, line->p2.y), boundaryColor)
                    };
                    window.draw(lineVerts, 2, sf::Lines);
                }
            }
        }

        // 2. MEJORA DE UX: Previsualizar qué se va a borrar
        if (step_ == Step::SelectingEntitiesToTrim && !boundaries_.empty()) {
            double tolerance = 10.0 / view.getScale();
            
            // entity es un unique_ptr, usamos entity.get() para comparaciones y casts
            for (auto& entity : engine.doc.entities) {
                if (std::find(boundaries_.begin(), boundaries_.end(), entity.get()) != boundaries_.end()) {
                    continue;
                }

                if (entity->isNear(mouseWorldPos, tolerance)) {
                    // dynamic_cast sobre entity.get(), no sobre entity
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        Point2D closestIntersect = line->p1; 
                        double minDist = std::numeric_limits<double>::max();
                        bool found = false;

                        for (Entity* boundary : boundaries_) {
                            if (auto* bLine = dynamic_cast<Line*>(boundary)) {
                                // ✅ CORRECCIÓN: 4 argumentos correctos (el compilador se confundía antes)
                                auto res = lineLineIntersection(line->p1, line->p2, bLine->p1, bLine->p2);
                                if (res.intersects) {
                                    double dist = std::hypot(res.point.x - mouseWorldPos.x, res.point.y - mouseWorldPos.y);
                                    if (dist < minDist) { 
                                        minDist = dist; 
                                        closestIntersect = res.point; 
                                        found = true; 
                                    }
                                }
                            }
                        }

                        if (found) {
                            sf::Color trimColor(255, 50, 50, 200); // Rojo para indicar "eliminación"
                            
                            // Decidir qué extremo de la línea está más cerca del clic para dibujar el segmento a borrar
                            Point2D endToTrim = (std::hypot(line->p1.x - mouseWorldPos.x, line->p1.y - mouseWorldPos.y) < 
                                                 std::hypot(line->p2.x - mouseWorldPos.x, line->p2.y - mouseWorldPos.y)) ? line->p1 : line->p2;
                            
                            sf::Vertex trimLine[] = {
                                sf::Vertex(w2s(endToTrim.x, endToTrim.y), trimColor),
                                sf::Vertex(w2s(closestIntersect.x, closestIntersect.y), trimColor)
                            };
                            window.draw(trimLine, 2, sf::Lines);
                            
                            // Dibujar un pequeño marcador en el punto de corte
                            sf::CircleShape marker(3.0f);
                            marker.setOrigin(3.0f, 3.0f);
                            marker.setPosition(w2s(closestIntersect.x, closestIntersect.y));
                            marker.setFillColor(trimColor);
                            window.draw(marker);
                        }
                    }
                    break; // Solo previsualizamos la entidad bajo el ratón
                }
            }
        }
    }

} // namespace cad