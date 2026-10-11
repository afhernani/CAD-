#include "cad/commands/modify/explode_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include <cmath>
#include <algorithm>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    ExplodeCommand::ExplodeCommand() {
        statusMessage_ = "DESCOMPONER | Entidades descompuestas.";
        finished_ = false;
    }

    void ExplodeCommand::execute(const std::string& input, Engine& engine) {
        (void)input; // No necesitamos input de texto para este comando
        
        if (engine.selectedEntities.empty()) {
            statusMessage_ = "DESCOMPONER | No hay entidades seleccionadas.";
            finished_ = true;
            return;
        }

        // 1. Guardar estado para UNDO
        engine.saveState();

        std::vector<std::unique_ptr<Entity>> newEntities;
        std::vector<Entity*> toDelete;

        // 2. Procesar cada entidad seleccionada
        for (Entity* e : engine.selectedEntities) {
            // Solo descomponemos tipos complejos
            if (dynamic_cast<Polyline*>(e) || 
                dynamic_cast<Polygon*>(e) || 
                dynamic_cast<BlockInsert*>(e)) {
                
                explodeEntity(e, newEntities);
                toDelete.push_back(e);
            } else {
                statusMessage_ = "DESCOMPONER | Algunas entidades no se pueden descomponer (Líneas, Círculos).";
            }
        }

        // 3. Eliminar las entidades originales del documento
        for (Entity* del : toDelete) {
            engine.doc.entities.erase(
                std::remove_if(engine.doc.entities.begin(), engine.doc.entities.end(),
                    [del](const std::unique_ptr<Entity>& e) { return e.get() == del; }),
                engine.doc.entities.end()
            );
        }

        // 4. Añadir las nuevas entidades primitivas
        for (auto& newEnt : newEntities) {
            engine.doc.addEntity(std::move(newEnt));
        }

        // 5. Limpiar selección y terminar
        engine.selectedEntities.clear();
        finished_ = true;
    }

    void ExplodeCommand::explodeEntity(Entity* entity, std::vector<std::unique_ptr<Entity>>& newEntities) {
        
        // --- CASO 1: POLILÍNEA ---
        if (auto* pl = dynamic_cast<Polyline*>(entity)) {
            for (size_t i = 0; i + 1 < pl->points.size(); ++i) {
                auto line = std::make_unique<Line>();
                line->p1 = pl->points[i];
                line->p2 = pl->points[i+1];
                line->layerName = pl->layerName;
                line->id = Entity::generateId();
                newEntities.push_back(std::move(line));
            }
            // Si es cerrada, añadir el último segmento
            if (pl->closed && pl->points.size() > 2) {
                auto line = std::make_unique<Line>();
                line->p1 = pl->points.back();
                line->p2 = pl->points.front();
                line->layerName = pl->layerName;
                line->id = Entity::generateId();
                newEntities.push_back(std::move(line));
            }
        }
        
        // --- CASO 2: POLÍGONO ---
        else if (auto* pg = dynamic_cast<Polygon*>(entity)) {
            for (size_t i = 0; i + 1 < pg->points.size(); ++i) {
                auto line = std::make_unique<Line>();
                line->p1 = pg->points[i];
                line->p2 = pg->points[i+1];
                line->layerName = pg->layerName;
                line->id = Entity::generateId();
                newEntities.push_back(std::move(line));
            }
            if (pg->points.size() > 2) {
                auto line = std::make_unique<Line>();
                line->p1 = pg->points.back();
                line->p2 = pg->points.front();
                line->layerName = pg->layerName;
                line->id = Entity::generateId();
                newEntities.push_back(std::move(line));
            }
        }

        // --- CASO 3: INSERCIÓN DE BLOQUE ---
        else if (auto* bi = dynamic_cast<BlockInsert*>(entity)) {
            if (bi->definition) {
                double rad = bi->blockRotation * PI / 180.0;
                double cosA = std::cos(rad);
                double sinA = std::sin(rad);

                for (const auto& defEnt : bi->definition->entities) {
                    // Clonar la entidad original del bloque
                    auto clone = defEnt->clone();
                    
                    // Aplicar transformaciones del BlockInsert a la copia
                    // 1. Escalar (asumiendo escala uniforme. Si tu BlockInsert tiene scale X/Y, ajústalo)
                    clone->scale(Point2D{0.0, 0.0}, bi->blockScale); 
                    
                    // 2. Rotar
                    clone->rotate(Point2D{0.0, 0.0}, bi->blockRotation);
                    
                    // 3. Mover al punto de inserción
                    clone->move(bi->insertPoint.x, bi->insertPoint.y);

                    // Generar nuevo ID y heredar capa (opcional, aquí heredamos la del bloque)
                    clone->id = Entity::generateId();
                    clone->layerName = bi->layerName; // Descomenta si quieres que herede la capa del bloque
                    
                    newEntities.push_back(std::move(clone));
                }
            }
        }
    }

    void ExplodeCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "DESCOMPONER | Cancelado.";
    }

    std::string ExplodeCommand::getStatusMessage() const { return statusMessage_; }
    bool ExplodeCommand::isComplete() const { return finished_; }

} // namespace cad