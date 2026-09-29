// src/core/geometry/entities/ellipse.cpp
#include "cad/core/geometry/entities/ellipse.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

Point2D Ellipse::getPointOnEllipse(double angle) const {
    double rotatedAngle = angle + rotationAngle;
    double x = center.x + majorRadius * std::cos(rotatedAngle);
    double y = center.y + minorRadius * std::sin(rotatedAngle);
    return {x, y};
}

void Ellipse::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                  const sf::Color& color, float viewScale) const {
    const int numPoints = 64;
    sf::VertexArray va(sf::LineStrip, numPoints + 1);
    double angleStep = 2.0 * PI / numPoints;
    for (int i = 0; i <= numPoints; ++i) {
        double angle = i * angleStep;
        Point2D pt = getPointOnEllipse(angle);
        va[i].position = w2s(pt.x, pt.y);
        va[i].color = color;
    }
    window.draw(va);
}

bool Ellipse::isNear(const Point2D& point, double tolerance) const {
    const int numPoints = 64;
    double angleStep = 2.0 * PI / numPoints;
    for (int i = 0; i < numPoints; ++i) {
        double angle = i * angleStep;
        Point2D pt = getPointOnEllipse(angle);
        double dist = std::hypot(point.x - pt.x, point.y - pt.y);
        if (dist <= tolerance) return true;
    }
    return false;
}

void Ellipse::move(double dx, double dy) {
    center.x += dx;
    center.y += dy;
}

void Ellipse::rotate(const Point2D& rotCenter, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double cosA = std::cos(rad), sinA = std::sin(rad);
    double dx = this->center.x - rotCenter.x;
    double dy = this->center.y - rotCenter.y;
    this->center.x = rotCenter.x + dx * cosA - dy * sinA;
    this->center.y = rotCenter.y + dx * sinA + dy * cosA;
    this->rotationAngle += rad;
}

void Ellipse::scale(const Point2D& base, double factor) {
    double dx = center.x - base.x, dy = center.y - base.y;
    center.x = base.x + dx * factor;
    center.y = base.y + dy * factor;
    majorRadius *= factor;
    minorRadius *= factor;
}

void Ellipse::mirror(const Point2D& axisP1, const Point2D& axisP2) {
    center = mirrorPoint(center, axisP1, axisP2);
    rotationAngle = -rotationAngle;
}

std::unique_ptr<Entity> Ellipse::clone() const {
    auto c = std::make_unique<Ellipse>();
    c->center = center;
    c->majorRadius = majorRadius;
    c->minorRadius = minorRadius;
    c->rotationAngle = rotationAngle;
    c->layerName = layerName;
    return c;
}

void Ellipse::copyFrom(const Entity& src) {
    auto& e = dynamic_cast<const Ellipse&>(src);
    center = e.center;
    majorRadius = e.majorRadius;
    minorRadius = e.minorRadius;
    rotationAngle = e.rotationAngle;
    layerName = e.layerName;
}

std::vector<Point2D> Ellipse::getGripPoints() const {
    std::vector<Point2D> grips;
    grips.push_back(center);
    grips.push_back(getPointOnEllipse(0.0));
    grips.push_back(getPointOnEllipse(PI / 2.0));
    grips.push_back(getPointOnEllipse(PI));
    grips.push_back(getPointOnEllipse(3 * PI / 2.0));
    return grips;
}

void Ellipse::moveGrip(int index, const Point2D& newPos) {
    if (index == 0) {
        center = newPos;
    }
    else if (index == 1) {
        double dx = newPos.x - center.x;
        double dy = newPos.y - center.y;
        majorRadius = std::hypot(dx, dy);
        rotationAngle = std::atan2(dy, dx);
    }
    else if (index == 2) {
        Point2D majorAxisPt = getPointOnEllipse(0.0);
        double dx = newPos.x - center.x;
        double dy = newPos.y - center.y;
        minorRadius = std::hypot(dx, dy);
        rotationAngle = std::atan2(majorAxisPt.y - center.y, majorAxisPt.x - center.x);
    }
    else if (index == 3) {
        double dx = newPos.x - center.x;
        double dy = newPos.y - center.y;
        majorRadius = std::hypot(dx, dy);
        rotationAngle = std::atan2(dy, dx) - PI;
    }
    else if (index == 4) {
        Point2D majorAxisPt = getPointOnEllipse(0.0);
        double dx = newPos.x - center.x;
        double dy = newPos.y - center.y;
        minorRadius = std::hypot(dx, dy);
        rotationAngle = std::atan2(majorAxisPt.y - center.y, majorAxisPt.x - center.x);
    }
}

std::vector<Point2D> Ellipse::getSnapPoints() const {
    std::vector<Point2D> snaps = {center};
    snaps.push_back(getPointOnEllipse(0.0));
    snaps.push_back(getPointOnEllipse(PI / 2.0));
    snaps.push_back(getPointOnEllipse(PI));
    snaps.push_back(getPointOnEllipse(3 * PI / 2.0));
    return snaps;
}

nlohmann::json Ellipse::toJson() const {
    return {
        {"type", "Ellipse"},
        {"center", {{"x", center.x}, {"y", center.y}}},
        {"majorRadius", majorRadius},
        {"minorRadius", minorRadius},
        {"rotationAngle", rotationAngle},
        {"layer", layerName}
    };
}

} // namespace cad