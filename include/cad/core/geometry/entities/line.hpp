// include/cad/core/geometry/entities/line.hpp
#pragma once
#include "../entity.hpp"

namespace cad {

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
        virtual std::vector<Point2D> getSnapPoints() const override;
        virtual void moveGrip(int index, const Point2D& newPos) override;
        virtual void copyFrom(const Entity& src) override; // Para poder cancelar con ESC
        virtual nlohmann::json toJson() const override;
    };
    
}