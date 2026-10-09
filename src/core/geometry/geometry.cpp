#include "cad/core/geometry/geometry.hpp"
#include "cad/core/math/math.hpp" 
#include <cmath>
#include <numbers>
#include <algorithm>

namespace cad {

    // Función factoría para reconstruir entidades desde JSON
    std::unique_ptr<Entity> Entity::fromJson(const nlohmann::json& j) {
        std::string type = j.value("type", "");
        std::string layer = j.value("layer", "0");

        if (type == "Line") {
            auto e = std::make_unique<Line>();
            e->p1 = {j["p1"]["x"].get<double>(), j["p1"]["y"].get<double>()};
            e->p2 = {j["p2"]["x"].get<double>(), j["p2"]["y"].get<double>()};
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            return e;
        }
        else if (type == "Circle") {
            auto e = std::make_unique<Circle>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->radius = j["radius"].get<double>();
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            return e;
        }
        else if (type == "Arc") {
            auto e = std::make_unique<Arc>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->radius = j["radius"].get<double>();
            e->startAngle = j["startAngle"].get<double>();
            e->endAngle = j["endAngle"].get<double>();
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            return e;
        }
        else if (type == "Polyline") {
            auto e = std::make_unique<Polyline>();
            e->closed = j.value("closed", false);
            for (const auto& pt : j["points"]) {
                e->points.push_back({pt["x"].get<double>(), pt["y"].get<double>()});
            }
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            return e;
        }
        else if (type == "Polygon") return Polygon::fromJson(j);
        else if (type == "Ellipse") {
            auto e = std::make_unique<Ellipse>();
            e->center = {j["center"]["x"].get<double>(), j["center"]["y"].get<double>()};
            e->majorRadius = j["majorRadius"].get<double>();
            e->minorRadius = j["minorRadius"].get<double>();
            e->rotationAngle = j.value("rotationAngle", 0.0);
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            return e;
        }
        // >>> COTA <<<
        else if (type == "Dimension") {
            auto e = std::make_unique<Dimension>();
            e->p1 = {j["p1"]["x"].get<double>(), j["p1"]["y"].get<double>()};
            e->p2 = {j["p2"]["x"].get<double>(), j["p2"]["y"].get<double>()};
            e->location = {j["location"]["x"].get<double>(), j["location"]["y"].get<double>()};
            e->value = j["value"].get<double>();
            e->isHorizontal = j.value("isHorizontal", false);
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            // Cargar tipo de cota
            e->type = static_cast<DimType>(j.value("dimType", 0));
            e->isAligned = j.value("isAligned", false);
            if (j.contains("p3")) {
                e->p3 = {j["p3"]["x"].get<double>(), j["p3"]["y"].get<double>()};
            }
            return e;
        }
        else if (type == "BlockInsert") {
            auto e = std::make_unique<BlockInsert>();
            e->insertPoint = {j["insertPoint"]["x"].get<double>(), j["insertPoint"]["y"].get<double>()};
            e->layerName = layer;
            e->blockScale = j.value("scale", 1.0);
            e->blockRotation = j.value("rotation", 0.0);
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            //La definición se resuelve después de cargar todo (ver Corrección 6)
            return e;
        }
        // >>> SOPORTE PARA TEXTO <<<
        else if (type == "Text") {
            auto e = std::make_unique<Text>();
            e->position = {j["position"]["x"].get<double>(), j["position"]["y"].get<double>()};
            e->content = j.value("content", "");
            e->height = j.value("height", 1.0);
            e->rotation = j.value("rotation", 0.0);
            e->layerName = layer;
            e->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
            return e;
        }
        else if (type == "Hatch") return Hatch::fromJson(j);
        return nullptr; // Tipo desconocido
    }
 
} // namespace cad