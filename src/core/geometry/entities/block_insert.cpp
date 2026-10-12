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
            
            // 1. Escalar desde el origen local del bloque (0,0)
            tempCopy->scale(Point2D{0.0, 0.0}, blockScale);
            
            // 2. Rotar desde el origen local del bloque (0,0)
            tempCopy->rotate(Point2D{0.0, 0.0}, blockRotation);
            
            // 3. Trasladar al punto de inserción en el mundo
            tempCopy->move(insertPoint.x, insertPoint.y);
            
            tempCopy->draw(window, w2s, color, viewScale);
        }
    }

    bool BlockInsert::isNear(const Point2D& point, double tolerance) const {
        if (!definition || definition->entities.empty()) {
            // Fallback: si no hay entidades, el punto de inserción actúa como punto de anclaje
            return std::hypot(point.x - insertPoint.x, point.y - insertPoint.y) <= tolerance;
        }

        // ✅ CORRECCIÓN: Aplicar las mismas transformaciones que en draw() 
        // y delegar la comprobación a la entidad interna.
        for (const auto& e : definition->entities) {
            auto tempCopy = e->clone();
            
            tempCopy->scale(Point2D{0.0, 0.0}, blockScale);
            tempCopy->rotate(Point2D{0.0, 0.0}, blockRotation);
            tempCopy->move(insertPoint.x, insertPoint.y);
            
            // Si el punto está cerca de CUALQUIER entidad transformada del bloque, el bloque está seleccionado
            if (tempCopy->isNear(point, tolerance)) {
                return true;
            }
        }
        
        return false;
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
        copy->id = id;
        return copy;
    }

    void BlockInsert::rotate(const Point2D& rotCenter, double angleDeg) {
        double rad = angleDeg * PI / 180.0;
        double dx = insertPoint.x - rotCenter.x;
        double dy = insertPoint.y - rotCenter.y;
        insertPoint.x = rotCenter.x + dx * std::cos(rad) - dy * std::sin(rad);
        insertPoint.y = rotCenter.y + dx * std::sin(rad) + dy * std::cos(rad);
        blockRotation += angleDeg;
        
        // Normalizar rotación
        while (blockRotation < 0.0) blockRotation += 360.0;
        while (blockRotation >= 360.0) blockRotation -= 360.0;
    }

    void BlockInsert::scale(const Point2D& basePoint, double factor) {
        insertPoint.x = basePoint.x + (insertPoint.x - basePoint.x) * factor;
        insertPoint.y = basePoint.y + (insertPoint.y - basePoint.y) * factor;
        blockScale *= factor;
    }

    void BlockInsert::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        // Reflejar el punto de inserción
        insertPoint = reflectPoint(insertPoint, axisP1, axisP2);
        
        // Al reflejar, la rotación se invierte respecto al eje de simetría
        double dx = axisP2.x - axisP1.x;
        double dy = axisP2.y - axisP1.y;
        double axisAngle = std::atan2(dy, dx) * 180.0 / PI;
        
        blockRotation = 2.0 * axisAngle - blockRotation;
        while (blockRotation < 0.0) blockRotation += 360.0;
        while (blockRotation >= 360.0) blockRotation -= 360.0;
    }

    std::vector<Point2D> BlockInsert::getGripPoints() const {
        // Por ahora, el grip principal es el punto de inserción para mover el bloque completo
        std::vector<Point2D> grips = {insertPoint};
        
        // Opcional: Si quieres que se puedan seleccionar grips internos, 
        // aquí podrías iterar y transformar los getGripPoints() de las entidades.
        // Para mantenerlo limpio y rápido, el punto de inserción es el estándar.
        return grips;
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
            id = other->id;
        }
    }

    nlohmann::json BlockInsert::toJson() const {
        nlohmann::json j;
        j["type"] = "BlockInsert";
        j["layer"] = layerName;
        j["id"] = id;
        j["insertPoint"] = {{"x", insertPoint.x}, {"y", insertPoint.y}};
        j["blockName"] = definition ? definition->name : "";
        j["scale"] = blockScale;
        j["rotation"] = blockRotation;
        return j;
    }

} // namespace cad