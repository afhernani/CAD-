#include "cad/core/geometry/entities/hatch.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

    // Función auxiliar: calcular intersección de dos segmentos
    namespace {
        bool segmentIntersection(const Point2D& p1, const Point2D& p2, 
                                 const Point2D& p3, const Point2D& p4, 
                                 Point2D& intersection) {
            double denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
            if (std::abs(denom) < 1e-10) return false; // Paralelos
            
            double ua = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x)) / denom;
            double ub = ((p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x)) / denom;
            
            if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
                intersection.x = p1.x + ua * (p2.x - p1.x);
                intersection.y = p1.y + ua * (p2.y - p1.y);
                return true;
            }
            return false;
        }
    }

    void Hatch::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                     const sf::Color& color, float viewScale) const {
        if (points.size() < 3) return;

        if (pattern == HatchPattern::SOLID) {
            sf::ConvexShape shape;
            shape.setPointCount(points.size());
            for (size_t i = 0; i < points.size(); ++i) {
                shape.setPoint(i, w2s(points[i].x, points[i].y));
            }
            sf::Color fillColor = color;
            fillColor.a = 150; 
            shape.setFillColor(fillColor);
            window.draw(shape);
            return;
        }

        // Calcular Bounding Box y Centro
        double minX = points[0].x, maxX = points[0].x;
        double minY = points[0].y, maxY = points[0].y;
        for (const auto& p : points) {
            minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        }
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        double diagLen = std::hypot(maxX - minX, maxY - minY);

        sf::VertexArray lines(sf::Lines);
        double rad = angle * 3.14159265358979323846 / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);

        // Función para rotar un punto alrededor del centro (cx, cy)
        auto rotatePoint = [&](double x, double y) -> Point2D {
            double dx = x - cx, dy = y - cy;
            return { cx + dx * cosA - dy * sinA, cy + dx * sinA + dy * cosA };
        };

        // Generar líneas paralelas cubriendo el bounding box
        double realSpacing = spacing * 5.0; // Escala visual
        for (double d = -diagLen; d <= diagLen; d += realSpacing) {
            // Línea base horizontal que cubre todo el ancho
            Point2D p1 = { cx - diagLen, cy + d };
            Point2D p2 = { cx + diagLen, cy + d };

            // Rotar la línea según el ángulo definido
            Point2D rp1 = rotatePoint(p1.x, p1.y);
            Point2D rp2 = rotatePoint(p2.x, p2.y);

            // Comprobar si el punto medio está dentro del polígono
            Point2D mid = { (rp1.x + rp2.x) / 2.0, (rp1.y + rp2.y) / 2.0 };
            if (isPointInPolygon(mid)) {
                lines.append(sf::Vertex(w2s(rp1.x, rp1.y), color));
                lines.append(sf::Vertex(w2s(rp2.x, rp2.y), color));
            }
        }
        window.draw(lines);
    }

    bool Hatch::isPointInPolygon(const Point2D& p) const {
        bool inside = false;
        for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
            if (((points[i].y > p.y) != (points[j].y > p.y)) &&
                (p.x < (points[j].x - points[i].x) * (p.y - points[i].y) / (points[j].y - points[i].y) + points[i].x)) {
                inside = !inside;
            }
        }
        return inside;
    }
    
    bool Hatch::isNear(const Point2D& point, double tolerance) const {
        // Primero comprobamos si está dentro del polígono
        if (isPointInPolygon(point)) return true;
        
        // Si no, comprobamos si está cerca de algún borde (opcional, para seleccionar el contorno)
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            // Distancia punto a segmento (simplificada aquí como distancia a los puntos)
            double d1 = std::hypot(point.x - points[i].x, point.y - points[i].y);
            if (d1 < tolerance) return true;
        }
        return false;
    }

    void Hatch::move(double dx, double dy) {
        for (auto& p : points) { p.x += dx; p.y += dy; }
    }

    std::unique_ptr<Entity> Hatch::clone() const {
        auto h = std::make_unique<Hatch>();
        h->copyFrom(*this);
        return h;
    }

    void Hatch::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * 3.14159265358979323846 / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        for (auto& p : points) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        }
    }

    void Hatch::scale(const Point2D& basePoint, double factor) {
        for (auto& p : points) {
            p.x = basePoint.x + (p.x - basePoint.x) * factor;
            p.y = basePoint.y + (p.y - basePoint.y) * factor;
        }
    }

    void Hatch::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x, dy = axisP2.y - axisP1.y;
        double len2 = dx*dx + dy*dy;
        if (len2 == 0) return;
        for (auto& p : points) {
            double t = ((p.x - axisP1.x)*dx + (p.y - axisP1.y)*dy) / len2;
            double projX = axisP1.x + t*dx, projY = axisP1.y + t*dy;
            p.x = 2*projX - p.x;
            p.y = 2*projY - p.y;
        }
    }

    std::vector<Point2D> Hatch::getGripPoints() const { return points; }
    std::vector<Point2D> Hatch::getSnapPoints() const { return points; }
    
    void Hatch::moveGrip(int index, const Point2D& newPos) {
        if (index >= 0 && index < points.size()) points[index] = newPos;
    }

    void Hatch::copyFrom(const Entity& src) {
        const Hatch* h = dynamic_cast<const Hatch*>(&src);
        if (h) {
            layerName = h->layerName;
            points = h->points;
            pattern = h->pattern;
            patternScale = h->patternScale;
            angle = h->angle;      // >>> NUEVO
            spacing = h->spacing;  // >>> NUEVO
        }
    }

    nlohmann::json Hatch::toJson() const {
        nlohmann::json j;
        j["type"] = "Hatch";
        j["layer"] = layerName;
        j["pattern"] = static_cast<int>(pattern);
        j["patternScale"] = patternScale;
        j["angle"] = angle;      // >>> NUEVO
        j["spacing"] = spacing;  // >>> NUEVO
        j["points"] = nlohmann::json::array();
        for (const auto& p : points) {
            j["points"].push_back({{"x", p.x}, {"y", p.y}});
        }
        return j;
    }

    std::unique_ptr<Hatch> Hatch::fromJson(const nlohmann::json& j) {
        auto h = std::make_unique<Hatch>();
        h->layerName = j.value("layer", "0");
        h->pattern = static_cast<HatchPattern>(j.value("pattern", 1));
        h->patternScale = j.value("patternScale", 1.0);
        h->angle = j.value("angle", 0.0);      // >>> NUEVO
        h->spacing = j.value("spacing", 1.0);  // >>> NUEVO
        if (j.contains("points")) {
            for (const auto& p : j["points"]) {
                h->points.push_back({p["x"].get<double>(), p["y"].get<double>()});
            }
        }
        return h;
    }

} // namespace cad