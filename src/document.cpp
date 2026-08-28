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

} // namespace cad