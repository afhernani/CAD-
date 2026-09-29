// include/cad/core/geometry/entities/polyline.hpp
#pragma once
#include "../entity.hpp"

namespace cad {

class Polyline : public Entity {
public:
    std::vector<Point2D> points;

    void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
             const sf::Color& color, float viewScale) const override;
    bool isNear(const Point2D& point, double tolerance) const override;
    void move(double dx, double dy) override;
    std::unique_ptr<Entity> clone() const override;
    void rotate(const Point2D& center, double angleDeg) override;
    void scale(const Point2D& basePoint, double factor) override;
    void mirror(const Point2D& axisP1, const Point2D& axisP2) override;

    std::vector<Point2D> getGripPoints() const override;
    std::vector<Point2D> getSnapPoints() const override;
    void moveGrip(int index, const Point2D& newPos) override;
    void copyFrom(const Entity& src) override;
    nlohmann::json toJson() const override;
};

} // namespace cad