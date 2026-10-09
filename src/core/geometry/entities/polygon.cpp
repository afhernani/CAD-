// src/core/geometry/entities/polygon.cpp
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

void Polygon::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                  const sf::Color& color, float viewScale) const {
    if (sides < 3) return;
    sf::VertexArray va(sf::LineStrip, sides + 1);
    double angleStep = 2.0 * PI / sides;
    for (int i = 0; i <= sides; ++i) {
        double angle = i * angleStep - PI / 2.0 + rotationOffset;
        double px = center.x + radius * std::cos(angle);
        double py = center.y + radius * std::sin(angle);
        va[i].position = w2s(px, py);
        va[i].color = color;
    }
    window.draw(va);
}

bool Polygon::isNear(const Point2D& point, double tolerance) const {
    double angleStep = 2.0 * PI / sides;
    Point2D prev, curr;
    for (int i = 0; i <= sides; ++i) {
        double angle = i * angleStep - PI / 2.0 + rotationOffset;
        curr.x = center.x + radius * std::cos(angle);
        curr.y = center.y + radius * std::sin(angle);
        if (i > 0) {
            if (distToSegment(point, prev, curr) <= tolerance) {
                return true;
            }
        }
        prev = curr;
    }
    return false;
}

void Polygon::move(double dx, double dy) {
    center.x += dx;
    center.y += dy;
}

std::unique_ptr<Entity> Polygon::clone() const {
    auto c = std::make_unique<Polygon>();
    c->center = center;
    c->sides = sides;
    c->radius = radius;
    c->rotationOffset = rotationOffset;
    c->layerName = layerName;
    c->id = id; // Copiar el mismo ID, o generar uno nuevo si es necesario
    return c;
}

void Polygon::rotate(const Point2D& rotCenter, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double cosA = std::cos(rad), sinA = std::sin(rad);
    double dx = this->center.x - rotCenter.x;
    double dy = this->center.y - rotCenter.y;
    this->center.x = rotCenter.x + dx * cosA - dy * sinA;
    this->center.y = rotCenter.y + dx * sinA + dy * cosA;
    this->rotationOffset += rad;
}

void Polygon::scale(const Point2D& basePoint, double factor) {
    center.x = basePoint.x + (center.x - basePoint.x) * factor;
    center.y = basePoint.y + (center.y - basePoint.y) * factor;
    radius *= factor;
}

void Polygon::mirror(const Point2D& axisP1, const Point2D& axisP2) {
    center = reflectPoint(center, axisP1, axisP2);
}

std::vector<Point2D> Polygon::getGripPoints() const {
    std::vector<Point2D> grips;
    grips.push_back(center);
    double angleStep = 2.0 * PI / sides;
    for (int i = 0; i < sides; ++i) {
        double angle = i * angleStep - PI / 2.0 + rotationOffset;
        grips.push_back({
            center.x + radius * std::cos(angle),
            center.y + radius * std::sin(angle)
        });
    }
    return grips;
}

std::vector<Point2D> Polygon::getSnapPoints() const {
    std::vector<Point2D> snaps = {center};
    double angleStep = 2.0 * PI / sides;
    for (int i = 0; i < sides; ++i) {
        double a1 = i * angleStep - PI / 2.0 + rotationOffset;
        double a2 = (i + 1) * angleStep - PI / 2.0 + rotationOffset;
        double x1 = center.x + radius * std::cos(a1);
        double y1 = center.y + radius * std::sin(a1);
        double x2 = center.x + radius * std::cos(a2);
        double y2 = center.y + radius * std::sin(a2);
        snaps.push_back({x1, y1});
        snaps.push_back({(x1 + x2) / 2.0, (y1 + y2) / 2.0});
    }
    return snaps;
}

void Polygon::moveGrip(int index, const Point2D& newPos) {
    if (index == 0) {
        center = newPos;
    } else {
        radius = std::hypot(newPos.x - center.x, newPos.y - center.y);
    }
}

void Polygon::copyFrom(const Entity& src) {
    auto& p = dynamic_cast<const Polygon&>(src);
    center = p.center;
    sides = p.sides;
    radius = p.radius;
    rotationOffset = p.rotationOffset;
    layerName = p.layerName;
    id = p.id; // Copiar el mismo ID, o generar uno nuevo si es necesario
}

nlohmann::json Polygon::toJson() const {
    return {
        {"type", "Polygon"},
        {"center", {{"x", center.x}, {"y", center.y}}},
        {"sides", sides},
        {"radius", radius},
        {"rotationOffset", rotationOffset},
        {"layer", layerName},
        {"id", id}
    };
}

} // namespace cad