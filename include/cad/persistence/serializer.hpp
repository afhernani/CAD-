#pragma once
#include <string>
#include <vector>
#include <memory>
#include "cad/core/geometry/entity.hpp"

namespace cad {

class Serializer {
public:
    // Serializa una lista de entidades a JSON
    static std::string serialize(const std::vector<std::unique_ptr<Entity>>& entities);

    // Deserializa JSON a una lista de entidades
    static std::vector<std::unique_ptr<Entity>> deserialize(const std::string& json);
};

} // namespace cad