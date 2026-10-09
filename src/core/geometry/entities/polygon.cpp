#include "cad/core/geometry/entities/polygon.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    void Polygon::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                       const sf::Color& color, float viewScale) const {
        if (points.size() < 3) return;

        sf::ConvexShape shape;
        shape.setPointCount(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            shape.setPoint(i, w2s(points[i].x, points[i].y));
        }
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(color);
        shape.setOutlineThickness(1.5f);
        window.draw(shape);
    }

    bool Polygon::isNear(const Point2D& point, double tolerance) const {
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            // Distancia punto a segmento
            double dx = points[j].x - points[i].x;
            double dy = points[j].y - points[i].y;
            double len2 = dx*dx + dy*dy;
            if (len2 == 0) {
                if (std::hypot(point.x - points[i].x, point.y - points[i].y) < tolerance) return true;
            } else {
                double t = ((point.x - points[i].x) * dx + (point.y - points[i].y) * dy) / len2;
                t = std::max(0.0, std::min(1.0, t));
                double projX = points[i].x + t * dx;
                double projY = points[i].y + t * dy;
                if (std::hypot(point.x - projX, point.y - projY) < tolerance) return true;
            }
        }
        return false;
    }

    void Polygon::move(double dx, double dy) {
        for (auto& p : points) { p.x += dx; p.y += dy; }
        center.x += dx; center.y += dy;
    }

    std::unique_ptr<Entity> Polygon::clone() const {
        auto p = std::make_unique<Polygon>();
        p->copyFrom(*this);
        return p;
    }

    void Polygon::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * PI / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        for (auto& p : points) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        }
        // Actualizar centro también
        double dx = this->center.x - center.x, dy = this->center.y - center.y;
        this->center.x = center.x + dx * cosA - dy * sinA;
        this->center.y = center.y + dx * sinA + dy * cosA;
    }

    void Polygon::scale(const Point2D& basePoint, double factor) {
        for (auto& p : points) {
            p.x = basePoint.x + (p.x - basePoint.x) * factor;
            p.y = basePoint.y + (p.y - basePoint.y) * factor;
        }
        this->center.x = basePoint.x + (this->center.x - basePoint.x) * factor;
        this->center.y = basePoint.y + (this->center.y - basePoint.y) * factor;
        this->radius *= factor;
    }

    void Polygon::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x, dy = axisP2.y - axisP1.y;
        double len2 = dx*dx + dy*dy;
        if (len2 == 0) return;
        for (auto& p : points) {
            double t = ((p.x - axisP1.x)*dx + (p.y - axisP1.y)*dy) / len2;
            double projX = axisP1.x + t*dx, projY = axisP1.y + t*dy;
            p.x = 2*projX - p.x;
            p.y = 2*projY - p.y;
        }
        double tC = ((center.x - axisP1.x)*dx + (center.y - axisP1.y)*dy) / len2;
        center.x = 2*(axisP1.x + tC*dx) - center.x;
        center.y = 2*(axisP1.y + tC*dy) - center.y;
    }

    std::vector<Point2D> Polygon::getGripPoints() const { return points; }
    std::vector<Point2D> Polygon::getSnapPoints() const { return points; }
    
    void Polygon::moveGrip(int index, const Point2D& newPos) {
        if (index >= 0 && index < points.size()) {
            points[index] = newPos;
            // Recalcular centro y radio aproximado si se mueve un grip
            double sumX = 0, sumY = 0;
            for (const auto& p : points) { sumX += p.x; sumY += p.y; }
            center.x = sumX / points.size();
            center.y = sumY / points.size();
            double maxDist = 0;
            for (const auto& p : points) {
                double d = std::hypot(p.x - center.x, p.y - center.y);
                if (d > maxDist) maxDist = d;
            }
            radius = maxDist;
        }
    }

    void Polygon::copyFrom(const Entity& src) {
        const auto& p = dynamic_cast<const Polygon&>(src);
        center = p.center;
        sides = p.sides;
        radius = p.radius;
        points = p.points;
        layerName = p.layerName;
        id = p.id;
    }

    nlohmann::json Polygon::toJson() const {
        nlohmann::json j;
        j["type"] = "Polygon";
        j["id"] = id;
        j["layer"] = layerName;
        j["center"] = {{"x", center.x}, {"y", center.y}};
        j["sides"] = sides;
        j["radius"] = radius;
        j["points"] = nlohmann::json::array();
        for (const auto& p : points) {
            j["points"].push_back({{"x", p.x}, {"y", p.y}});
        }
        return j;
    }

    std::unique_ptr<Polygon> Polygon::fromJson(const nlohmann::json& j) {
        auto p = std::make_unique<Polygon>();
        p->id = j.value("id", Entity::generateId());
        p->layerName = j.value("layer", "0");
        p->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
        p->sides = j.value("sides", 6);
        p->radius = j.value("radius", 1.0);
        if (j.contains("points")) {
            for (const auto& pt : j["points"]) {
                p->points.push_back({pt["x"].get<double>(), pt["y"].get<double>()});
            }
        }
        return p;
    }

} // namespace cad