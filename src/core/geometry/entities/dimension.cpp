// src/core/geometry/entities/dimension.cpp
#include "cad/core/geometry/entities/dimension.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

void Dimension::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                    const sf::Color& color, float viewScale) const {
    Point2D ext1, ext2, lineStart, lineEnd;

    if (type == DimType::ALIGNED || isAligned) {
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double len = std::sqrt(dx*dx + dy*dy);
        if (len == 0) return;
        double nx = -dy / len;
        double ny = dx / len;
        double vx = location.x - p1.x;
        double vy = location.y - p1.y;
        double offset = vx * nx + vy * ny;
        lineStart = {p1.x + nx * offset, p1.y + ny * offset};
        lineEnd   = {p2.x + nx * offset, p2.y + ny * offset};
        ext1 = p1; ext2 = p2;
    }
    else if (type == DimType::RADIUS || type == DimType::DIAMETER) {
        lineStart = p1;
        lineEnd = p2;
        ext1 = p1; ext2 = p2;
    }
    else if (type == DimType::ANGULAR) {
        // location = vértice, p1 = punto línea 1, p2 = punto línea 2, p3 = punto de clic
        double arcRadius = std::hypot(p3.x - location.x, p3.y - location.y);
        if (arcRadius < 1.0) arcRadius = 10.0;
        // Ángulos desde el vértice hacia las dos líneas
        double angle1 = std::atan2(p1.y - location.y, p1.x - location.x);
        double angle2 = std::atan2(p2.y - location.y, p2.x - location.x);
        // Dibujar el ARCO
        const int numPoints = 64;
        sf::VertexArray arc(sf::LineStrip, numPoints);
        double diff = angle2 - angle1;
        while (diff < 0) diff += 2 * PI;
        while (diff >= 2 * PI) diff -= 2 * PI;
        double step = diff / (numPoints - 1);
        for (int i = 0; i < numPoints; ++i) {
            double angle = angle1 + i * step;
            double px = location.x + arcRadius * std::cos(angle);
            double py = location.y + arcRadius * std::sin(angle);
            arc[i].position = w2s(px, py);
            arc[i].color = color;
        }
        window.draw(arc);
        // Líneas de extensión desde el vértice hasta los extremos del arco
        sf::Color extColor = color;
        extColor.a = 150;
        Point2D ext1End = {location.x + arcRadius * std::cos(angle1), location.y + arcRadius * std::sin(angle1)};
        Point2D ext2End = {location.x + arcRadius * std::cos(angle2), location.y + arcRadius * std::sin(angle2)};
        sf::Vertex ext1[] = { sf::Vertex(w2s(location.x, location.y), extColor), sf::Vertex(w2s(ext1End.x, ext1End.y), extColor) };
        sf::Vertex ext2[] = { sf::Vertex(w2s(location.x, location.y), extColor), sf::Vertex(w2s(ext2End.x, ext2End.y), extColor) };
        window.draw(ext1, 2, sf::Lines);
        window.draw(ext2, 2, sf::Lines);
        // Flechas en los extremos del arco
        float arrowSize = 4.0f;
        sf::CircleShape arrow1(arrowSize);
        arrow1.setFillColor(color);
        arrow1.setOrigin(arrowSize, arrowSize);
        arrow1.setPosition(w2s(ext1End.x, ext1End.y));
        window.draw(arrow1);

        sf::CircleShape arrow2(arrowSize);
        arrow2.setFillColor(color);
        arrow2.setOrigin(arrowSize, arrowSize);
        arrow2.setPosition(w2s(ext2End.x, ext2End.y));
        window.draw(arrow2);
        return; // Salir para no dibujar línea recta ni flechas triangulares
    }
    else {
        // HORIZONTAL / VERTICAL por defecto
        if (isHorizontal) {
            double y = location.y;
            ext1 = {p1.x, y}; ext2 = {p2.x, y};
            lineStart = {p1.x, y}; lineEnd = {p2.x, y};
        } else {
            double x = location.x;
            ext1 = {x, p1.y}; ext2 = {x, p2.y};
            lineStart = {x, p1.y}; lineEnd = {x, p2.y};
        }
    }
    // --- DIBUJO ---
    sf::Color dimColor = color;
    dimColor.a = 150;
    // Líneas de extensión
    if (type != DimType::ANGULAR && type != DimType::RADIUS && type != DimType::DIAMETER) {
        sf::Vertex extLine1[] = { sf::Vertex(w2s(p1.x, p1.y), dimColor), sf::Vertex(w2s(ext1.x, ext1.y), dimColor) };
        sf::Vertex extLine2[] = { sf::Vertex(w2s(p2.x, p2.y), dimColor), sf::Vertex(w2s(ext2.x, ext2.y), dimColor) };
        window.draw(extLine1, 2, sf::Lines);
        window.draw(extLine2, 2, sf::Lines);
    }
    // Línea de cota (o líder para radio)
    sf::Vertex dimLine[] = { sf::Vertex(w2s(lineStart.x, lineStart.y), color), sf::Vertex(w2s(lineEnd.x, lineEnd.y), color) };
    window.draw(dimLine, 2, sf::Lines);
    // Flechas (Tamaño fijo en píxeles)
    float arrowLen = 8.0f;
    float arrowWidth = 3.0f;
    sf::Vector2f sPos = w2s(lineStart.x, lineStart.y);
    sf::Vector2f ePos = w2s(lineEnd.x, lineEnd.y);
    sf::Vector2f dir = ePos - sPos;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len > 0) { dir.x /= len; dir.y /= len; }
    sf::Vector2f perp = {-dir.y, dir.x};
    // Flecha inicio no dibujar en cota de RADIO
    if (type != DimType::RADIUS) {
        sf::Vertex arrow1[] = {
            sf::Vertex(sPos, color),
            sf::Vertex(sf::Vector2f(sPos.x + dir.x * arrowLen + perp.x * arrowWidth, sPos.y + dir.y * arrowLen + perp.y * arrowWidth), color),
            sf::Vertex(sf::Vector2f(sPos.x + dir.x * arrowLen - perp.x * arrowWidth, sPos.y + dir.y * arrowLen - perp.y * arrowWidth), color)
        };
        window.draw(arrow1, 3, sf::Triangles);
    }
    // Flecha final
    sf::Vertex arrow2[] = {
        sf::Vertex(ePos, color),
        sf::Vertex(sf::Vector2f(ePos.x - dir.x * arrowLen + perp.x * arrowWidth, ePos.y - dir.y * arrowLen + perp.y * arrowWidth), color),
        sf::Vertex(sf::Vector2f(ePos.x - dir.x * arrowLen - perp.x * arrowWidth, ePos.y - dir.y * arrowLen - perp.y * arrowWidth), color)
    };
    window.draw(arrow2, 3, sf::Triangles);
}

