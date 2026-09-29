// src/core/geometry/entities/circle.cpp
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

void Circle::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                 const sf::Color& color, float viewScale) const {
    sf::CircleShape shape(static_cast<float>(radius * viewScale));
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(color);
    shape.setOutlineThickness(1.5f);
    shape.setOrigin(static_cast<float>(radius * viewScale),
                    static_cast<float>(radius * viewScale));
    shape.setPosition(w2s(center.x, center.y));
    window.draw(shape);
}

bool Circle::isNear(const Point2D& point, double tolerance) const {
    double dist = std::hypot(point.x - center.x, point.y - center.y);
    return std::abs(dist - radius) <= tolerance;
}

void Circle::move(double dx, double dy) {
    center.x += dx;
    center.y += dy;
}

std::unique_ptr<Entity> Circle::clone() const {
    auto c = std::make_unique<Circle>();
    c->center = center;
    c->radius = radius;
    c->layerName = layerName;
    return c;
}

void Circle::rotate(const Point2D& rotCenter, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double cosA = std::cos(rad), sinA = std::sin(rad);
    double dx = center.x - rotCenter.x;
    double dy = center.y - rotCenter.y;
    center.x = rotCenter.x + dx * cosA - dy * sinA;
    center.y = rotCenter.y + dx * sinA + dy * cosA;
}

void Circle::scale(const Point2D& basePoint, double factor) {
    center.x = basePoint.x + (center.x - basePoint.x) * factor;
    center.y = basePoint.y + (center.y - basePoint.y) * factor;
    radius *= factor;
}

void Circle::mirror(const Point2D& axisP1, const Point2D& axisP2) {
    center = reflectPoint(center, axisP1, axisP2);
}

std::vector<Point2D> Circle::getGripPoints() const {
    return {center, {center.x + radius, center.y}};
}

std::vector<Point2D> Circle::getSnapPoints() const {
    return {
        center,
        {center.x + radius, center.y},
        {center.x - radius, center.y},
        {center.x, center.y + radius},
        {center.x, center.y - radius}
    };
}

void Circle::moveGrip(int index, const Point2D& newPos) {
    if (index == 0) {
        center = newPos;
    } else if (index == 1) {
        radius = std::hypot(newPos.x - center.x, newPos.y - center.y);
    }
}

void Circle::copyFrom(const Entity& src) {
    auto& c = dynamic_cast<const Circle&>(src);
    center = c.center;
    radius = c.radius;
    layerName = c.layerName;
}

nlohmann::json Circle::toJson() const {
    return {
        {"type", "Circle"},
        {"center", {{"x", center.x}, {"y", center.y}}},
        {"radius", radius},
        {"layer", layerName}
    };
}

} // namespace cad