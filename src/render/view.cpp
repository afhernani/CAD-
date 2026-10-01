#include "cad/render/view.hpp"

namespace cad {

    View::View() = default;

    sf::Vector2f View::worldToScreen(double wx, double wy) const {
        float sx = (wx + panX_) * scale_;
        float sy = (canvasHeight_ - (wy + panY_) * scale_);
        return {sx, sy};
    }

    Point2D View::screenToWorld(float sx, float sy) const {
        float wx = (sx / scale_) - panX_;
        float wy = ((canvasHeight_ - sy) / scale_) - panY_;
        return {wx, wy};
    }

    void View::zoom(float factor, const sf::Vector2f& mouseScreenPos) {
        // 1. Calcular el punto del mundo bajo el cursor ANTES del zoom
        Point2D worldBefore = screenToWorld(mouseScreenPos.x, mouseScreenPos.y);

        // 2. Aplicar el nuevo scale
        scale_ *= factor;
        if (scale_ < 0.01f) scale_ = 0.01f;
        if (scale_ > 100.0f) scale_ = 100.0f;

        // 3. Recalcular el pan para que worldBefore siga bajo el cursor
        panX_ = (mouseScreenPos.x / scale_) - worldBefore.x;
        panY_ = ((canvasHeight_ - mouseScreenPos.y) / scale_) - worldBefore.y;
    }

    void View::pan(const sf::Vector2f& deltaScreen) {
        panX_ += (deltaScreen.x / scale_);
        panY_ -= (deltaScreen.y / scale_); // Y invertida en SFML
    }

} // namespace cad