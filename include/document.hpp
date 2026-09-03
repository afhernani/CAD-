#pragma once
#include "geometry.hpp"
#include "layer.hpp"
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <fstream>
#include "json.hpp"

namespace cad {

    class Document {
    public:
        std::vector<std::unique_ptr<Entity>> entities;
        std::map<std::string, Layer> layers;
        std::string currentLayerName = "0";

        Document();
        ~Document() = default;

        void clear();

        // Gestión de entidades
        void addEntity(std::unique_ptr<Entity> entity);

        // Gestión de capas
        void addLayer(const std::string& name);
        void setCurrentLayer(const std::string& name);
        void setLayerVisibility(const std::string& name, bool visible);
        void setLayerFrozen(const std::string& name, bool frozen);
        void setLayerLocked(const std::string& name, bool locked);
        
        const Layer* getCurrentLayer() const;
        const Layer* getLayer(const std::string& name) const;

        // ... (lo que ya tienes) ...
        void saveToFile(const std::string& filename);
        void loadFromFile(const std::string& filename);
        
    };

} // namespace cad