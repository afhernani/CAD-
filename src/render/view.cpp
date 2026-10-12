#include "cad/render/view.hpp"
#include "cad/core/geometry/rect.hpp"
#include "cad/core/constants.hpp" // Para WINDOW_WIDTH, MENU_HEIGHT, etc.
#include <algorithm>

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

        Rect View::getVisibleWorldBounds() const {
        // Calculamos las esquinas de la zona de dibujo (Canvas) en píxeles
        float screenLeft = 0.0f;
        float screenTop = static_cast<float>(MENU_HEIGHT + TOOLBAR_HEIGHT);
        float screenRight = static_cast<float>(WINDOW_WIDTH);
        float screenBottom = static_cast<float>(WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT);

        // Convertimos las esquinas a coordenadas del mundo
        Point2D worldTopLeft = screenToWorld(screenLeft, screenTop);
        Point2D worldBottomRight = screenToWorld(screenRight, screenBottom);

        // Devolvemos el Rect (asegurando min/max correctos por si el zoom invierte ejes)
        return Rect(worldTopLeft.x, worldTopLeft.y, worldBottomRight.x, worldBottomRight.y);
    }

} // namespace cad