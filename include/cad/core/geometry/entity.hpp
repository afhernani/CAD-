#pragma once
#include "point.hpp"
#include <string>
#include <vector>
#include <memory>
#include "json.hpp"

namespace cad {

enum class DimType {
    HORIZONTAL,
    VERTICAL,
    ALIGNED,
    RADIUS,
    DIAMETER,
    ANGULAR
};

class Entity {
public:
    std::string id;
    // metodo estatico para generar un ID único para cada entidad.
    static std::string generateId() {
        static uint64_t counter = 1;
        return "ent_" + std::to_string(counter++);
    }
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

    virtual std::vector<Point2D> getGripPoints() const = 0;
    virtual std::vector<Point2D> getSnapPoints() const = 0;
    virtual void moveGrip(int index, const Point2D& newPos) = 0;
    virtual void copyFrom(const Entity& src) = 0;

    virtual void update(const class Document& doc) {} 

    virtual nlohmann::json toJson() const = 0;
    static std::unique_ptr<Entity> fromJson(const nlohmann::json& j);
};

} // namespace cad