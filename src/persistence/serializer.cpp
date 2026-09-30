#include "cad/persistence/serializer.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/ellipse.hpp"
#include "cad/core/geometry/entities/dimension.hpp"
#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace cad {

// ============================================================================
// SERIALIZACIÓN: Entidades → JSON
// ============================================================================
std::string Serializer::serialize(const std::vector<std::unique_ptr<Entity>>& entities) {
    json j;
    j["version"] = "1.0";
    j["entities"] = json::array();

    for (const auto& e : entities) {
        if (!e) continue;

        json ent;
        ent["layer"] = e->layerName;

        if (auto* line = dynamic_cast<const Line*>(e.get())) {
            ent["type"] = "LINE";
            ent["p1_x"] = line->p1.x;
            ent["p1_y"] = line->p1.y;
            ent["p2_x"] = line->p2.x;
            ent["p2_y"] = line->p2.y;
        }
        else if (auto* circle = dynamic_cast<const Circle*>(e.get())) {
            ent["type"] = "CIRCLE";
            ent["center_x"] = circle->center.x;
            ent["center_y"] = circle->center.y;
            ent["radius"] = circle->radius;
        }
        else if (auto* arc = dynamic_cast<const Arc*>(e.get())) {
            ent["type"] = "ARC";
            ent["center_x"] = arc->center.x;
            ent["center_y"] = arc->center.y;
            ent["radius"] = arc->radius;
            ent["start_angle"] = arc->startAngle;
            ent["end_angle"] = arc->endAngle;
        }
        else if (auto* poly = dynamic_cast<const Polyline*>(e.get())) {
            ent["type"] = "POLYLINE";
            json points = json::array();
            for (const auto& p : poly->points) {
                points.push_back({{"x", p.x}, {"y", p.y}});
            }
            ent["points"] = points;
            ent["closed"] = poly->closed;
        }
        else if (auto* polygon = dynamic_cast<const Polygon*>(e.get())) {
            ent["type"] = "POLYGON";
            ent["center_x"] = polygon->center.x;
            ent["center_y"] = polygon->center.y;
            ent["radius"] = polygon->radius;
            ent["sides"] = polygon->sides;
        }
        else if (auto* ellipse = dynamic_cast<const Ellipse*>(e.get())) {
            ent["type"] = "ELLIPSE";
            ent["center_x"] = ellipse->center.x;
            ent["center_y"] = ellipse->center.y;
            ent["major_radius"] = ellipse->majorRadius;
            ent["minor_radius"] = ellipse->minorRadius;
            ent["rotation"] = ellipse->rotationAngle;
        }
        else if (auto* dim = dynamic_cast<const Dimension*>(e.get())) {
            ent["type"] = "DIMENSION";
            ent["p1_x"] = dim->p1.x;
            ent["p1_y"] = dim->p1.y;
            ent["p2_x"] = dim->p2.x;
            ent["p2_y"] = dim->p2.y;
            ent["location_x"] = dim->location.x;
            ent["location_y"] = dim->location.y;
            ent["value"] = dim->value;
            ent["dim_type"] = static_cast<int>(dim->type);
            ent["is_horizontal"] = dim->isHorizontal;
            ent["is_aligned"] = dim->isAligned;
        }
        else {
            ent["type"] = "UNKNOWN";
        }

        j["entities"].push_back(ent);
    }

    // dump(4) genera JSON con indentación de 4 espacios
    return j.dump(4);
}

