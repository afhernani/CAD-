#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/persistence/serializer.hpp"
#include <SFML/Graphics/Color.hpp>
#include <fstream>
#include <sstream>
#include <iostream> 

namespace cad {

    Document::Document() {
        Layer defaultLayer("0");
        defaultLayer.isCurrent = true;
        defaultLayer.color = sf::Color::White;
        layers["0"] = defaultLayer;
    }

    void Document::clear() {
        entities.clear();
        blockDefinitions.clear();
        layers.clear();
        
        Layer defaultLayer("0");
        defaultLayer.isCurrent = true;
        defaultLayer.color = sf::Color::White;
        layers["0"] = defaultLayer;
        currentLayerName = "0";

        filePath = "";
        isModified = false;
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

    // Guarda en la ruta actual. Si está vacío, falla (el Engine debe manejar el "Guardar Como")
    bool Document::save() {
        if (isUntitled()) return false;
        saveToFile(filePath);
        isModified = false;
        return true;
        // Nota: Si saveToFile lanza excepciones, podrías envolverlo en un try/catch aquí.
    }

    // Guarda en una ruta nueva, actualiza la memoria y marca como limpio
    bool Document::saveAs(const std::string& path) {
        saveToFile(path);
        filePath = path;
        isModified = false;
        return true;
    }

    // Carga los datos, actualiza la ruta y marca como limpio
    bool Document::load(const std::string& path) {
        loadFromFile(path); // Tu método existente que ya maneja entidades y bloques
        filePath = path;
        isModified = false;
        return true;
    }

    // NUEVA IMPLEMENTACIÓN: Delegar en el Serializer
    void Document::saveToFile(const std::string& filename) {
        try {
            std::string jsonStr = Serializer::serialize(*this);
            std::ofstream o(filename);
            if (o.is_open()) {
                o << jsonStr;
                o.close();
            }
        } catch (const std::exception& e) {
            std::cerr << "[Document] Error al guardar en archivo: " << e.what() << std::endl;
        }
    }

    // NUEVA IMPLEMENTACIÓN: Delegar en el Serializer
    void Document::loadFromFile(const std::string& filename) {
        try {
            std::ifstream i(filename);
            if (!i.is_open()) {
                std::cerr << "[Document] No se pudo abrir el archivo: " << filename << std::endl;
                return;
            }
            
            std::stringstream buffer;
            buffer << i.rdbuf();
            std::string jsonStr = buffer.str();
            i.close();

            Serializer::deserialize(jsonStr, *this);
            
        } catch (const std::exception& e) {
            std::cerr << "[Document] Error al cargar desde archivo: " << e.what() << std::endl;
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