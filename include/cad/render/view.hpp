#pragma once
#include <SFML/Graphics.hpp>
#include "cad/core/geometry/point.hpp"

namespace cad {

class View {
public:
    View();

    // Transformaciones de coordenadas
    sf::Vector2f worldToScreen(double wx, double wy) const;
    Point2D screenToWorld(float sx, float sy) const;

    // Gestión de cámara
    void zoom(float factor, const sf::Vector2f& mouseScreenPos);
    void pan(const sf::Vector2f& deltaScreen);

    // Getters
    float getScale() const { return scale_; }
    void setScale(float s) { scale_ = s; }
    Point2D getPan() const { return {panX_, panY_}; }
    void setPan(Point2D p) { panX_ = p.x; panY_ = p.y; }

    // >>> NUEVO: Gestión del canvasHeight <<<
    void setCanvasHeight(float height) { canvasHeight_ = height; }
    float getCanvasHeight() const { return canvasHeight_; }

private:
    float scale_ = 1.0f;
    float panX_ = 50.0f;
    float panY_ = 50.0f;
    float canvasHeight_ = 1.0f;
};

} // namespace cad