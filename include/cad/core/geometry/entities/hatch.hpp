#pragma once
#include "../entity.hpp"
#include "../point.hpp"
#include <vector>

namespace cad {

    enum class HatchPattern { SOLID, DIAGONAL, CROSS };

    class Hatch : public Entity {
    public:
        std::vector<Point2D> points; 
        HatchPattern pattern = HatchPattern::DIAGONAL;
        double patternScale = 1.0; 
        double angle = 0.0;      // >>> NUEVO: Ángulo en grados
        double spacing = 1.0;    // >>> NUEVO: Distancia entre líneas

        Hatch() = default;
        ~Hatch() override = default;

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
        static std::unique_ptr<Hatch> fromJson(const nlohmann::json& j);

    private:
        bool isPointInPolygon(const Point2D& p) const;
    };

} // namespace cad