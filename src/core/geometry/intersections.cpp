#include "cad/core/geometry/intersections.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

    IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2, const Point2D& b1, const Point2D& b2) {
        double denom = (b2.y - b1.y) * (a2.x - a1.x) - (b2.x - b1.x) * (a2.y - a1.y);
        if (std::abs(denom) < 1e-10) return {false, {0,0}, 0.0};
        double ua = ((b2.x - b1.x) * (a1.y - b1.y) - (b2.y - b1.y) * (a1.x - b1.x)) / denom;
        double ub = ((a2.x - a1.x) * (a1.y - b1.y) - (a2.y - a1.y) * (a1.x - b1.x)) / denom;
        if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
            return {true, {a1.x + ua * (a2.x - a1.x), a1.y + ua * (a2.y - a1.y)}, ua};
        }
        return {false, {0,0}, 0.0};
    }

    std::vector<Point2D> lineCircleIntersection(const Point2D& p1, const Point2D& p2, const Point2D& center, double radius) {
        std::vector<Point2D> pts;
        double dx = p2.x - p1.x, dy = p2.y - p1.y;
        double fx = p1.x - center.x, fy = p1.y - center.y;
        double a = dx * dx + dy * dy;
        double b = 2.0 * (fx * dx + fy * dy);
        double c = (fx * fx + fy * fy) - radius * radius;
        double discriminant = b * b - 4.0 * a * c;
        
        if (discriminant < 0) return pts;
        discriminant = std::sqrt(discriminant);
        double t1 = (-b - discriminant) / (2.0 * a);
        double t2 = (-b + discriminant) / (2.0 * a);
        
        pts.push_back({p1.x + t1 * dx, p1.y + t1 * dy});
        if (discriminant > 1e-10) pts.push_back({p1.x + t2 * dx, p1.y + t2 * dy});
        return pts;
    }

    std::vector<Point2D> circleCircleIntersection(const Point2D& c1, double r1, const Point2D& c2, double r2) {
        std::vector<Point2D> pts;
        double d = std::hypot(c2.x - c1.x, c2.y - c1.y);
        if (d > r1 + r2 || d < std::abs(r1 - r2) || d == 0.0) return pts;
        
        double a = (r1 * r1 - r2 * r2 + d * d) / (2.0 * d);
        double h = std::sqrt(std::max(0.0, r1 * r1 - a * a));
        double x2 = c1.x + a * (c2.x - c1.x) / d;
        double y2 = c1.y + a * (c2.y - c1.y) / d;
        
        pts.push_back({x2 + h * (c2.y - c1.y) / d, y2 - h * (c2.x - c1.x) / d});
        if (h > 1e-10) pts.push_back({x2 - h * (c2.y - c1.y) / d, y2 + h * (c2.x - c1.x) / d});
        return pts;
    }

} // namespace cad