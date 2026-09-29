#pragma once
#include <SFML/Graphics.hpp>
#include <functional>

namespace cad {

struct Point2D {
    double x = 0.0;
    double y = 0.0;
};

using WorldToScreenFn = std::function<sf::Vector2f(double, double)>;

} // namespace cad