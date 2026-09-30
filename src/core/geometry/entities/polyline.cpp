// src/core/geometry/entities/polyline.cpp
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

void Polyline::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                   const sf::Color& color, float viewScale) const {
    if (points.size() < 2) return;

    // Si está cerrada, necesitamos un vértice extra en el LineStrip para volver al inicio
    size_t numVertices = closed ? points.size() + 1 : points.size();
    sf::VertexArray va(sf::LineStrip, numVertices);

    // Dibujar todos los puntos reales
    for (size_t i = 0; i < points.size(); ++i) {
        va[i].position = w2s(points[i].x, points[i].y);
        va[i].color = color;
    }

    // Si está cerrada, añadir el primer punto al final para cerrar el bucle visualmente
    if (closed) {
        va[points.size()].position = w2s(points[0].x, points[0].y);
        va[points.size()].color = color;
    }

    window.draw(va);
}

bool Polyline::isNear(const Point2D& point, double tolerance) const {
    for (size_t i = 1; i < points.size(); ++i) {
        if (distToSegment(point, points[i - 1], points[i]) <= tolerance)
            return true;
    }
    return false;
}

void Polyline::move(double dx, double dy) {
    for (auto& p : points) {
        p.x += dx;
        p.y += dy;
    }
}

std::unique_ptr<Entity> Polyline::clone() const {
    auto c = std::make_unique<Polyline>();
    c->points = points;
    c->closed = closed; 
    c->layerName = layerName;
    return c;
}

void Polyline::rotate(const Point2D& rotCenter, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double cosA = std::cos(rad), sinA = std::sin(rad);
    for (auto& p : points) {
        double dx = p.x - rotCenter.x;
        double dy = p.y - rotCenter.y;
        p.x = rotCenter.x + dx * cosA - dy * sinA;
        p.y = rotCenter.y + dx * sinA + dy * cosA;
    }
}

void Polyline::scale(const Point2D& basePoint, double factor) {
    for (auto& p : points) {
        p.x = basePoint.x + (p.x - basePoint.x) * factor;
        p.y = basePoint.y + (p.y - basePoint.y) * factor;
    }
}

void Polyline::mirror(const Point2D& axisP1, const Point2D& axisP2) {
    for (auto& p : points) {
        p = reflectPoint(p, axisP1, axisP2);
    }
}

std::vector<Point2D> Polyline::getGripPoints() const {
    return points;
}

std::vector<Point2D> Polyline::getSnapPoints() const {
    std::vector<Point2D> snaps = points;
    for (size_t i = 1; i < points.size(); ++i) {
        snaps.push_back({
            (points[i - 1].x + points[i].x) / 2.0,
            (points[i - 1].y + points[i].y) / 2.0
        });
    }
    return snaps;
}

void Polyline::moveGrip(int index, const Point2D& newPos) {
    if (index >= 0 && index < static_cast<int>(points.size())) {
        points[index] = newPos;
    }
}

void Polyline::copyFrom(const Entity& src) {
    auto& pl = dynamic_cast<const Polyline&>(src);
    points = pl.points;
    layerName = pl.layerName;
}

nlohmann::json Polyline::toJson() const {
    nlohmann::json pts = nlohmann::json::array();
    for (const auto& p : points) {
        pts.push_back({{"x", p.x}, {"y", p.y}});
    }
    return {
        {"type", "Polyline"},
        {"points", pts},
        {"layer", layerName}
    };
}

} // namespace cad