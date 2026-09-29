#include "cad/commands/block/block_insert_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"

namespace cad {

BlockInsertCommand::BlockInsertCommand() {
    statusMessage_ = "INSERTAR | Nombre del bloque a insertar:";
}

void BlockInsertCommand::execute(const std::string& input, Engine& engine) {
    if (step_ == Step::WaitingName) {
        if (input.empty()) {
            statusMessage_ = "INSERTAR | Comando cancelado.";
            finished_ = true;
            return;
        }
        definition_ = engine.doc.findBlockDefinition(input);
        if (!definition_) {
            statusMessage_ = "INSERTAR | Bloque no encontrado. Nombre:";
            return; // Se queda en WaitingName
        }
        step_ = Step::WaitingInsertPoint;
        statusMessage_ = "INSERTAR | Punto de inserción:";
    }
}

void BlockInsertCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::WaitingInsertPoint && definition_) {
        engine.saveState();
        
        auto newInsert = std::make_unique<BlockInsert>();
        newInsert->definition = definition_;
        newInsert->insertPoint = point;
        newInsert->layerName = engine.doc.currentLayerName;
        engine.doc.addEntity(std::move(newInsert));
        
        statusMessage_ = "INSERTAR | Bloque insertado. Nombre o Enter para terminar:";
        finished_ = true; 
    }
}

void BlockInsertCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "INSERTAR | Comando cancelado.";
}

std::string BlockInsertCommand::getStatusMessage() const { return statusMessage_; }
bool BlockInsertCommand::isComplete() const { return finished_; }

} // namespace cad