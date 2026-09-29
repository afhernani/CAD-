#pragma once
#include <SFML/Graphics.hpp>
#include "cad/core/geometry/point.hpp"

namespace cad {

class View {
public:
    View();

    // Transformaciones de coordenadas
    sf::Vector2f worldToScreen(double wx, double wy, float canvasHeight) const;
    Point2D screenToWorld(float sx, float sy, float canvasHeight) const;

    // Gestión de cámara
    void zoom(float factor, const sf::Vector2f& mouseScreenPos, float canvasHeight);
    void pan(const sf::Vector2f& deltaScreen);

    // Getters
    float getScale() const { return scale_; }
    Point2D getPan() const { return {panX_, panY_}; }
    void setScale(float s) { scale_ = s; }
    void setPan(Point2D p) { panX_ = p.x; panY_ = p.y; }

private:
    float scale_ = 1.0f;
    float panX_ = 50.0f;
    float panY_ = 50.0f;
};

} // namespace cad