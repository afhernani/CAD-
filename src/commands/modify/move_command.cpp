#include "cad/commands/modify/move_command.hpp"
#include "cad/commands/engine.hpp" // Necesario para acceder a Engine
#include <sstream>

namespace cad {

MoveCommand::MoveCommand() {
    statusMessage_ = "MOVER | Especificar punto base:";
}

void MoveCommand::execute(const std::string& input, Engine& engine) {
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
    } else {
        statusMessage_ = "MOVER | Formato inválido. Usa coordenadas x,y";
    }
}

void MoveCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::BasePoint) {
        basePoint_ = point;
        hasBasePoint_ = true;
        step_ = Step::Destination;
        statusMessage_ = "MOVER | Especificar punto destino:";
    } 
    else if (step_ == Step::Destination) {
        // Calcular delta
        double dx = point.x - basePoint_.x;
        double dy = point.y - basePoint_.y;
        
        // Guardar estado para Undo
        engine.saveState();
        
        // Mover entidades seleccionadas
        for (Entity* entity : engine.selectedEntities) {
            entity->move(dx, dy);
        }
        
        statusMessage_ = "Entidades movidas (" + std::to_string(engine.selectedEntities.size()) + ").";
        finished_ = true;
        
        // Limpiar selección al terminar (opcional, estilo AutoCAD)
        engine.selectedEntities.clear();
    }
}

void MoveCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string MoveCommand::getStatusMessage() const {
    return statusMessage_;
}

bool MoveCommand::isComplete() const {
    return finished_;
}

} // namespace cad