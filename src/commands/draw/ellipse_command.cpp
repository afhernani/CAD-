#include "cad/commands/draw/ellipse_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include <sstream>
#include <cmath>

namespace cad {

EllipseCommand::EllipseCommand() {
    statusMessage_ = "ELIPSE | Especificar centro:";
}

void EllipseCommand::execute(const std::string& input, Engine& engine) {
    // Intentar parsear como coordenada "x,y"
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
        return;
    }

    // Intentar parsear como número (radio menor)
    try {
        double value = std::stod(input);
        
        if (step_ == Step::WaitingMinorRadius && value > 0) {
            // Crear la elipse
            auto ellipse = std::make_unique<Ellipse>();
            ellipse->center = center_;
            ellipse->majorRadius = majorRadius_;
            ellipse->minorRadius = value;
            ellipse->rotationAngle = rotationAngle_;
            ellipse->layerName = engine.doc.currentLayerName;
            engine.saveState();
            engine.doc.addEntity(std::move(ellipse));
            
            statusMessage_ = "Elipse creada.";
            finished_ = true;
        }
        else {
            statusMessage_ = "ELIPSE | Valor no válido.";
        }
    } catch (...) {
        statusMessage_ = "ELIPSE | Formato no válido. Usa coordenadas x,y o un número.";
    }
}

void EllipseCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::WaitingCenter) {
        center_ = point;
        hasCenter_ = true;
        step_ = Step::WaitingMajorAxis;
        statusMessage_ = "ELIPSE | Punto final del eje mayor:";
    }
    else if (step_ == Step::WaitingMajorAxis) {
        majorAxisEnd_ = point;
        hasMajorAxis_ = true;
        
        // Calcular radio mayor y rotación
        double dx = point.x - center_.x;
        double dy = point.y - center_.y;
        majorRadius_ = std::sqrt(dx * dx + dy * dy);
        rotationAngle_ = std::atan2(dy, dx);
        
        step_ = Step::WaitingMinorRadius;
        statusMessage_ = "ELIPSE | Radio del eje menor (número) o punto:";
    }
    else if (step_ == Step::WaitingMinorRadius) {
        // Calcular radio menor desde centro hasta este punto
        double dx = point.x - center_.x;
        double dy = point.y - center_.y;
        double minorRadius = std::sqrt(dx * dx + dy * dy);
        
        if (minorRadius < 0.001) {
            statusMessage_ = "ELIPSE | Radio demasiado pequeño. Intenta de nuevo:";
            return;
        }
        
        auto ellipse = std::make_unique<Ellipse>();
        ellipse->center = center_;
        ellipse->majorRadius = majorRadius_;
        ellipse->minorRadius = minorRadius;
        ellipse->rotationAngle = rotationAngle_;
        ellipse->layerName = engine.doc.currentLayerName;
        engine.saveState();
        engine.doc.addEntity(std::move(ellipse));
        
        statusMessage_ = "Elipse creada.";
        finished_ = true;
    }
}

void EllipseCommand::onCancel() {
    step_ = Step::WaitingCenter;
    hasCenter_ = false;
    hasMajorAxis_ = false;
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string EllipseCommand::getStatusMessage() const {
    return statusMessage_;
}

bool EllipseCommand::isComplete() const {
    return finished_;
}

} // namespace cad