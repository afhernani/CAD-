#include "cad/core/selection/selection_manager.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/arc.hpp"  
#include <algorithm>
#include <cmath>

namespace cad {

SelectionMode SelectionManager::determineMode(const Point2D& start, const Point2D& end) {
    if (start.x < end.x) return SelectionMode::WINDOW;
    if (start.x > end.x) return SelectionMode::CROSSING;
    return SelectionMode::SINGLE;
}

void SelectionManager::selectByWindow(const Point2D& p1, const Point2D& p2,
                                      const std::vector<std::unique_ptr<Entity>>& entities,
                                      std::vector<Entity*>& selectedEntities,
                                      bool addToSelection,
                                      const Document& doc) {
    double minX = std::min(p1.x, p2.x);
    double maxX = std::max(p1.x, p2.x);
    double minY = std::min(p1.y, p2.y);
    double maxY = std::max(p1.y, p2.y);

    SelectionMode mode = determineMode(p1, p2);

    if (!addToSelection) {
        selectedEntities.clear();
    }

    for (const auto& entity : entities) {
        // >>> FILTRO DE SEGURIDAD DE CAPAS <<<
        const Layer* layer = doc.getLayer(entity->layerName);
        if (!layer || !layer->visible || layer->frozen || layer->locked) {
            continue; // Ignorar entidades de capas no seleccionables
        }
        
        bool shouldSelect = false;

        if (mode == SelectionMode::WINDOW) {
            shouldSelect = isEntityCompletelyInside(entity.get(), minX, maxX, minY, maxY);
        } else if (mode == SelectionMode::CROSSING) {
            shouldSelect = isEntityIntersectingOrInside(entity.get(), minX, maxX, minY, maxY);
        }

        if (shouldSelect) {
            if (std::find(selectedEntities.begin(),
                          selectedEntities.end(),
                          entity.get()) == selectedEntities.end()) {
                selectedEntities.push_back(entity.get());
            }
        }
    }
}

bool SelectionManager::isEntityCompletelyInside(Entity* entity,
                                                double minX, double maxX,
                                                double minY, double maxY) {
    auto grips = entity->getGripPoints();
    if (grips.empty()) return false;

    for (const auto& grip : grips) {
        if (grip.x < minX || grip.x > maxX || grip.y < minY || grip.y > maxY) {
            return false;
        }
    }
    return true;
}

bool SelectionManager::isEntityIntersectingOrInside(Entity* entity,
                                                    double minX, double maxX,
                                                    double minY, double maxY) {
    auto grips = entity->getGripPoints();
    for (const auto& grip : grips) {
        if (grip.x >= minX && grip.x <= maxX && grip.y >= minY && grip.y <= maxY) {
            return true;
        }
    }

    if (auto* line = dynamic_cast<Line*>(entity)) {
        return lineIntersectsRectangle(line->p1, line->p2, minX, maxX, minY, maxY);
    }

    return false;
}

bool SelectionManager::lineIntersectsRectangle(const Point2D& p1, const Point2D& p2,
                                               double minX, double maxX,
                                               double minY, double maxY) {
    Point2D corners[4] = {
        {minX, minY}, {maxX, minY}, {maxX, maxY}, {minX, maxY}
    };

    for (int i = 0; i < 4; ++i) {
        Point2D edgeStart = corners[i];
        Point2D edgeEnd = corners[(i + 1) % 4];

        // Calcular intersección de SEGMENTOS (no líneas infinitas)
        // usando el algoritmo de parámetros t y u
        double denom = (p2.x - p1.x) * (edgeEnd.y - edgeStart.y) -
                       (p2.y - p1.y) * (edgeEnd.x - edgeStart.x);

        // Si son paralelos, no hay intersección
        if (std::abs(denom) < 1e-10) continue;

        double t = ((edgeStart.x - p1.x) * (edgeEnd.y - edgeStart.y) -
                    (edgeStart.y - p1.y) * (edgeEnd.x - edgeStart.x)) / denom;

        double u = ((edgeStart.x - p1.x) * (p2.y - p1.y) -
                    (edgeStart.y - p1.y) * (p2.x - p1.x)) / denom;

        // Solo hay intersección si ambos parámetros están en [0, 1]
        // (es decir, el punto de cruce está dentro de AMBOS segmentos)
        if (t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0) {
            return true;
        }
    }
    return false;
}

} // namespace cad