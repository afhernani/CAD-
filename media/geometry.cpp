#include "geometry.hpp"
#include <cmath>
#include <numbers>

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

} // namespace cad