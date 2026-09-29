// include/cad/core/geometry/entities/dimension.hpp
#pragma once
#include "../entity.hpp"

namespace cad {

class Dimension : public Entity {
public:
    Point2D p1;          // Primer punto a medir
    Point2D p2;          // Segundo punto a medir
    Point2D location;    // Ubicación de la línea de cota
    Point2D p3;
    double value = 0.0;  // Valor medido (calculado automáticamente)
    bool isHorizontal = true; // True si es cota horizontal, False si es vertical
    bool isAligned = false; // True si es cota alineada
    DimType type = DimType::HORIZONTAL; // Tipo de cota

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
    void moveGrip(int index, const Point2D& newPos) override;
    std::vector<Point2D> getSnapPoints() const override;
    nlohmann::json toJson() const override;
};

} // namespace cad