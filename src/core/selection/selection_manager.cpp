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
                                      bool addToSelection) {
    double minX = std::min(p1.x, p2.x);
    double maxX = std::max(p1.x, p2.x);
    double minY = std::min(p1.y, p2.y);
    double maxY = std::max(p1.y, p2.y);

    SelectionMode mode = determineMode(p1, p2);

    if (!addToSelection) {
        selectedEntities.clear();
    }

    for (const auto& entity : entities) {
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

        auto inter = lineLineIntersection(p1, p2, edgeStart, edgeEnd);
        if (inter.intersects) {
            return true;
        }
    }
    return false;
}

} // namespace cad