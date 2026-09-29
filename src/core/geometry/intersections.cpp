#include "cad/core/geometry/intersections.hpp"
#include <cmath>

namespace cad {

IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2,
                                       const Point2D& b1, const Point2D& b2) {
    IntersectionResult result;
    double dx1 = a2.x - a1.x, dy1 = a2.y - a1.y;
    double dx2 = b2.x - b1.x, dy2 = b2.y - b1.y;
    double denom = dx1 * dy2 - dy1 * dx2;
    if (std::abs(denom) < 1e-10) return result;
    double t = ((b1.x - a1.x) * dy2 - (b1.y - a1.y) * dx2) / denom;
    result.intersects = true;
    result.point = { a1.x + t * dx1, a1.y + t * dy1 };
    result.param = t;
    return result;
}

} // namespace cad