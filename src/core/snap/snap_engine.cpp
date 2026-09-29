#include "cad/core/snap/snap_engine.hpp"
#include "cad/core/geometry/entity.hpp"
#include "cad/core/geometry/entities/line.hpp" // Necesario para dynamic_cast a Line
#include "cad/core/geometry/intersections.hpp" // Para lineLineIntersection
#include <cmath>
#include <algorithm>

namespace cad {

SnapEngine::SnapResult SnapEngine::findSnap(const std::vector<std::unique_ptr<Entity>>& entities, 
                                            const Point2D& mousePos, 
                                            double viewScale) const {
    SnapResult result;
    
    // La tolerancia en unidades del mundo depende del zoom
    double tolerance = PIXEL_TOLERANCE / viewScale;
    double minDist = tolerance;

    // 1. Buscar snaps en puntos clave (Extremos, Centros, Puntos Medios)
    // Nota: Asumimos que Entity tiene un método getSnapPoints() que devuelve std::vector<Point2D>
    for (const auto& entity : entities) {
        // Opcional: Podrías filtrar por capa visible aquí si pasas el Document completo
        // if (!entity->isVisible()) continue; 

        auto snaps = entity->getSnapPoints();
        for (const auto& pt : snaps) {
            double dist = std::hypot(mousePos.x - pt.x, mousePos.y - pt.y);
            if (dist < minDist) {
                minDist = dist;
                result.point = pt;
                result.active = true;
                result.type = SnapType::ENDPOINT; // Por defecto, luego podrías refinar según el tipo de punto
            }
        }
    }

    // 2. Buscar Intersecciones entre entidades cercanas
    // Optimización: Solo calcular si ambas entidades están cerca del ratón
    for (size_t i = 0; i < entities.size(); ++i) {
        for (size_t j = i + 1; j < entities.size(); ++j) {
            const auto& e1 = entities[i];
            const auto& e2 = entities[j];

            if (e1->isNear(mousePos, tolerance * 2) && e2->isNear(mousePos, tolerance * 2)) {
                
                // Solo calculamos intersección si ambas son líneas (por ahora)
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
            }
        }
    }

    return result;
}

} // namespace cad