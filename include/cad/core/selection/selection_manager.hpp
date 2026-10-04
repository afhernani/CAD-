#pragma once
#include "../geometry/entity.hpp"
#include "../geometry/point.hpp"
#include "cad/core/document/document.hpp"
#include <vector>
#include <memory>

namespace cad {

enum class SelectionMode {
    SINGLE,
    WINDOW,
    CROSSING
};

class SelectionManager {
public:
    SelectionManager() = default;

    // Selección por ventana: recibe las entidades del documento y el vector de seleccionados
    void selectByWindow(const Point2D& p1, const Point2D& p2,
                        const std::vector<std::unique_ptr<Entity>>& entities,
                        std::vector<Entity*>& selectedEntities,
                        bool addToSelection,
                        const Document& doc);

    static SelectionMode determineMode(const Point2D& start, const Point2D& end);

private:
    bool isEntityCompletelyInside(Entity* entity, double minX, double maxX,
                                  double minY, double maxY);
    bool isEntityIntersectingOrInside(Entity* entity, double minX, double maxX,
                                      double minY, double maxY);
    bool lineIntersectsRectangle(const Point2D& p1, const Point2D& p2,
                                 double minX, double maxX, double minY, double maxY);
};

} // namespace cad