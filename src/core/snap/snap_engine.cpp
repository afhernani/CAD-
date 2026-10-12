#include "cad/core/snap/snap_engine.hpp"
#include "cad/core/geometry/entity.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/intersections.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;

        // Helper para verificar si un ángulo está dentro del rango del arco (antihorario)
        bool isAngleBetween(double angle, double start, double end) {
            auto norm = [](double a) { 
                a = std::fmod(a, 2.0 * PI); 
                if (a < 0) a += 2.0 * PI; 
                return a; 
            };
            angle = norm(angle); 
            start = norm(start); 
            end = norm(end);
            if (start <= end) return angle >= start && angle <= end;
            return angle >= start || angle <= end;
        }
    }

    SnapEngine::SnapResult SnapEngine::findSnap(const std::vector<std::unique_ptr<Entity>>& entities, 
                                                const Point2D& mousePos, 
                                                double viewScale) const {
        SnapResult result;
        double tolerance = PIXEL_TOLERANCE / viewScale;
        double minDist = tolerance;

        // ============================================================
        // 1. PUNTOS CLAVE (Extremos, Centros, Puntos Medios)
        // ============================================================
        for (const auto& entity : entities) {
            auto snaps = entity->getSnapPoints();
            for (const auto& pt : snaps) {
                double dist = std::hypot(mousePos.x - pt.x, mousePos.y - pt.y);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = pt;
                    result.active = true;

                    // >>> LÓGICA CORREGIDA PARA DETECTAR EL TIPO EXACTO <<<
                    bool isMidpoint = false;

                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        // Punto medio de una línea
                        Point2D mid = {(line->p1.x + line->p2.x) / 2.0, (line->p1.y + line->p2.y) / 2.0};
                        if (std::hypot(pt.x - mid.x, pt.y - mid.y) < 0.001) {
                            isMidpoint = true;
                        }
                        result.type = isMidpoint ? SnapType::MIDPOINT : SnapType::ENDPOINT;
                    } 
                    else if (auto* arc = dynamic_cast<Arc*>(entity.get())) {
                        // Centro del arco
                        if (std::hypot(pt.x - arc->center.x, pt.y - arc->center.y) < 0.001) {
                            result.type = SnapType::CENTER;
                        } else {
                            // Punto medio del arco (cálculo angular)
                            double sRad = arc->startAngle * PI / 180.0;
                            double eRad = arc->endAngle * PI / 180.0;
                            double diff = eRad - sRad;
                            while (diff < 0) diff += 2.0 * PI;
                            while (diff >= 2.0 * PI) diff -= 2.0 * PI;
                            double midRad = sRad + diff / 2.0;
                            Point2D mid = {
                                arc->center.x + arc->radius * std::cos(midRad), 
                                arc->center.y + arc->radius * std::sin(midRad)
                            };
                            
                            if (std::hypot(pt.x - mid.x, pt.y - mid.y) < 0.001) {
                                isMidpoint = true;
                            }
                            result.type = isMidpoint ? SnapType::MIDPOINT : SnapType::ENDPOINT;
                        }
                    } 
                    else if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                        // En círculos, el primer punto suele ser el centro
                        if (std::hypot(pt.x - circle->center.x, pt.y - circle->center.y) < 0.001) {
                            result.type = SnapType::CENTER;
                        } else {
                            result.type = SnapType::ENDPOINT;
                        }
                    } 
                    else {
                        result.type = SnapType::ENDPOINT;
                    }
                }
            }
        }

        // ============================================================
        // 2. INTERSECCIONES (Lógica simétrica y robusta)
        // ============================================================
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                const auto& e1 = entities[i];
                const auto& e2 = entities[j];

                // Usamos || (O) en lugar de && (Y) para ser más permisivos cerca del cruce
                if (e1->isNear(mousePos, tolerance * 5) || e2->isNear(mousePos, tolerance * 5)) {
                    
                    std::vector<Point2D> intersections;

                    // Helper para extraer intersecciones sin importar el orden (e1, e2) o (e2, e1)
                    auto getIntersections = [&](Entity* a, Entity* b) {
                        if (auto l_a = dynamic_cast<Line*>(a)) {
                            if (auto l_b = dynamic_cast<Line*>(b)) {
                                auto res = lineLineIntersection(l_a->p1, l_a->p2, l_b->p1, l_b->p2);
                                if (res.intersects) intersections.push_back(res.point);
                            } else if (auto c_b = dynamic_cast<Circle*>(b)) {
                                auto pts = lineCircleIntersection(l_a->p1, l_a->p2, c_b->center, c_b->radius);
                                intersections.insert(intersections.end(), pts.begin(), pts.end());
                            } else if (auto arc_b = dynamic_cast<Arc*>(b)) {
                                auto pts = lineCircleIntersection(l_a->p1, l_a->p2, arc_b->center, arc_b->radius);
                                for (const auto& pt : pts) {
                                    double angle = std::atan2(pt.y - arc_b->center.y, pt.x - arc_b->center.x);
                                    if (isAngleBetween(angle, arc_b->startAngle * PI / 180.0, arc_b->endAngle * PI / 180.0)) {
                                        intersections.push_back(pt);
                                    }
                                }
                            }
                        } else if (auto c_a = dynamic_cast<Circle*>(a)) {
                            if (auto c_b = dynamic_cast<Circle*>(b)) {
                                auto pts = circleCircleIntersection(c_a->center, c_a->radius, c_b->center, c_b->radius);
                                intersections.insert(intersections.end(), pts.begin(), pts.end());
                            } else if (auto l_b = dynamic_cast<Line*>(b)) {
                                auto pts = lineCircleIntersection(l_b->p1, l_b->p2, c_a->center, c_a->radius);
                                intersections.insert(intersections.end(), pts.begin(), pts.end());
                            } else if (auto arc_b = dynamic_cast<Arc*>(b)) {
                                auto pts = circleCircleIntersection(c_a->center, c_a->radius, arc_b->center, arc_b->radius);
                                for (const auto& pt : pts) {
                                    double angle = std::atan2(pt.y - arc_b->center.y, pt.x - arc_b->center.x);
                                    if (isAngleBetween(angle, arc_b->startAngle * PI / 180.0, arc_b->endAngle * PI / 180.0)) {
                                        intersections.push_back(pt);
                                    }
                                }
                            }
                        } else if (auto arc_a = dynamic_cast<Arc*>(a)) {
                            if (auto l_b = dynamic_cast<Line*>(b)) {
                                auto pts = lineCircleIntersection(l_b->p1, l_b->p2, arc_a->center, arc_a->radius);
                                for (const auto& pt : pts) {
                                    double angle = std::atan2(pt.y - arc_a->center.y, pt.x - arc_a->center.x);
                                    if (isAngleBetween(angle, arc_a->startAngle * PI / 180.0, arc_a->endAngle * PI / 180.0)) {
                                        intersections.push_back(pt);
                                    }
                                }
                            } else if (auto c_b = dynamic_cast<Circle*>(b)) {
                                auto pts = circleCircleIntersection(arc_a->center, arc_a->radius, c_b->center, c_b->radius);
                                for (const auto& pt : pts) {
                                    double angle = std::atan2(pt.y - arc_a->center.y, pt.x - arc_a->center.x);
                                    if (isAngleBetween(angle, arc_a->startAngle * PI / 180.0, arc_a->endAngle * PI / 180.0)) {
                                        intersections.push_back(pt);
                                    }
                                }
                            } else if (auto arc_b = dynamic_cast<Arc*>(b)) {
                                auto pts = circleCircleIntersection(arc_a->center, arc_a->radius, arc_b->center, arc_b->radius);
                                for (const auto& pt : pts) {
                                    double angleA = std::atan2(pt.y - arc_a->center.y, pt.x - arc_a->center.x);
                                    double angleB = std::atan2(pt.y - arc_b->center.y, pt.x - arc_b->center.x);
                                    if (isAngleBetween(angleA, arc_a->startAngle * PI / 180.0, arc_a->endAngle * PI / 180.0) &&
                                        isAngleBetween(angleB, arc_b->startAngle * PI / 180.0, arc_b->endAngle * PI / 180.0)) {
                                        intersections.push_back(pt);
                                    }
                                }
                            }
                        }
                    };

                    getIntersections(e1.get(), e2.get());

                    // Comprobar si alguna intersección calculada está cerca del ratón
                    for (const auto& pt : intersections) {
                        double dist = std::hypot(mousePos.x - pt.x, mousePos.y - pt.y);
                        if (dist < minDist) {
                            minDist = dist;
                            result.point = pt;
                            result.active = true;
                            result.type = SnapType::INTERSECTION;
                        }
                    }
                }
            }
        }

        // DEBUG (Puedes quitar esto cuando funcione)
        // if (result.active) {
        //     std::string typeStr = (result.type == SnapType::CENTER) ? "CENTRO" : 
        //                           (result.type == SnapType::INTERSECTION) ? "INTERSECCION" :
        //                           (result.type == SnapType::MIDPOINT) ? "PUNTO_MEDIO" : "EXTREMO";
        //     std::cout << "[SNAP] " << typeStr << " en (" << result.point.x << ", " << result.point.y << ")\n";
        // }

        return result;
    }

} // namespace cad