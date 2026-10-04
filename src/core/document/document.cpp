#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include <SFML/Graphics/Color.hpp>

namespace cad {

    Document::Document() {
        Layer defaultLayer("0");
        defaultLayer.isCurrent = true;
        defaultLayer.color = sf::Color::White;
        layers["0"] = defaultLayer;
    }

    void Document::clear() {
        entities.clear();
    }

    void Document::addEntity(std::unique_ptr<Entity> entity) {
        entities.push_back(std::move(entity));
    }

    void Document::addLayer(const std::string& name) {
        if (layers.find(name) == layers.end() && !name.empty()) {
            Layer newLayer(name);
            newLayer.color = sf::Color::Cyan;
            layers[name] = newLayer;
        }
    }

    void Document::setCurrentLayer(const std::string& name) {
        if (layers.find(name) != layers.end()) {
            for (auto& pair : layers) {
                pair.second.isCurrent = false;
            }
            layers[name].isCurrent = true;
            currentLayerName = name;
        }
    }

    void Document::setLayerVisibility(const std::string& name, bool visible) {
        if (layers.find(name) != layers.end()) {
            layers[name].visible = visible;
        }
    }

    void Document::setLayerFrozen(const std::string& name, bool frozen) {
        if (layers.find(name) != layers.end()) {
            layers[name].frozen = frozen;
        }
    }

    void Document::setLayerLocked(const std::string& name, bool locked) {
        if (layers.find(name) != layers.end()) {
            layers[name].locked = locked;
        }
    }

    const Layer* Document::getCurrentLayer() const {
        auto it = layers.find(currentLayerName);
        return (it != layers.end()) ? &(it->second) : nullptr;
    }

    const Layer* Document::getLayer(const std::string& name) const {
        auto it = layers.find(name);
        return (it != layers.end()) ? &(it->second) : nullptr;
    }

    void Document::setLayerColor(const std::string& name, const sf::Color& color) {
        if (layers.find(name) != layers.end()) {
            layers[name].color = color;
        }
    }

    void Document::saveToFile(const std::string& filename) {
        nlohmann::json j;
        j["version"] = "1.0";
        j["currentLayer"] = currentLayerName;
        
        // Guardar definiciones de bloque
        nlohmann::json defsArray = nlohmann::json::array();
        for (const auto& def : blockDefinitions) {
            nlohmann::json jd;
            jd["name"] = def->name;
            jd["basePoint"] = {{"x", def->basePoint.x}, {"y", def->basePoint.y}};
            nlohmann::json entsArray = nlohmann::json::array();
            for (const auto& e : def->entities) {
                entsArray.push_back(e->toJson());
            }
            jd["entities"] = entsArray;
            defsArray.push_back(jd);
        }
        j["blockDefinitions"] = defsArray;

        nlohmann::json entitiesArray = nlohmann::json::array();
        for (const auto& e : entities) {
            entitiesArray.push_back(e->toJson());
        }
        j["entities"] = entitiesArray;

        std::ofstream o(filename);
        if (o.is_open()) {
            o << j.dump(4);
            o.close();
        }
    }

    void Document::loadFromFile(const std::string& filename) {
        std::ifstream i(filename);
        if (!i.is_open()) return; // Archivo no encontrado
        
        nlohmann::json j;
        i >> j;
        i.close();

        currentLayerName = j.value("currentLayer", "0");
        entities.clear();
        blockDefinitions.clear();

        // 1. Cargar definiciones de bloque primero
        if (j.contains("blockDefinitions")) {
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
                blockDefinitions.push_back(std::move(def));
            }
        }
        // 2. Cargar entidades
        if (j.contains("entities")) {
            for (auto& je : j["entities"]) {
                auto e = Entity::fromJson(je);
                if (e) {
                    // Resolver puntero de BlockInsert
                    if (auto* bi = dynamic_cast<BlockInsert*>(e.get())) {
                        std::string name = je.value("blockName", "");
                        bi->definition = findBlockDefinition(name);
                    }
                    entities.push_back(std::move(e));
                }
            }
        }
    }
    // Implementación de métodos para BlockDefinition
    BlockDefinition* Document::addBlockDefinition(std::unique_ptr<BlockDefinition> def) {
        BlockDefinition* ptr = def.get();
        blockDefinitions.push_back(std::move(def));
        return ptr;
    }

    BlockDefinition* Document::findBlockDefinition(const std::string& name) {
        for (auto& def : blockDefinitions) {
            if (def->name == name) return def.get();
        }
        return nullptr;
    }
} // namespace cad