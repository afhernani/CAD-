#include "cad/core/geometry/entities/image.hpp"
#include "cad/core/math/math.hpp" // Para reflectPoint
#include <cmath>
#include <filesystem>
#include <algorithm>
#include <iostream>

namespace cad {

    ImageEntity::ImageEntity() {
        layerName = "0";
    }

    bool ImageEntity::loadTexture() const {
        if (textureLoadAttempted) return texture != nullptr;
        textureLoadAttempted = true;

        if (imagePath.empty() || !std::filesystem::exists(imagePath)) {
            return false;
        }

        texture = std::make_shared<sf::Texture>();
        if (texture->loadFromFile(imagePath)) {
            // Calcular el aspect ratio original al cargar la textura
            unsigned int pxWidth = texture->getSize().x;
            unsigned int pxHeight = texture->getSize().y;
            if (pxWidth > 0) {
                aspectRatio_ = static_cast<double>(pxHeight) / static_cast<double>(pxWidth);
            }
            return true;
        }
        return false;
    }

    void ImageEntity::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s, 
                           const sf::Color& color, float viewScale) const {
        if (!loadTexture()) return;

        sf::Sprite sprite;
        sprite.setTexture(*texture);

        // Obtener dimensiones originales de la textura
        unsigned int texWidth = texture->getSize().x;
        unsigned int texHeight = texture->getSize().y;

        // Calcular factores de escala considerando el zoom de la vista (viewScale)
        float scaleX = (static_cast<float>(width) * viewScale) / static_cast<float>(texWidth);
        float scaleY = (static_cast<float>(height) * viewScale) / static_cast<float>(texHeight);

        sprite.setScale(scaleX, scaleY);

        // Calcular posición en pantalla del insertPoint (esquina inferior-izquierda en mundo)
        sf::Vector2f screenPos = w2s(insertPoint.x, insertPoint.y);

        // Establecer el origen en la esquina inferior-izquierda de la textura original
        sprite.setOrigin(0.0f, static_cast<float>(texHeight));

        // Posicionar el sprite
        sprite.setPosition(screenPos);

        // Rotación (SFML rota en sentido horario, CAD en antihorario)
        sprite.setRotation(-static_cast<float>(rotation));

        window.draw(sprite);
    }

    bool ImageEntity::isNear(const Point2D& point, double tolerance) const {
        // Comprobación de Bounding Box (caja delimitadora)
        double minX = std::min(insertPoint.x, insertPoint.x + width);
        double maxX = std::max(insertPoint.x, insertPoint.x + width);
        double minY = std::min(insertPoint.y, insertPoint.y + height);
        double maxY = std::max(insertPoint.y, insertPoint.y + height);

        return (point.x >= minX - tolerance && point.x <= maxX + tolerance &&
                point.y >= minY - tolerance && point.y <= maxY + tolerance);
    }

    void ImageEntity::move(double dx, double dy) {
        insertPoint.x += dx;
        insertPoint.y += dy;
    }

    std::unique_ptr<Entity> ImageEntity::clone() const {
        auto img = std::make_unique<ImageEntity>();
        img->insertPoint = insertPoint;
        img->width = width;
        img->height = height;
        img->rotation = rotation;
        img->imagePath = imagePath;
        img->layerName = layerName;
        img->id = id;
        img->aspectRatio_ = aspectRatio_; // Copiar proporción
        return img;
    }

    void ImageEntity::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * 3.14159265358979323846 / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        double dx = insertPoint.x - center.x;
        double dy = insertPoint.y - center.y;
        
        insertPoint.x = center.x + dx * cosA - dy * sinA;
        insertPoint.y = center.y + dx * sinA + dy * cosA;
        
        rotation += angleDeg;
        while (rotation < 0) rotation += 360.0;
        while (rotation >= 360.0) rotation -= 360.0;
    }

    void ImageEntity::scale(const Point2D& basePoint, double factor) {
        insertPoint.x = basePoint.x + (insertPoint.x - basePoint.x) * factor;
        insertPoint.y = basePoint.y + (insertPoint.y - basePoint.y) * factor;
        width *= factor;
        height *= factor;
    }

    void ImageEntity::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        insertPoint = reflectPoint(insertPoint, axisP1, axisP2);
        
        double dx = axisP2.x - axisP1.x;
        double dy = axisP2.y - axisP1.y;
        double axisAngle = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
        
        rotation = 2.0 * axisAngle - rotation;
        while (rotation < 0) rotation += 360.0;
        while (rotation >= 360.0) rotation -= 360.0;
    }

    std::vector<Point2D> ImageEntity::getGripPoints() const {
        return {
            insertPoint, // 0: Inferior izquierda
            {insertPoint.x + width, insertPoint.y}, // 1: Inferior derecha
            {insertPoint.x + width, insertPoint.y + height}, // 2: Superior derecha
            {insertPoint.x, insertPoint.y + height}, // 3: Superior izquierda
            {insertPoint.x + width/2.0, insertPoint.y + height/2.0} // 4: Centro
        };
    }

    std::vector<Point2D> ImageEntity::getSnapPoints() const {
        return getGripPoints();
    }

    void ImageEntity::moveGrip(int index, const Point2D& newPos) {
        if (index == 0) {
            // Esquina inferior izquierda: solo mueve la posición
            insertPoint = newPos;
        } 
        else if (index == 1) {
            // Esquina inferior derecha: cambia el ancho, alto se ajusta
            double newWidth = std::max(1.0, newPos.x - insertPoint.x);
            width = newWidth;
            height = newWidth * aspectRatio_; 
        } 
        else if (index == 2) {
            // Esquina superior derecha: cambia el ancho, alto se ajusta
            double newWidth = std::max(1.0, newPos.x - insertPoint.x);
            width = newWidth;
            height = newWidth * aspectRatio_; 
        } 
        else if (index == 3) {
            // Esquina superior izquierda: cambia el alto, ancho se ajusta
            double newHeight = std::max(1.0, newPos.y - insertPoint.y);
            height = newHeight;
            width = newHeight / aspectRatio_; 
        } 
        else if (index == 4) {
            // Centro: mueve toda la imagen
            double cx = insertPoint.x + width / 2.0;
            double cy = insertPoint.y + height / 2.0;
            insertPoint.x += (newPos.x - cx);
            insertPoint.y += (newPos.y - cy);
        }
    }

    void ImageEntity::copyFrom(const Entity& src) {
        auto& img = dynamic_cast<const ImageEntity&>(src);
        insertPoint = img.insertPoint;
        width = img.width;
        height = img.height;
        rotation = img.rotation;
        imagePath = img.imagePath;
        layerName = img.layerName;
        id = img.id;
        aspectRatio_ = img.aspectRatio_; // Copiar proporción
    }

    nlohmann::json ImageEntity::toJson() const {
        return {
            {"type", "Image"},
            {"insertPoint", {{"x", insertPoint.x}, {"y", insertPoint.y}}},
            {"width", width},
            {"height", height},
            {"rotation", rotation},
            {"imagePath", imagePath},
            {"layer", layerName},
            {"id", id}
        };
    }

} // namespace cad