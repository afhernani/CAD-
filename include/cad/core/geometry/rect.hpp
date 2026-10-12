#pragma once
#include <algorithm>

namespace cad {

struct Rect {
    double minX, minY, maxX, maxY;

    Rect() : minX(0), minY(0), maxX(0), maxY(0) {}
    Rect(double x1, double y1, double x2, double y2) 
        : minX(std::min(x1, x2)), minY(std::min(y1, y2)), 
          maxX(std::max(x1, x2)), maxY(std::max(y1, y2)) {}

    // Comprobación de intersección (Algoritmo estándar AABB)
    bool intersects(const Rect& other) const {
        return !(minX > other.maxX || maxX < other.minX || 
                 minY > other.maxY || maxY < other.minY);
    }
};

} // namespace cad