bool Dimension::isNear(const Point2D& point, double tolerance) const {
    Point2D ext1, ext2, lineStart, lineEnd;
    if (type == DimType::ALIGNED || isAligned) {
        double dx = p2.x - p1.x, dy = p2.y - p1.y;
        double len = std::sqrt(dx*dx + dy*dy);
        if (len == 0) return false;
        double nx = -dy / len, ny = dx / len;
        double vx = location.x - p1.x, vy = location.y - p1.y;
        double offset = vx * nx + vy * ny;
        lineStart = {p1.x + nx * offset, p1.y + ny * offset};
        lineEnd   = {p2.x + nx * offset, p2.y + ny * offset};
        ext1 = p1; ext2 = p2;
    }
    else if (type == DimType::RADIUS || type == DimType::DIAMETER) {
        lineStart = p1; lineEnd = p2;
        ext1 = p1; ext2 = p2;
    }
    else if (type == DimType::ANGULAR) {
        double arcRadius = std::hypot(p3.x - location.x, p3.y - location.y);
        double angle1 = std::atan2(p1.y - location.y, p1.x - location.x);
        double angle2 = std::atan2(p2.y - location.y, p2.x - location.x);
        double distToCenter = std::hypot(point.x - location.x, point.y - location.y);
        double distToArc = std::abs(distToCenter - arcRadius);
        double angleP = std::atan2(point.y - location.y, point.x - location.x);
        double diff = angle2 - angle1;
        while (diff < 0) diff += 2 * PI;
        double diffP = angleP - angle1;
        while (diffP < 0) diffP += 2 * PI;
        if (distToArc <= tolerance && diffP <= diff) return true;
        Point2D ext1End = {location.x + arcRadius * std::cos(angle1), location.y + arcRadius * std::sin(angle1)};
        Point2D ext2End = {location.x + arcRadius * std::cos(angle2), location.y + arcRadius * std::sin(angle2)};
        double d1 = distToSegment(point, location, ext1End);
        double d2 = distToSegment(point, location, ext2End);
        return (d1 <= tolerance || d2 <= tolerance);
    }
    else {
        if (isHorizontal) {
            double y = location.y;
            ext1 = {p1.x, y}; ext2 = {p2.x, y};
            lineStart = {p1.x, y}; lineEnd = {p2.x, y};
        } else {
            double x = location.x;
            ext1 = {x, p1.y}; ext2 = {x, p2.y};
            lineStart = {x, p1.y}; lineEnd = {x, p2.y};
        }
    }
    double d1 = distToSegment(point, p1, ext1);
    double d2 = distToSegment(point, p2, ext2);
    double d3 = distToSegment(point, lineStart, lineEnd);
    return (d1 <= tolerance || d2 <= tolerance || d3 <= tolerance);
}

void Dimension::move(double dx, double dy) {
    p1.x += dx; p1.y += dy;
    p2.x += dx; p2.y += dy;
    location.x += dx; location.y += dy;
}

