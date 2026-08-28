#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>

namespace cad {

    struct Point2D {
        double x = 0.0;
        double y = 0.0;
    };

    using WorldToScreenFn = std::function<sf::Vector2f(double, double)>;

    // Clase base abstracta para todas las entidades
    class Entity {
    public:
        std::string layerName = "0";
        virtual ~Entity() = default;
        virtual void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                         const sf::Color& color, float viewScale) const = 0;
    };

    class Line : public Entity {
    public:
        Point2D p1;
        Point2D p2;

        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                 const sf::Color& color, float viewScale) const override;
    };

    class Circle : public Entity {
    public:
        Point2D center;
        double radius = 0.0;

        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                 const sf::Color& color, float viewScale) const override;
    };

} // namespace cad