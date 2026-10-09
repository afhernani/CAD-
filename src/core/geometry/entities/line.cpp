#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
//#include <numbers>
#include <algorithm>

namespace cad {
    // --- Line ---
    void Line::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                    const sf::Color& color, float viewScale) const {
        sf::Vertex pts[] = {
            sf::Vertex(w2s(p1.x, p1.y), color),
            sf::Vertex(w2s(p2.x, p2.y), color)
        };
        window.draw(pts, 2, sf::Lines);
    }

    bool Line::isNear(const Point2D& point, double tolerance) const {
        // Distancia punto a segmento
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double lenSq = dx * dx + dy * dy;
        
        if (lenSq == 0.0) return std::hypot(point.x - p1.x, point.y - p1.y) <= tolerance;

        double t = ((point.x - p1.x) * dx + (point.y - p1.y) * dy) / lenSq;
        t = std::max(0.0, std::min(1.0, t)); // Clamp entre 0 y 1

        double projX = p1.x + t * dx;
        double projY = p1.y + t * dy;

        double distSq = (point.x - projX) * (point.x - projX) + (point.y - projY) * (point.y - projY);
        return distSq <= (tolerance * tolerance);
    }

    void Line::move(double dx, double dy) {
        p1.x += dx; p1.y += dy;
        p2.x += dx; p2.y += dy;
    }

    std::unique_ptr<Entity> Line::clone() const {
        auto c = std::make_unique<Line>();
        c->p1 = p1; c->p2 = p2;
        c->layerName = layerName;
        c->id = id; // Copiar el mismo ID, o generar uno nuevo si es necesario
        return c;
    }
    void Line::rotate(const Point2D& center, double angleDeg) {
        const double PI = 3.14159265358979323846;
        double rad = angleDeg * PI / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        auto rot = [&](Point2D& p) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        };
        rot(p1); rot(p2);
    }

    void Line::scale(const Point2D& basePoint, double factor) {
        p1.x = basePoint.x + (p1.x - basePoint.x) * factor;
        p1.y = basePoint.y + (p1.y - basePoint.y) * factor;
        p2.x = basePoint.x + (p2.x - basePoint.x) * factor;
        p2.y = basePoint.y + (p2.y - basePoint.y) * factor;
    }

    void Line::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        p1 = reflectPoint(p1, axisP1, axisP2);
        p2 = reflectPoint(p2, axisP1, axisP2);
    }

    // --- MÉTODO TRIM PARA LINE ---
    void Line::trim(const Point2D& cutPoint, bool keepStart) {
        double d1 = std::hypot(cutPoint.x - p1.x, cutPoint.y - p1.y);
        double d2 = std::hypot(cutPoint.x - p2.x, cutPoint.y - p2.y);
        
        if (keepStart) {
            // Mantener p1, mover p2 al punto de corte
            p2 = cutPoint;
        } else {
            // Mantener p2, mover p1 al punto de corte
            p1 = cutPoint;
        }
    }

    // --- MÉTODO EXTEND PARA LINE ---
    void Line::extend(const Point2D& borderPoint) {
        // Alargar la línea hasta borderPoint en la dirección del segmento
        double dx = p2.x - p1.x, dy = p2.y - p1.y;
        double len = std::hypot(dx, dy);
        if (len < 1e-10) return;
        
        // Determinar qué extremo está más cerca del borderPoint
        double d1 = std::hypot(borderPoint.x - p1.x, borderPoint.y - p1.y);
        double d2 = std::hypot(borderPoint.x - p2.x, borderPoint.y - p2.y);
        
        if (d1 < d2) {
            // Alargar desde p1
            p1 = borderPoint;
        } else {
            // Alargar desde p2
            p2 = borderPoint;
        }
    }

    // --- Implementaciones de Grips y CopyFrom ---

    // LINE
    std::vector<Point2D> Line::getGripPoints() const { return {p1, p2}; }
    void Line::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) p1 = newPos; else if (index == 1) p2 = newPos;
    }
    void Line::copyFrom(const Entity& src) {
        auto& l = dynamic_cast<const Line&>(src);
        p1 = l.p1; p2 = l.p2; layerName = l.layerName; id = l.id; // Copiar el mismo ID, o generar uno nuevo si es necesario
    }
    
    std::vector<Point2D> Line::getSnapPoints() const {
        Point2D mid = {(p1.x + p2.x) / 2.0, (p1.y + p2.y) / 2.0};
        return {p1, p2, mid}; // Extremos y Punto Medio
    }

    // --- Implementaciones JSON ---
    nlohmann::json Line::toJson() const {
        return {{"type", "Line"}, {"p1", {{"x", p1.x}, {"y", p1.y}}}, {"p2", {{"x", p2.x}, {"y", p2.y}}}, {"layer", layerName},{"id", id}};
    }


}