#pragma once
#include "../entity.hpp"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include "json.hpp"

namespace cad {

class ImageEntity : public Entity {
public:
    // Variables de datos
    Point2D insertPoint; // Esquina inferior izquierda en coordenadas de mundo
    double width = 10.0;
    double height = 10.0;
    double rotation = 0.0; // En grados
    std::string imagePath;

    ImageEntity();

    // Métodos virtuales de Entity
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

private:
    // Variables internas de gestión de textura
    mutable std::shared_ptr<sf::Texture> texture;
    mutable bool textureLoadAttempted = false;
    mutable double aspectRatio_ = 1.0; // Relación ancho/alto original

    // Método helper para cargar la textura de forma perezosa
    bool loadTexture() const;
};

} // namespace cad