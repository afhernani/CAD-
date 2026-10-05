#pragma once
#include <SFML/Graphics.hpp>
#include "cad/commands/engine.hpp"
#include "cad/render/view.hpp"
#include "cad/core/constants.hpp"

namespace cad {

class Renderer {
public:
    Renderer() = default;

    // Método principal de renderizado del canvas
    void render(sf::RenderWindow& window, const View& view, Engine& engine, 
                const sf::Vector2i& mouseScreenPos, const Point2D& mouseWorldPos,
                sf::Font& font, bool isSnapped, const Point2D& snappedPoint, bool showAxes) const;

    void drawSelectionRect(sf::RenderWindow& window, const View& view,
                          const Point2D& startPoint, const Point2D& endPoint) const;

private:
    // Constantes de layout (deben coincidir con App)
    // static constexpr unsigned int WINDOW_WIDTH = 1280;
    // static constexpr unsigned int WINDOW_HEIGHT = 720;
    // static constexpr unsigned int MENU_HEIGHT = 30;
    // static constexpr unsigned int TOOLBAR_HEIGHT = 60;
    // static constexpr unsigned int COMMAND_HEIGHT = 90;
    // static constexpr unsigned int STATUS_HEIGHT = 25;
    // static constexpr float CANVAS_HEIGHT = static_cast<float>(WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT);

    void drawGrid(sf::RenderWindow& window, const View& view, const Engine& engine) const;
    void drawAxes(sf::RenderWindow& window, const View& view) const;
    void drawEntities(sf::RenderWindow& window, const View& view, const Engine& engine) const;
    void drawGrips(sf::RenderWindow& window, const View& view, const Engine& engine) const;
    void drawDimensionTexts(sf::RenderWindow& window, const View& view, const Engine& engine, sf::Font& font) const;
    void drawCrosshair(sf::RenderWindow& window, const View& view, const sf::Vector2i& mouseScreenPos, 
                       const Point2D& mouseWorldPos, bool isSnapped, const Point2D& snappedPoint) const;
    void drawDrawingFeedback(sf::RenderWindow& window, const View& view, Engine& engine, 
                             const Point2D& mouseWorldPos, sf::Font& font) const;
    void drawTexts(sf::RenderWindow& window, const View& view, const Engine& engine, sf::Font& font) const;
};

} // namespace cad