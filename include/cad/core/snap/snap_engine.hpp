#pragma once
#include "../geometry/point.hpp"
#include <vector>
#include <memory>

namespace cad {

// Forward declaration para evitar includes pesados
class Entity;

enum class SnapType { 
    NONE, 
    ENDPOINT, 
    MIDPOINT, 
    CENTER, 
    INTERSECTION 
};

class SnapEngine {
public:
    struct SnapResult {
        bool active = false;
        Point2D point{0.0, 0.0};
        SnapType type = SnapType::NONE;
    };

    // Calcula el punto de snap más cercano basado en las entidades visibles
    // entities: Lista de entidades del documento
    // mousePos: Posición actual del ratón en coordenadas del mundo
    // viewScale: Escala actual para calcular la tolerancia en píxeles
    SnapResult findSnap(const std::vector<std::unique_ptr<Entity>>& entities, 
                        const Point2D& mousePos, 
                        double viewScale) const;

private:
    // Tolerancia base en píxeles para considerar un punto "cercano"
    static constexpr double PIXEL_TOLERANCE = 10.0; 
};

} // namespace cad