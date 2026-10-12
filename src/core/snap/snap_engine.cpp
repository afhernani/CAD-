#include "cad/core/snap/snap_engine.hpp"
#include "cad/core/geometry/entity.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/intersections.hpp"
#include <cmath>
#include <algorithm>

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
            angle = norm(angle); start = norm(start); end = norm(end);
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

        // 1. Buscar snaps en puntos clave (Extremos, Centros, Puntos Medios)
        for (const auto& entity : entities) {
            auto snaps = entity->getSnapPoints();
            for (const auto& pt : snaps) {
                double dist = std::hypot(mousePos.x - pt.x, mousePos.y - pt.y);
                if (dist < minDist) {
                    minDist = dist;
                    result.point = pt;
                    result.active = true;
                    // ✅ CORRECCIÓN: No sobrescribir con ENDPOINT ciegamente.
                    // Si la entidad es un Círculo o Arco, es probable que el primer punto sea el CENTRO.
                    if (dynamic_cast<Circle*>(entity.get()) || dynamic_cast<Arc*>(entity.get())) {
                        result.type = SnapType::CENTER;
                    } else {
                        result.type = SnapType::ENDPOINT; // Fallback razonable
                    }
                }
            }
        }

        // 2. Buscar Intersecciones entre entidades cercanas
        for (size_t i = 0; i < entities.size(); ++i) {
            for (size_t j = i + 1; j < entities.size(); ++j) {
                const auto& e1 = entities[i];
                const auto& e2 = entities[j];

                // Optimización: Solo calcular si ambas entidades están cerca del ratón
                if (e1->isNear(mousePos, tolerance * 2) && e2->isNear(mousePos, tolerance * 2)) {
                    
                    // --- CASO A: Línea - Línea ---
                    if (auto l1 = dynamic_cast<Line*>(e1.get())) {
                        if (auto l2 = dynamic_cast<Line*>(e2.get())) {
                            auto inter = lineLineIntersection(l1->p1, l1->p2, l2->p1, l2->p2);
                            if (inter.intersects) {
                                double dist = std::hypot(mousePos.x - inter.point.x, mousePos.y - inter.point.y);
                                if (dist < minDist) {
                                    minDist = dist;
                                    result.point = inter.point;
                                    result.active = true;
                                    result.type = SnapType::INTERSECTION;
                                }
                            }
                        }
                    }
                    
                    // --- CASO B: Línea - Círculo ---
                    if (auto l1 = dynamic_cast<Line*>(e1.get())) {
                        if (auto c2 = dynamic_cast<Circle*>(e2.get())) {
                            auto pts = lineCircleIntersection(l1->p1, l1->p2, c2->center, c2->radius);
                            for (const auto& pt : pts) {
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
                    
                    // --- CASO C: Círculo - Círculo ---
                    if (auto c1 = dynamic_cast<Circle*>(e1.get())) {
                        if (auto c2 = dynamic_cast<Circle*>(e2.get())) {
                            auto pts = circleCircleIntersection(c1->center, c1->radius, c2->center, c2->radius);
                            for (const auto& pt : pts) {
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

                    // --- CASO D: Línea - Arco (con validación de ángulo) ---
                    if (auto l1 = dynamic_cast<Line*>(e1.get())) {
                        if (auto a2 = dynamic_cast<Arc*>(e2.get())) {
                            auto pts = lineCircleIntersection(l1->p1, l1->p2, a2->center, a2->radius);
                            for (const auto& pt : pts) {
                                double angle = std::atan2(pt.y - a2->center.y, pt.x - a2->center.x);
                                if (isAngleBetween(angle, a2->startAngle * PI / 180.0, a2->endAngle * PI / 180.0)) {
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
                }
            }
        }

        return result;
    }

} // namespace cad