// include/cad/core/geometry/entities/block_insert.hpp
#pragma once
#include "../entity.hpp"
#include "../../document/block.hpp"

namespace cad {

    // struct BlockDefinition {
    //     std::string name;
    //     Point2D basePoint = {0.0, 0.0}; // Punto base del bloque
    //     std::vector<std::unique_ptr<Entity>> entities;
    // };

    class BlockInsert : public Entity {
    public:
        BlockDefinition* definition = nullptr; // Puntero a la definición del bloque
        Point2D insertPoint = {0.0, 0.0}; // Punto de inserción
        
        // >>> VARIABLES DE LA INSTANCIA DEL BLOQUE <<<
        double blockScale = 1.0;      // Escala de la inserción
        double blockRotation = 0.0;   // Rotación en grados
        
        // >>> FIRMA CORREGIDA: coincide exactamente con Entity (incluye 'const' al final) <<<
        void draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
            const sf::Color& color, float viewScale) const override;
    
        bool isNear(const Point2D& p, double tolerance) const override;
        void move(double dx, double dy) override;
        std::unique_ptr<Entity> clone() const override;
        
        // Métodos requeridos por Entity
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