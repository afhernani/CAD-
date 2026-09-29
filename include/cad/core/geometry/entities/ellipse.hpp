// include/cad/core/geometry/entities/ellipse.hpp
#pragma once
#include "../entity.hpp"

namespace cad {

class Ellipse : public Entity {
public:
    Point2D center;
    double majorRadius = 1.0;
    double minorRadius = 0.5;
    double rotationAngle = 0.0; // en radianes

    void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
             const sf::Color& color, float viewScale) const override;
    bool isNear(const Point2D& point, double tolerance) const override;
    void move(double dx, double dy) override;
    void rotate(const Point2D& center, double angleDeg) override;
    void scale(const Point2D& base, double factor) override;
    void mirror(const Point2D& axisP1, const Point2D& axisP2) override;
    std::unique_ptr<Entity> clone() const override;
    void copyFrom(const Entity& src) override;

    std::vector<Point2D> getGripPoints() const override;
    std::vector<Point2D> getSnapPoints() const override;
    void moveGrip(int index, const Point2D& newPos) override;
    nlohmann::json toJson() const override;

private:
    Point2D getPointOnEllipse(double angle) const;
};

} // namespace cad