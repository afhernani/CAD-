// src/core/geometry/entities/arc.cpp
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;

    // ✅ Helper: Normaliza cualquier ángulo al rango [0, 2*PI)
    double normalizeAngle(double angle) {
        while (angle < 0.0) angle += 2.0 * PI;
        while (angle >= 2.0 * PI) angle -= 2.0 * PI;
        return angle;
    }

    // ✅ Helper: Verifica si un ángulo está dentro del rango antihorario [start, end]
    bool isAngleBetween(double angle, double start, double end) {
        angle = normalizeAngle(angle);
        start = normalizeAngle(start);
        end = normalizeAngle(end);
        
        if (start <= end) {
            return angle >= start && angle <= end;
        } else {
            // Caso donde el arco cruza el ángulo 0 (ej: de 300° a 60°)
            return angle >= start || angle <= end;
        }
    }
}

void Arc::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
              const sf::Color& color, float viewScale) const {
    const int numPoints = 64;
    sf::VertexArray va(sf::LineStrip, numPoints);
    double startRad = startAngle * PI / 180.0;
    double endRad = endAngle * PI / 180.0;
    
    // ✅ Calcular la diferencia en sentido antihorario (siempre positiva)
    double diff = endRad - startRad;
    while (diff < 0.0) diff += 2.0 * PI;
    while (diff >= 2.0 * PI) diff -= 2.0 * PI;
    
    // Si es prácticamente un círculo completo, lo dibujamos como tal
    if (diff < 1e-5) diff = 2.0 * PI;

    double step = diff / (numPoints - 1);
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
    // 1. Comprobar distancia al radio
    double dist = std::hypot(point.x - center.x, point.y - center.y);
    if (std::abs(dist - radius) > tolerance) return false;
    
    // 2. ✅ Comprobar que el punto está dentro del segmento del arco, no en el círculo completo
    double angle = std::atan2(point.y - center.y, point.x - center.x);
    double startRad = startAngle * PI / 180.0;
    double endRad = endAngle * PI / 180.0;
    
    return isAngleBetween(angle, startRad, endRad);
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
    c->id = id;
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
    
    // Normalizar a [0, 360)
    while (this->startAngle < 0.0) this->startAngle += 360.0;
    while (this->startAngle >= 360.0) this->startAngle -= 360.0;
    while (this->endAngle < 0.0) this->endAngle += 360.0;
    while (this->endAngle >= 360.0) this->endAngle -= 360.0;
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
    
    // Al reflejar, el sentido antihorario se invierte, así que intercambiamos start y end
    double newStart = reflectAngle(endAngle);
    double newEnd = reflectAngle(startAngle);
    startAngle = newStart;
    endAngle = newEnd;
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
    
    // ✅ Calcular el punto medio angular correctamente, incluso si cruza el 0
    double diff = eRad - sRad;
    while (diff < 0.0) diff += 2.0 * PI;
    while (diff >= 2.0 * PI) diff -= 2.0 * PI;
    double midRad = sRad + diff / 2.0;
    
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
        if (startAngle < 0) startAngle += 360.0;
    } else if (index == 2) {
        endAngle = std::atan2(newPos.y - center.y, newPos.x - center.x) * 180.0 / PI;
        if (endAngle < 0) endAngle += 360.0;
    }
}

void Arc::copyFrom(const Entity& src) {
    auto& a = dynamic_cast<const Arc&>(src);
    center = a.center;
    radius = a.radius;
    startAngle = a.startAngle;
    endAngle = a.endAngle;
    layerName = a.layerName;
    id = a.id;
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