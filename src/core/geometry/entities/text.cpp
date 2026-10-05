#include "cad/core/geometry/entities/text.hpp"
#include <cmath>
#include <iostream>

namespace cad {

    void Text::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                    const sf::Color& color, float viewScale) const {
        // Dibujamos una pequeña cruz en el punto de inserción como referencia visual
        sf::Vector2f pos = w2s(position.x, position.y);
        sf::Vertex v1, v2, v3, v4;
        v1.position = {pos.x - 5, pos.y}; v1.color = color;
        v2.position = {pos.x + 5, pos.y}; v2.color = color;
        v3.position = {pos.x, pos.y - 5}; v3.color = color;
        v4.position = {pos.x, pos.y + 5}; v4.color = color;
        sf::Vertex lines[] = {v1, v2, v3, v4};
        window.draw(lines, 4, sf::Lines);
    }

    bool Text::isNear(const Point2D& point, double tolerance) const {
        // 1. Estimación del ancho del texto: 
        // Cada carácter ocupa aproximadamente el 60% de la altura en fuentes estándar.
        double textWidth = content.length() * height * 0.6;
        double textHeight = height;
        
        // 2. Definir la caja delimitadora (Bounding Box) con la tolerancia incluida.
        // El texto en SFML se dibuja alineado abajo-izquierda desde 'position'.
        double minX = position.x - tolerance;
        double maxX = position.x + textWidth + tolerance;
        double minY = position.y - textHeight - tolerance;
        double maxY = position.y + tolerance;
        
        // 3. Comprobar si el punto del clic está dentro de este rectángulo
        return (point.x >= minX && point.x <= maxX && 
                point.y >= minY && point.y <= maxY);
    }

    void Text::move(double dx, double dy) {
        position.x += dx;
        position.y += dy;
    }

    std::unique_ptr<Entity> Text::clone() const {
        auto t = std::make_unique<Text>();
        t->copyFrom(*this);
        return t;
    }

    void Text::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * 3.14159265358979323846 / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        double dx = position.x - center.x, dy = position.y - center.y;
        position.x = center.x + dx * cosA - dy * sinA;
        position.y = center.y + dx * sinA + dy * cosA;
        rotation += rad;
    }

    void Text::scale(const Point2D& basePoint, double factor) {
        position.x = basePoint.x + (position.x - basePoint.x) * factor;
        position.y = basePoint.y + (position.y - basePoint.y) * factor;
        height *= factor;
    }

    void Text::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x, dy = axisP2.y - axisP1.y;
        double len2 = dx*dx + dy*dy;
        if (len2 == 0) return;
        double t = ((position.x - axisP1.x)*dx + (position.y - axisP1.y)*dy) / len2;
        double projX = axisP1.x + t*dx, projY = axisP1.y + t*dy;
        position.x = 2*projX - position.x;
        position.y = 2*projY - position.y;
        rotation = -rotation; // Invertir rotación al reflejar
    }

    std::vector<Point2D> Text::getGripPoints() const { return {position}; }
    std::vector<Point2D> Text::getSnapPoints() const { return {position}; }
    
    void Text::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) position = newPos;
    }

    void Text::copyFrom(const Entity& src) {
        const Text* t = dynamic_cast<const Text*>(&src);
        if (t) {
            layerName = t->layerName;
            position = t->position; content = t->content;
            height = t->height; rotation = t->rotation;
        }
    }

    nlohmann::json Text::toJson() const {
        std::cout << "[DEBUG] Serializando Text a JSON: '" << content << "'" << std::endl;
        
        nlohmann::json j;
        j["type"] = "Text";
        j["layer"] = layerName;
        j["position"] = {{"x", position.x}, {"y", position.y}};
        j["content"] = content;
        j["height"] = height;
        j["rotation"] = rotation;
        return j;
    }

    std::unique_ptr<Text> Text::fromJson(const nlohmann::json& j) {
        auto t = std::make_unique<Text>();
        t->layerName = j.value("layer", "0");
        if (j.contains("position")) { t->position.x = j["position"]["x"].get<double>(); t->position.y = j["position"]["y"].get<double>(); }
        t->content = j.value("content", "");
        t->height = j.value("height", 1.0);
        t->rotation = j.value("rotation", 0.0);
        return t;
    }

} // namespace cad