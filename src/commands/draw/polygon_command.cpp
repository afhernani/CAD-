// src/commands/draw/polygon_command.cpp
#include "cad/commands/draw/polygon_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include <sstream>
#include <cmath>

namespace cad {

PolygonCommand::PolygonCommand() {
    statusMessage_ = "POLIGONO | Especificar centro:";
}

void PolygonCommand::execute(const std::string& input, Engine& engine) {
    // Intentar parsear como coordenada "x,y"
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
        return;
    }

    // Intentar parsear como número (lados o radio)
    try {
        double value = std::stod(input);
        
        if (step_ == Step::WaitingSides) {
            int numSides = static_cast<int>(value);
            if (numSides < 3) {
                statusMessage_ = "POLIGONO | Mínimo 3 lados. Intenta de nuevo:";
                return;
            }
            sides_ = numSides;
            hasSides_ = true;
            step_ = Step::WaitingRadius;
            statusMessage_ = "POLIGONO | Radio (número) o punto:";
        }
        else if (step_ == Step::WaitingRadius && value > 0) {
            // Crear el polígono
            auto polygon = std::make_unique<Polygon>();
            polygon->center = center_;
            polygon->sides = sides_;
            polygon->radius = value;
            polygon->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(polygon));
            
            statusMessage_ = "Polígono creado (" + std::to_string(sides_) + " lados).";
            finished_ = true;
        }
        else {
            statusMessage_ = "POLIGONO | Valor no válido.";
        }
    } catch (...) {
        statusMessage_ = "POLIGONO | Formato no válido. Usa coordenadas x,y o un número.";
    }
}

void PolygonCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::WaitingCenter) {
        center_ = point;
        hasCenter_ = true;
        step_ = Step::WaitingSides;
        statusMessage_ = "POLIGONO | Número de lados (ej: 6):";
    }
    else if (step_ == Step::WaitingRadius) {
        // Calcular radio desde centro hasta este punto
        double dx = point.x - center_.x;
        double dy = point.y - center_.y;
        double radius = std::sqrt(dx * dx + dy * dy);
        
        if (radius < 0.001) {
            statusMessage_ = "POLIGONO | Radio demasiado pequeño. Intenta de nuevo:";
            return;
        }
        
        auto polygon = std::make_unique<Polygon>();
        polygon->center = center_;
        polygon->sides = sides_;
        polygon->radius = radius;
        polygon->layerName = engine.doc.currentLayerName;
        engine.saveState();
        engine.doc.addEntity(std::move(polygon));
        
        statusMessage_ = "Polígono creado (" + std::to_string(sides_) + " lados).";
        finished_ = true;
    }
}

void PolygonCommand::onCancel() {
    step_ = Step::WaitingCenter;
    hasCenter_ = false;
    hasSides_ = false;
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string PolygonCommand::getStatusMessage() const {
    return statusMessage_;
}

bool PolygonCommand::isComplete() const {
    return finished_;
}

} // namespace cad