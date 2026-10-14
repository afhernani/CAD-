#include "cad/persistence/serializer.hpp"
#include "cad/core/geometry/entity.hpp" // Para Entity::fromJson
#include <json.hpp>
#include <iostream>

using json = nlohmann::json;

namespace cad {

    // ============================================================================
    // SERIALIZACIÓN: Documento completo → JSON
    // ============================================================================
    std::string Serializer::serialize(const Document& doc) {
        json j;
        j["version"] = "1.0";
        j["currentLayer"] = doc.currentLayerName;

        // 1. Guardar todas las capas
        json layersArray = json::array();
        for (const auto& pair : doc.layers) {
            json layerJson;
            layerJson["name"] = pair.first;
            layerJson["visible"] = pair.second.visible;
            layerJson["frozen"] = pair.second.frozen;
            layerJson["locked"] = pair.second.locked;
            layerJson["isCurrent"] = pair.second.isCurrent;
            layerJson["color"] = {
                {"r", pair.second.color.r},
                {"g", pair.second.color.g},
                {"b", pair.second.color.b},
                {"a", pair.second.color.a}
            };
            layersArray.push_back(layerJson);
        }
        j["layers"] = layersArray;

        // 2. Guardar definiciones de bloque
        json defsArray = json::array();
        for (const auto& def : doc.blockDefinitions) {
            json jd;
            jd["name"] = def->name;
            jd["basePoint"] = {{"x", def->basePoint.x}, {"y", def->basePoint.y}};
            
            json entsArray = json::array();
            for (const auto& e : def->entities) {
                entsArray.push_back(e->toJson());
            }
            jd["entities"] = entsArray;
            defsArray.push_back(jd);
        }
        j["blockDefinitions"] = defsArray;

        // 3. Guardar entidades (usando polimorfismo)
        j["entities"] = json::array();
        for (const auto& e : doc.entities) {
            if (e) {
                j["entities"].push_back(e->toJson());
            }
        }

        return j.dump(4);
    }

    // ============================================================================
    // DESERIALIZACIÓN: JSON → Documento completo
    // ============================================================================
    void Serializer::deserialize(const std::string& jsonStr, Document& doc) {
        try {
            json j = json::parse(jsonStr);

            // Limpiar estado actual
            doc.currentLayerName = j.value("currentLayer", "0");
            doc.entities.clear();
            doc.layers.clear(); 

            // 1. Cargar capas
            if (j.contains("layers") && j["layers"].is_array()) {
                for (const auto& layerJson : j["layers"]) {
                    std::string name = layerJson.value("name", "0");
                    Layer layer(name);
                    layer.visible = layerJson.value("visible", true);
                    layer.frozen = layerJson.value("frozen", false);
                    layer.locked = layerJson.value("locked", false);
                    layer.isCurrent = layerJson.value("isCurrent", false);
                    
                    if (layerJson.contains("color")) {
                        layer.color.r = layerJson["color"].value("r", 255);
                        layer.color.g = layerJson["color"].value("g", 255);
                        layer.color.b = layerJson["color"].value("b", 255);
                        layer.color.a = layerJson["color"].value("a", 255);
                    }
                    doc.layers[name] = layer;
                }
            } else {
                // Fallback para archivos antiguos que no guardaron capas
                Layer defaultLayer("0");
                defaultLayer.isCurrent = true;
                doc.layers["0"] = defaultLayer;
            }

            // 2. Cargar definiciones de bloque
            if (j.contains("blockDefinitions") && j["blockDefinitions"].is_array()) {
                for (auto& jd : j["blockDefinitions"]) {
                    auto def = std::make_unique<BlockDefinition>();
                    def->name = jd.value("name", "");
                    if (jd.contains("basePoint")) {
                        def->basePoint = {jd["basePoint"]["x"].get<double>(), 
                                        jd["basePoint"]["y"].get<double>()};
                    }
                    if (jd.contains("entities")) {
                        for (auto& je : jd["entities"]) {
                            auto e = Entity::fromJson(je);
                            if (e) def->entities.push_back(std::move(e));
                        }
                    }
                    doc.blockDefinitions.push_back(std::move(def));
                }
            }

            // 3. Cargar entidades
            if (j.contains("entities") && j["entities"].is_array()) {
                for (const auto& ent : j["entities"]) {
                    auto e = Entity::fromJson(ent);
                    if (e) {
                        // Resolver puntero de BlockInsert a su definición
                        if (auto* bi = dynamic_cast<BlockInsert*>(e.get())) {
                            std::string name = ent.value("blockName", "");
                            bi->definition = doc.findBlockDefinition(name);
                        }
                        doc.entities.push_back(std::move(e));
                    }
                }
            }
        }
        catch (const json::exception& e) {
            std::cerr << "[Serializer] Error al parsear JSON: " << e.what() << std::endl;
        }
    }

} // namespace cad