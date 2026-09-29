#pragma once
#include "point.hpp"

namespace cad {

struct IntersectionResult {
    bool intersects = false;
    Point2D point;
    double param = 0.0;
};

IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2,
                                       const Point2D& b1, const Point2D& b2);

} // namespace cad