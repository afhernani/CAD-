#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <functional>
#include <vector>

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
        virtual bool isNear(const Point2D& point, double tolerance) const = 0;
        virtual void move(double dx, double dy) = 0;
        virtual std::unique_ptr<Entity> clone() const = 0;
        virtual void rotate(const Point2D& center, double angleDeg) = 0;
        virtual void scale(const Point2D& basePoint, double factor) = 0;
        virtual void mirror(const Point2D& axisP1, const Point2D& axisP2) = 0;
    };

    class Line : public Entity {
    public:
        Point2D p1;
        Point2D p2;

        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                 const sf::Color& color, float viewScale) const override;
        // Metodos virtuales:
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& basePoint, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
        void trim(const Point2D& cutPoint, bool keepStart);
        void extend(const Point2D& borderPoint);
    };

    class Circle : public Entity {
    public:
        Point2D center;
        double radius = 0.0;

        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                 const sf::Color& color, float viewScale) const override;
        // Metodos virtuales:
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& basePoint, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
    };

    class Arc : public Entity {
    public:
        Point2D center;
        double radius = 0.0;
        double startAngle = 0.0; // en grados
        double endAngle = 0.0;   // en grados
        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                  const sf::Color& color, float viewScale) const override;
        // Metodos virtuales:
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& basePoint, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
    };

    class Polyline : public Entity {
    public:
        std::vector<Point2D> points;
        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                  const sf::Color& color, float viewScale) const override;
        // Metodos virtuales:
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& basePoint, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
    };

    class Polygon : public Entity {
    public:
        Point2D center;
        int sides = 3;
        double radius = 0.0;
        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                  const sf::Color& color, float viewScale) const override;
        // Metodos virtuales:
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& basePoint, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
    };

    // Funciones auxiliares de intersección
    struct IntersectionResult {
        bool intersects = false;
        Point2D point;
        double param = 0.0; // Parámetro t en [0,1] para segmentos
    };
    
    IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2,
                                           const Point2D& b1, const Point2D& b2);

} // namespace cad