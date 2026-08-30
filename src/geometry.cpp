#include "geometry.hpp"
#include <cmath>
#include <numbers>
#include <algorithm>

namespace cad {

    // --- FUNCIÓN AUXILIAR (Privada del archivo) ---
    // Calcula la distancia mínima entre un punto 'p' y el segmento 'a'-'b'
    namespace {
        double distToSegment(const Point2D& p, const Point2D& a, const Point2D& b) {
            double dx = b.x - a.x;
            double dy = b.y - a.y;
            double lenSq = dx * dx + dy * dy;
            
            if (lenSq == 0.0) return std::hypot(p.x - a.x, p.y - a.y);

            double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / lenSq;
            t = std::max(0.0, std::min(1.0, t)); // Clamp entre 0 y 1

            double projX = a.x + t * dx;
            double projY = a.y + t * dy;

            return std::hypot(p.x - projX, p.y - projY);
        }
    }

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

    // --- Circle ---
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
        // Está cerca si la distancia al centro es aprox el radio (tolerancia)
        return std::abs(dist - radius) <= tolerance;
    }

    void Circle::move(double dx, double dy) {
        center.x += dx; center.y += dy;
    }

    // --- Arc (Arco) ---
    void Arc::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                   const sf::Color& color, float viewScale) const {
        // Usamos 64 puntos para suavidad
        const int numPoints = 64;
        sf::VertexArray va(sf::LineStrip, numPoints);
        
        // Convertir grados a radianes
        double startRad = startAngle * std::numbers::pi / 180.0;
        double endRad = endAngle * std::numbers::pi / 180.0;
        
        // Calcular el paso angular
        // Nota: Si el arco cruza el 0/360, la lógica simple de interpolación lineal 
        // podría dibujar la línea incorrecta. Para simplificar, asumimos barrido directo.
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
        // Comprobamos si está sobre el anillo del círculo (radio +/- tolerancia)
        // Nota: Para ser más estrictos podríamos comprobar el ángulo, pero para 
        // selección básica en CAD, comprobar el radio suele ser suficiente y rápido.
        return std::abs(dist - radius) <= tolerance;
    }

    void Arc::move(double dx, double dy) {
        center.x += dx; 
        center.y += dy;
        // El radio y los ángulos no cambian al mover.
    }

    // --- Polyline (Polilínea) ---
    void Polyline::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                        const sf::Color& color, float viewScale) const {
        if (points.size() < 2) return;

        sf::VertexArray va(sf::LineStrip, points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            va[i].position = w2s(points[i].x, points[i].y);
            va[i].color = color;
        }
        window.draw(va);
    }

    bool Polyline::isNear(const Point2D& point, double tolerance) const {
        for (size_t i = 0; i + 1 < points.size(); ++i) {
            if (distToSegment(point, points[i], points[i+1]) <= tolerance) {
                return true;
            }
        }
        return false;
    }

    void Polyline::move(double dx, double dy) {
        for (auto& p : points) {
            p.x += dx;
            p.y += dy;
        }
    }

    // --- Polygon (Polígono Regular) ---
    void Polygon::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                       const sf::Color& color, float viewScale) const {
        if (sides < 3) return;

        // sides + 1 para cerrar el polígono (volver al primer punto)
        sf::VertexArray va(sf::LineStrip, sides + 1);
        double angleStep = 2.0 * std::numbers::pi / sides;

        for (int i = 0; i <= sides; ++i) {
            // Empezamos en -90 grados (arriba) para que el polígono se vea "de pie"
            double angle = i * angleStep - std::numbers::pi / 2.0;
            double px = center.x + radius * std::cos(angle);
            double py = center.y + radius * std::sin(angle);
            
            va[i].position = w2s(px, py);
            va[i].color = color;
        }
        window.draw(va);
    }

    bool Polygon::isNear(const Point2D& point, double tolerance) const {
        // Un polígono es un bucle cerrado de segmentos.
        // Calculamos los vértices y comprobamos distancia a cada lado.
        double angleStep = 2.0 * std::numbers::pi / sides;
        
        Point2D prev, curr;
        for (int i = 0; i <= sides; ++i) {
            double angle = i * angleStep - std::numbers::pi / 2.0;
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
        // El radio y los lados no cambian al mover.
    }

} // namespace cad