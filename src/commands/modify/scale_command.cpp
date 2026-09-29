#include "cad/commands/modify/scale_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>
#include <cmath>

namespace cad {

ScaleCommand::ScaleCommand() {
    statusMessage_ = "ESCALAR | Punto base:";
}

void ScaleCommand::execute(const std::string& input, Engine& engine) {
    // 1. Intentar parsear como coordenada "x,y"
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
        return;
    }

    // 2. Intentar parsear como factor de escala (número)
    try {
        double factor = std::stod(input);
        
        if (step_ == Step::Factor && factor > 0) {
            // Guardar estado para Undo
            engine.saveState();
            
            // Aplicar escala a las entidades seleccionadas
            for (Entity* entity : engine.selectedEntities) {
                entity->scale(basePoint_, factor);
            }
            
            statusMessage_ = "Entidades escaladas (factor: " + std::to_string(factor) + ").";
            finished_ = true;
            
            // Limpiar selección
            engine.selectedEntities.clear();
        } else {
            statusMessage_ = "ESCALAR | Factor inválido. Debe ser mayor que 0.";
        }
    } catch (...) {
        statusMessage_ = "ESCALAR | Formato inválido. Usa coordenadas x,y o un número.";
    }
}

void ScaleCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::BasePoint) {
        basePoint_ = point;
        hasBasePoint_ = true;
        step_ = Step::Factor;
        statusMessage_ = "ESCALAR | Factor de escala (número) o punto:";
    } 
    else if (step_ == Step::Factor) {
        // Calcular factor como distancia desde el punto base hasta el punto actual
        double dx = point.x - basePoint_.x;
        double dy = point.y - basePoint_.y;
        double factor = std::sqrt(dx * dx + dy * dy);
        
        if (factor < 0.001) {
            statusMessage_ = "ESCALAR | Factor demasiado pequeño. Intenta de nuevo:";
            return;
        }
        
        // Guardar estado para Undo
        engine.saveState();
        
        // Aplicar escala
        for (Entity* entity : engine.selectedEntities) {
            entity->scale(basePoint_, factor);
        }
        
        statusMessage_ = "Entidades escaladas (factor: " + std::to_string(factor) + ").";
        finished_ = true;
        
        // Limpiar selección
        engine.selectedEntities.clear();
    }
}

void ScaleCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string ScaleCommand::getStatusMessage() const {
    return statusMessage_;
}

bool ScaleCommand::isComplete() const {
    return finished_;
}

} // namespace cad