#pragma once
#include "../geometry/entity.hpp"
#include <string>
#include <vector>
#include <memory>

namespace cad {

struct BlockDefinition {
    std::string name;
    Point2D basePoint = {0.0, 0.0};
    std::vector<std::unique_ptr<Entity>> entities;
};

} // namespace cad