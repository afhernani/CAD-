#pragma once
#include "../entity.hpp"
#include "../point.hpp"
#include <string>

namespace cad {

    class Text : public Entity {
    public:
        Point2D position;
        std::string content;
        double height = 1.0;
        double rotation = 0.0; // En radianes

        Text() = default;
        ~Text() override = default;

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
        static std::unique_ptr<Text> fromJson(const nlohmann::json& j);
    };

} // namespace cad