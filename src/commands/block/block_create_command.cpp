#include "cad/commands/block/block_create_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include <sstream>
#include <algorithm>

namespace cad {

BlockCreateCommand::BlockCreateCommand() {
    statusMessage_ = "BLOQUE | Nombre del bloque:";
}

void BlockCreateCommand::execute(const std::string& input, Engine& engine) {
    if (step_ == Step::WaitingName) {
        if (input.empty()) {
            statusMessage_ = "BLOQUE | Nombre vacío. Comando cancelado.";
            finished_ = true;
            return;
        }
        blockName_ = input;
        // Verificar si ya existe
        if (engine.doc.findBlockDefinition(blockName_)) {
            statusMessage_ = "BLOQUE | Ya existe un bloque con ese nombre. Nuevo nombre:";
            return; // Se queda en WaitingName
        }
        step_ = Step::WaitingBasePoint;
        statusMessage_ = "BLOQUE | Punto base del bloque (clic o coordenada):";
    }
    else if (step_ == Step::SelectingEntities) {
        // Enter vacío termina la selección
        if (input.empty()) {
            if (selectedEntities_.empty()) {
                statusMessage_ = "BLOQUE | No hay entidades. Comando cancelado.";
                finished_ = true;
            } else {
                executeCreate(engine);
                finished_ = true;
            }
        }
    }
}

void BlockCreateCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::WaitingBasePoint) {
        basePoint_ = point;
        step_ = Step::SelectingEntities;
        statusMessage_ = "BLOQUE | Selecciona entidades (clic) y pulsa Enter para terminar:";
    }
}

void BlockCreateCommand::executeCreate(Engine& engine) {
    engine.saveState();
    
    // 1. Crear la definición del bloque
    auto newDef = std::make_unique<BlockDefinition>();
    newDef->name = blockName_;
    newDef->basePoint = basePoint_;
    
    // Clonar y trasladar las entidades seleccionadas al origen del bloque
    for (Entity* e : selectedEntities_) {
        auto copy = e->clone();
        copy->move(-basePoint_.x, -basePoint_.y);
        newDef->entities.push_back(std::move(copy));
    }
    
    // Añadir la definición al documento
    BlockDefinition* defPtr = engine.doc.addBlockDefinition(std::move(newDef));
    
    // 2. Crear una instancia (BlockInsert) en la posición del punto base
    auto newInsert = std::make_unique<BlockInsert>();
    newInsert->definition = defPtr;
    newInsert->insertPoint = basePoint_;
    newInsert->layerName = engine.doc.currentLayerName;
    engine.doc.addEntity(std::move(newInsert));
    
    statusMessage_ = "BLOQUE | '" + blockName_ + "' creado con " +
                     std::to_string(selectedEntities_.size()) + " entidades.";
}

void BlockCreateCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "BLOQUE | Comando cancelado.";
}

std::string BlockCreateCommand::getStatusMessage() const { return statusMessage_; }
bool BlockCreateCommand::isComplete() const { return finished_; }

} // namespace cad