void Dimension::rotate(const Point2D& center, double angleDeg) {
    double rad = angleDeg * PI / 180.0;
    double cosA = std::cos(rad), sinA = std::sin(rad);
    auto rotatePoint = [&](Point2D& p) {
        double dx = p.x - center.x, dy = p.y - center.y;
        p.x = center.x + dx * cosA - dy * sinA;
        p.y = center.y + dx * sinA + dy * cosA;
    };
    rotatePoint(p1); rotatePoint(p2); rotatePoint(location);
}

void Dimension::scale(const Point2D& base, double factor) {
    p1.x = base.x + (p1.x - base.x) * factor; p1.y = base.y + (p1.y - base.y) * factor;
    p2.x = base.x + (p2.x - base.x) * factor; p2.y = base.y + (p2.y - base.y) * factor;
    location.x = base.x + (location.x - base.x) * factor; location.y = base.y + (location.y - base.y) * factor;
    value *= factor;
}

void Dimension::mirror(const Point2D& axisP1, const Point2D& axisP2) {
    p1 = mirrorPoint(p1, axisP1, axisP2);
    p2 = mirrorPoint(p2, axisP1, axisP2);
    location = mirrorPoint(location, axisP1, axisP2);
}

std::unique_ptr<Entity> Dimension::clone() const {
    auto c = std::make_unique<Dimension>();
    c->p1 = p1; c->p2 = p2; c->location = location;
    c->p3 = p3;
    c->value = value; c->isHorizontal = isHorizontal;
    c->type = type;
    c->isAligned = isAligned;
    c->layerName = layerName;
    return c;
}

void Dimension::copyFrom(const Entity& src) {
    auto& d = dynamic_cast<const Dimension&>(src);
    p1 = d.p1; p2 = d.p2; location = d.location;
    p3 = d.p3;
    value = d.value; isHorizontal = d.isHorizontal;
    type = d.type;
    isAligned = d.isAligned;
    layerName = d.layerName;
}

std::vector<Point2D> Dimension::getGripPoints() const {
    std::vector<Point2D> grips;
    if (type == DimType::ANGULAR) {
        double arcRadius = std::hypot(p3.x - location.x, p3.y - location.y);
        double angle1 = std::atan2(p1.y - location.y, p1.x - location.x);
        double angle2 = std::atan2(p2.y - location.y, p2.x - location.x);
        grips.push_back(location);
        grips.push_back({location.x + arcRadius * std::cos(angle1), location.y + arcRadius * std::sin(angle1)});
        grips.push_back({location.x + arcRadius * std::cos(angle2), location.y + arcRadius * std::sin(angle2)});
        grips.push_back(p3);
    }
    else if (type == DimType::RADIUS || type == DimType::DIAMETER) {
        grips.push_back(p1);
        grips.push_back(p2);
        grips.push_back(location);
    }
    else if (type == DimType::ALIGNED || isAligned) {
        grips.push_back(p1);
        grips.push_back(p2);
        grips.push_back(location);
    }
    else {
        grips.push_back(p1);
        grips.push_back(p2);
        grips.push_back(location);
    }
    return grips;
}

void Dimension::moveGrip(int index, const Point2D& newPos) {
    if (index == 0) p1 = newPos;
    else if (index == 1) p2 = newPos;
    else if (index == 2) location = newPos;
    else if (index == 3) p3 = newPos;

    if (type == DimType::RADIUS || type == DimType::DIAMETER) {
        value = std::hypot(p2.x - p1.x, p2.y - p1.y);
        if (type == DimType::DIAMETER) value *= 2.0;
    }
    else if (type == DimType::ALIGNED || isAligned) {
        value = std::hypot(p2.x - p1.x, p2.y - p1.y);
    }
    else if (type == DimType::ANGULAR) {
        double angle1 = std::atan2(p1.y - location.y, p1.x - location.x);
        double angle2 = std::atan2(p2.y - location.y, p2.x - location.x);
        double angle = angle2 - angle1;
        if (angle < 0) angle += 2 * PI;
        value = angle * 180.0 / PI;
    }
    else {
        if (isHorizontal) value = std::abs(p2.x - p1.x);
        else value = std::abs(p2.y - p1.y);
    }
}

std::vector<Point2D> Dimension::getSnapPoints() const {
    return {p1, p2, location};
}

nlohmann::json Dimension::toJson() const {
    return {
        {"type", "Dimension"},
        {"dimType", static_cast<int>(type)},
        {"isAligned", isAligned},
        {"p1", {{"x", p1.x}, {"y", p1.y}}},
        {"p2", {{"x", p2.x}, {"y", p2.y}}},
        {"p3", {{"x", p3.x}, {"y", p3.y}}},
        {"location", {{"x", location.x}, {"y", location.y}}},
        {"value", value},
        {"isHorizontal", isHorizontal},
        {"layer", layerName}
    };
}

} // namespace cad