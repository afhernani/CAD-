#include "cad/commands/modify/copy_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>

namespace cad {

CopyCommand::CopyCommand() {
    statusMessage_ = "COPIAR | Especificar punto base:";
}

void CopyCommand::execute(const std::string& input, Engine& engine) {
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
    } else {
        statusMessage_ = "COPIAR | Formato inválido. Usa coordenadas x,y";
    }
}

void CopyCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::BasePoint) {
        basePoint_ = point;
        hasBasePoint_ = true;
        step_ = Step::Destination;
        statusMessage_ = "COPIAR | Especificar punto destino:";
    } 
    else if (step_ == Step::Destination) {
        // Calcular delta
        double dx = point.x - basePoint_.x;
        double dy = point.y - basePoint_.y;
        
        // Guardar estado para Undo
        engine.saveState();
        
        // Clonar y mover cada entidad seleccionada
        for (Entity* entity : engine.selectedEntities) {
            auto copy = entity->clone();
            copy->move(dx, dy);
            engine.doc.addEntity(std::move(copy));
        }
        
        statusMessage_ = "Entidades copiadas (" + std::to_string(engine.selectedEntities.size()) + ").";
        finished_ = true;
        
        // Limpiar selección al terminar
        engine.selectedEntities.clear();
    }
}

void CopyCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string CopyCommand::getStatusMessage() const {
    return statusMessage_;
}

bool CopyCommand::isComplete() const {
    return finished_;
}

} // namespace cad