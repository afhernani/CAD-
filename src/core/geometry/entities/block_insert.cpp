// src/core/geometry/entities/block_insert.cpp
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/core/math/math.hpp"
#include <cmath>
#include <algorithm>
#include <limits>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    void BlockInsert::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                        const sf::Color& color, float viewScale) const {
        if (!definition) return;
        if (definition->entities.empty()) return;

        for (const auto& e : definition->entities) {
            auto tempCopy = e->clone();
            // Las entidades están en -basePoint, solo sumamos insertPoint
            tempCopy->move(insertPoint.x, insertPoint.y);
            tempCopy->draw(window, w2s, color, viewScale);
        }
    }

    bool BlockInsert::isNear(const Point2D& point, double tolerance) const {
        if (!definition || definition->entities.empty()) {
            // Si no hay definición, usar el insertPoint como fallback
            double dx = point.x - insertPoint.x;
            double dy = point.y - insertPoint.y;
            return std::sqrt(dx * dx + dy * dy) <= tolerance;
        }
        // >>> CORRECCIÓN: Calcular bounding box de las entidades desplazadas <<<
        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double maxY = std::numeric_limits<double>::lowest();

        for (const auto& e : definition->entities) {
            auto snaps = e->getSnapPoints();
            for (const auto& pt : snaps) {
                // Desplazar los puntos por insertPoint
                double px = pt.x + insertPoint.x;
                double py = pt.y + insertPoint.y;
                minX = std::min(minX, px);
                minY = std::min(minY, py);
                maxX = std::max(maxX, px);
                maxY = std::max(maxY, py);
            }
        }
        // Verificar si el punto está dentro del bounding box (con tolerancia)
        return point.x >= minX - tolerance && point.x <= maxX + tolerance &&
            point.y >= minY - tolerance && point.y <= maxY + tolerance;
    }

    void BlockInsert::move(double dx, double dy) {
        insertPoint.x += dx;
        insertPoint.y += dy;
    }

    std::unique_ptr<Entity> BlockInsert::clone() const {
        auto copy = std::make_unique<BlockInsert>();
        copy->definition = definition;  // Mismo puntero (compartido)
        copy->insertPoint = insertPoint;
        copy->blockScale = blockScale;
        copy->blockRotation = blockRotation;
        copy->layerName = layerName;
        return copy;
    }

    void BlockInsert::rotate(const Point2D& rotCenter, double angleDeg) {
        double rad = angleDeg * PI / 180.0;
        double dx = insertPoint.x - rotCenter.x;
        double dy = insertPoint.y - rotCenter.y;
        insertPoint.x = rotCenter.x + dx * std::cos(rad) - dy * std::sin(rad);
        insertPoint.y = rotCenter.y + dx * std::sin(rad) + dy * std::cos(rad);
        blockRotation += angleDeg;
    }

    void BlockInsert::scale(const Point2D& basePoint, double factor) {
        insertPoint.x = basePoint.x + (insertPoint.x - basePoint.x) * factor;
        insertPoint.y = basePoint.y + (insertPoint.y - basePoint.y) * factor;
        blockScale *= factor;
    }

    void BlockInsert::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x;
        double dy = axisP2.y - axisP1.y;
        double len2 = dx * dx + dy * dy;
        if (len2 > 0) {
            double t = ((insertPoint.x - axisP1.x) * dx + (insertPoint.y - axisP1.y) * dy) / len2;
            insertPoint.x = 2 * (axisP1.x + t * dx) - insertPoint.x;
            insertPoint.y = 2 * (axisP1.y + t * dy) - insertPoint.y;
        }
    }

    std::vector<Point2D> BlockInsert::getGripPoints() const {
        return {insertPoint};
    }

    std::vector<Point2D> BlockInsert::getSnapPoints() const {
        return {insertPoint};
    }

    void BlockInsert::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) {
            insertPoint = newPos;
        }
    }

    void BlockInsert::copyFrom(const Entity& src) {
        if (auto* other = dynamic_cast<const BlockInsert*>(&src)) {
            definition = other->definition;
            insertPoint = other->insertPoint;
            blockScale = other->blockScale;
            blockRotation = other->blockRotation;
            layerName = other->layerName;
        }
    }

    nlohmann::json BlockInsert::toJson() const {
        nlohmann::json j;
        j["type"] = "BlockInsert";
        j["layer"] = layerName;
        j["insertPoint"] = {{"x", insertPoint.x}, {"y", insertPoint.y}};
        j["blockName"] = definition ? definition->name : "";
        j["scale"] = blockScale;
        j["rotation"] = blockRotation;
        return j;
    }

} // namespace cad