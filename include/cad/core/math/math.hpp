#pragma once
#include "../geometry/point.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

    inline double distToSegment(const Point2D& p, const Point2D& a, const Point2D& b) {
        double dx = b.x - a.x, dy = b.y - a.y;
        double lenSq = dx * dx + dy * dy;
        if (lenSq == 0.0) return std::hypot(p.x - a.x, p.y - a.y);
        double t = std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq, 0.0, 1.0);
        return std::hypot(p.x - (a.x + t * dx), p.y - (a.y + t * dy));
    }

    inline Point2D reflectPoint(const Point2D& p, const Point2D& a, const Point2D& b) {
        double dx = b.x - a.x, dy = b.y - a.y;
        double lenSq = dx * dx + dy * dy;
        if (lenSq == 0.0) return p;
        double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq;
        return { 2.0 * (a.x + t * dx) - p.x, 2.0 * (a.y + t * dy) - p.y };
    }
    // Añade esto dentro del namespace cad, después de reflectPoint:
    // Función auxiliar para reflejar un punto respecto a un eje definido por dos puntos
    inline Point2D mirrorPoint(const Point2D& p, const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x;
        double dy = axisP2.y - axisP1.y;
        double lenSq = dx * dx + dy * dy;
        if (lenSq == 0.0) return p;
        double t = ((p.x - axisP1.x) * dx + (p.y - axisP1.y) * dy) / lenSq;
        double footX = axisP1.x + t * dx;
        double footY = axisP1.y + t * dy;
        return { 2.0 * footX - p.x, 2.0 * footY - p.y };
    }
    
} // namespace cad