// src/core/geometry/entities/arc.cpp
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

void Arc::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
              const sf::Color& color, float viewScale) const {
    const int numPoints = 64;
    sf::VertexArray va(sf::LineStrip, numPoints);
    double startRad = startAngle * PI / 180.0;
    double endRad = endAngle * PI / 180.0;
    double step = (endRad - startRad) / (numPoints - 1);
    for (int i = 0; i < numPoints; ++i) {
        double angle = startRad + i * step;
        double px = center.x + radius * std::cos(angle);
        double py = center.y + radius * std::sin(angle);
        va[i].position = w2s(px, py);
        va[i].color = color;
    }
    window.draw(va);
}

bool Arc::isNear(const Point2D& point, double tolerance) const {
    double dist = std::hypot(point.x - center.x, point.y - center.y);
    return std::abs(dist - radius) <= tolerance;
}

void Arc::move(double dx, double dy) {
    center.x += dx;
    center.y += dy;
}

std::unique_ptr<Entity> Arc::clone() const {
    auto c = std::make_unique<Arc>();
    c->center = center;
    c->radius = radius;
    c->startAngle = startAngle;
    c->endAngle = endAngle;
    c->layerName = layerName;
    c->id = id; // Copiar el mismo ID, o generar uno nuevo si es necesario
    return c;
}

void Arc::rotate(const Point2D& rotCenter, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double cosA = std::cos(rad), sinA = std::sin(rad);
    double dx = this->center.x - rotCenter.x;
    double dy = this->center.y - rotCenter.y;
    this->center.x = rotCenter.x + dx * cosA - dy * sinA;
    this->center.y = rotCenter.y + dx * sinA + dy * cosA;
    this->startAngle += angleDeg;
    this->endAngle += angleDeg;
    auto normalize = [](double& ang) {
        while (ang < 0.0) ang += 360.0;
        while (ang >= 360.0) ang -= 360.0;
    };
    normalize(this->startAngle);
    normalize(this->endAngle);
}

void Arc::scale(const Point2D& basePoint, double factor) {
    center.x = basePoint.x + (center.x - basePoint.x) * factor;
    center.y = basePoint.y + (center.y - basePoint.y) * factor;
    radius *= factor;
}

void Arc::mirror(const Point2D& axisP1, const Point2D& axisP2) {
    center = reflectPoint(center, axisP1, axisP2);
    double dx = axisP2.x - axisP1.x;
    double dy = axisP2.y - axisP1.y;
    double axisAngle = std::atan2(dy, dx) * 180.0 / PI;
    auto reflectAngle = [&](double ang) {
        double newAng = 2.0 * axisAngle - ang;
        while (newAng < 0.0) newAng += 360.0;
        while (newAng >= 360.0) newAng -= 360.0;
        return newAng;
    };
    startAngle = reflectAngle(startAngle);
    endAngle = reflectAngle(endAngle);
}

std::vector<Point2D> Arc::getGripPoints() const {
    double sRad = startAngle * PI / 180.0;
    double eRad = endAngle * PI / 180.0;
    return {
        center,
        {center.x + radius * std::cos(sRad), center.y + radius * std::sin(sRad)},
        {center.x + radius * std::cos(eRad), center.y + radius * std::sin(eRad)}
    };
}

std::vector<Point2D> Arc::getSnapPoints() const {
    double sRad = startAngle * PI / 180.0;
    double eRad = endAngle * PI / 180.0;
    double midRad = (sRad + eRad) / 2.0;
    if (std::abs(eRad - sRad) > PI) midRad += PI;
    return {
        center,
        {center.x + radius * std::cos(sRad), center.y + radius * std::sin(sRad)},
        {center.x + radius * std::cos(eRad), center.y + radius * std::sin(eRad)},
        {center.x + radius * std::cos(midRad), center.y + radius * std::sin(midRad)}
    };
}

void Arc::moveGrip(int index, const Point2D& newPos) {
    if (index == 0) {
        center = newPos;
    } else if (index == 1) {
        startAngle = std::atan2(newPos.y - center.y, newPos.x - center.x) * 180.0 / PI;
    } else if (index == 2) {
        endAngle = std::atan2(newPos.y - center.y, newPos.x - center.x) * 180.0 / PI;
    }
}

void Arc::copyFrom(const Entity& src) {
    auto& a = dynamic_cast<const Arc&>(src);
    center = a.center;
    radius = a.radius;
    startAngle = a.startAngle;
    endAngle = a.endAngle;
    layerName = a.layerName;
    id = a.id; // Copiar el mismo ID, o generar uno nuevo si es necesario
}

nlohmann::json Arc::toJson() const {
    return {
        {"type", "Arc"},
        {"center", {{"x", center.x}, {"y", center.y}}},
        {"radius", radius},
        {"startAngle", startAngle},
        {"endAngle", endAngle},
        {"layer", layerName},
        {"id", id}
    };
}

} // namespace cad