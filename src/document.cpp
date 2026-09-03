#include "document.hpp"
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

    void Document::saveToFile(const std::string& filename) {
        nlohmann::json j;
        j["version"] = "1.0";
        j["currentLayer"] = currentLayerName;
        
        nlohmann::json entitiesArray = nlohmann::json::array();
        for (const auto& e : entities) {
            entitiesArray.push_back(e->toJson());
        }
        j["entities"] = entitiesArray;

        std::ofstream o(filename);
        if (o.is_open()) {
            o << j.dump(4); // 4 espacios de indentación para que sea legible
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
        
        if (j.contains("entities")) {
            for (auto& je : j["entities"]) {
                auto e = Entity::fromJson(je);
                if (e) entities.push_back(std::move(e));
            }
        }
    }

} // namespace cad