// ============================================================================
// DESERIALIZACIÓN: JSON → Entidades
// ============================================================================
std::vector<std::unique_ptr<Entity>> Serializer::deserialize(const std::string& jsonStr) {
    std::vector<std::unique_ptr<Entity>> entities;

    try {
        json j = json::parse(jsonStr);

        if (!j.contains("entities") || !j["entities"].is_array()) {
            std::cerr << "[Serializer] JSON sin array 'entities'." << std::endl;
            return entities;
        }

        for (const auto& ent : j["entities"]) {
            std::string type = ent.value("type", "UNKNOWN");
            std::string layer = ent.value("layer", "0");

            if (type == "LINE") {
                auto line = std::make_unique<Line>();
                line->layerName = layer;
                line->p1.x = ent.value("p1_x", 0.0);
                line->p1.y = ent.value("p1_y", 0.0);
                line->p2.x = ent.value("p2_x", 0.0);
                line->p2.y = ent.value("p2_y", 0.0);
                entities.push_back(std::move(line));
            }
            else if (type == "CIRCLE") {
                auto circle = std::make_unique<Circle>();
                circle->layerName = layer;
                circle->center.x = ent.value("center_x", 0.0);
                circle->center.y = ent.value("center_y", 0.0);
                circle->radius = ent.value("radius", 1.0);
                entities.push_back(std::move(circle));
            }
            else if (type == "ARC") {
                auto arc = std::make_unique<Arc>();
                arc->layerName = layer;
                arc->center.x = ent.value("center_x", 0.0);
                arc->center.y = ent.value("center_y", 0.0);
                arc->radius = ent.value("radius", 1.0);
                arc->startAngle = ent.value("start_angle", 0.0);
                arc->endAngle = ent.value("end_angle", 90.0);
                entities.push_back(std::move(arc));
            }
            else if (type == "POLYLINE") {
                auto poly = std::make_unique<Polyline>();
                poly->layerName = layer;
                poly->closed = ent.value("closed", false);
                if (ent.contains("points") && ent["points"].is_array()) {
                    for (const auto& p : ent["points"]) {
                        Point2D pt;
                        pt.x = p.value("x", 0.0);
                        pt.y = p.value("y", 0.0);
                        poly->points.push_back(pt);
                    }
                }
                entities.push_back(std::move(poly));
            }
            else if (type == "POLYGON") {
                auto polygon = std::make_unique<Polygon>();
                polygon->layerName = layer;
                polygon->center.x = ent.value("center_x", 0.0);
                polygon->center.y = ent.value("center_y", 0.0);
                polygon->radius = ent.value("radius", 1.0);
                polygon->sides = ent.value("sides", 3);
                entities.push_back(std::move(polygon));
            }
            else if (type == "ELLIPSE") {
                auto ellipse = std::make_unique<Ellipse>();
                ellipse->layerName = layer;
                ellipse->center.x = ent.value("center_x", 0.0);
                ellipse->center.y = ent.value("center_y", 0.0);
                ellipse->majorRadius = ent.value("major_radius", 1.0);
                ellipse->minorRadius = ent.value("minor_radius", 0.5);
                ellipse->rotationAngle = ent.value("rotation", 0.0);
                entities.push_back(std::move(ellipse));
            }
            else if (type == "DIMENSION") {
                auto dim = std::make_unique<Dimension>();
                dim->layerName = layer;
                dim->p1.x = ent.value("p1_x", 0.0);
                dim->p1.y = ent.value("p1_y", 0.0);
                dim->p2.x = ent.value("p2_x", 0.0);
                dim->p2.y = ent.value("p2_y", 0.0);
                dim->location.x = ent.value("location_x", 0.0);
                dim->location.y = ent.value("location_y", 0.0);
                dim->value = ent.value("value", 0.0);
                dim->type = static_cast<DimType>(ent.value("dim_type", 0));
                dim->isHorizontal = ent.value("is_horizontal", true);
                dim->isAligned = ent.value("is_aligned", false);
                entities.push_back(std::move(dim));
            }
            else {
                std::cerr << "[Serializer] Tipo desconocido ignorado: " << type << std::endl;
            }
        }
    }
    catch (const json::exception& e) {
        std::cerr << "[Serializer] Error al parsear JSON: " << e.what() << std::endl;
    }

    return entities;
}

} // namespace cad