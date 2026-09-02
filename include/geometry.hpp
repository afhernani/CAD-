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
        // Métodos virtuales para grips y copia
        virtual std::vector<Point2D> getGripPoints() const = 0;
        virtual void moveGrip(int index, const Point2D& newPos) = 0;
        virtual void copyFrom(const Entity& src) = 0; // Para poder cancelar con ESC
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
        // Métodos para grips y copyfrom
        virtual std::vector<Point2D> getGripPoints() const override;
        virtual void moveGrip(int index, const Point2D& newPos) override;
        virtual void copyFrom(const Entity& src) override; // Para poder cancelar con ESC
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
        // Métodos para grips y copyfrom
        virtual std::vector<Point2D> getGripPoints() const override;
        virtual void moveGrip(int index, const Point2D& newPos) override;
        virtual void copyFrom(const Entity& src) override; // Para poder cancelar con ESC
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
        // Métodos para grips y copyfrom
        virtual std::vector<Point2D> getGripPoints() const override;
        virtual void moveGrip(int index, const Point2D& newPos) override;
        virtual void copyFrom(const Entity& src) override; // Para poder cancelar con ESC
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
        // Métodos para grips y copyfrom
        virtual std::vector<Point2D> getGripPoints() const override;
        virtual void moveGrip(int index, const Point2D& newPos) override;
        virtual void copyFrom(const Entity& src) override; // Para poder cancelar con ESC
    };

    class Polygon : public Entity {
    public:
        Point2D center;
        int sides = 3;
        double radius = 0.0;
        double rotationOffset = 0.0; // En radianes
        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                  const sf::Color& color, float viewScale) const override;
        // Metodos virtuales:
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& basePoint, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
        // Métodos para grips y copyfrom
        virtual std::vector<Point2D> getGripPoints() const override;
        virtual void moveGrip(int index, const Point2D& newPos) override;
        virtual void copyFrom(const Entity& src) override; // Para poder cancelar con ESC
    };

    // Funciones auxiliares de intersección
    struct IntersectionResult {
        bool intersects = false;
        Point2D point;
        double param = 0.0; // Parámetro t en [0,1] para segmentos
    };
    
    IntersectionResult lineLineIntersection(const Point2D& a1, const Point2D& a2,
                                           const Point2D& b1, const Point2D& b2);

    class Ellipse : public Entity {
    public:
        Point2D center;
        double majorRadius = 1.0;  // Semi-eje mayor
        double minorRadius = 0.5;  // Semi-eje menor
        double rotationAngle = 0.0; // En radianes
        
        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                const sf::Color& color, float viewScale) const override;
        bool isNear(const Point2D& point, double tolerance) const override;
        void move(double dx, double dy) override;
        void rotate(const Point2D& center, double angleDeg) override;
        void scale(const Point2D& base, double factor) override;
        void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
        std::unique_ptr<Entity> clone() const override;
        void copyFrom(const Entity& src) override;
        
        // Para grips
        std::vector<Point2D> getGripPoints() const override;
        void moveGrip(int index, const Point2D& newPos) override;
        
    private:
        // Convierte coordenadas polares de la elipse a cartesianas
        Point2D getPointOnEllipse(double angle) const;
    };

} // namespace cad