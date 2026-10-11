#pragma once
#include "point.hpp"
#include <vector>

namespace cad {

struct IntersectionResult {
    bool intersects = false;
    Point2D point;
    double param = 0.0;
};

IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2, const Point2D& b1, const Point2D& b2);

// Nuevas funciones para soportar Arcos y Círculos
std::vector<Point2D> lineCircleIntersection(const Point2D& p1, const Point2D& p2, const Point2D& center, double radius);
std::vector<Point2D> circleCircleIntersection(const Point2D& c1, double r1, const Point2D& c2, double r2);

} // namespace cad