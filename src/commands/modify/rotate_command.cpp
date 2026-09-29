#include "cad/commands/modify/rotate_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>
#include <cmath>

namespace cad {

namespace {
    constexpr double PI = 3.14159265358979323846;
}

RotateCommand::RotateCommand() {
    statusMessage_ = "ROTAR | Centro de rotación:";
}

void RotateCommand::execute(const std::string& input, Engine& engine) {
    // 1. Intentar parsear como coordenada "x,y"
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
        return;
    }

    // 2. Intentar parsear como ángulo (número en grados)
    try {
        double angle = std::stod(input);
        
        if (step_ == Step::Angle) {
            // Guardar estado para Undo
            engine.saveState();
            
            // Aplicar rotación a las entidades seleccionadas
            for (Entity* entity : engine.selectedEntities) {
                entity->rotate(basePoint_, angle);
            }
            
            statusMessage_ = "Entidades rotadas " + std::to_string(angle) + "°.";
            finished_ = true;
            
            // Limpiar selección
            engine.selectedEntities.clear();
        } else {
            statusMessage_ = "ROTAR | Primero especifica el centro de rotación.";
        }
    } catch (...) {
        statusMessage_ = "ROTAR | Formato inválido. Usa coordenadas x,y o un ángulo en grados.";
    }
}

void RotateCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::BasePoint) {
        basePoint_ = point;
        hasCenter_ = true;
        step_ = Step::Angle;
        statusMessage_ = "ROTAR | Ángulo de rotación (grados) o punto:";
    } 
    else if (step_ == Step::Angle) {
        // Calcular ángulo entre el punto base y el punto actual
        double dx = point.x - basePoint_.x;
        double dy = point.y - basePoint_.y;
        double angle = std::atan2(dy, dx) * 180.0 / PI;
        
        // Guardar estado para Undo
        engine.saveState();
        
        // Aplicar rotación
        for (Entity* entity : engine.selectedEntities) {
            entity->rotate(basePoint_, angle);
        }
        
        statusMessage_ = "Entidades rotadas " + std::to_string(angle) + "°.";
        finished_ = true;
        
        // Limpiar selección
        engine.selectedEntities.clear();
    }
}

void RotateCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string RotateCommand::getStatusMessage() const {
    return statusMessage_;
}

bool RotateCommand::isComplete() const {
    return finished_;
}

} // namespace cad