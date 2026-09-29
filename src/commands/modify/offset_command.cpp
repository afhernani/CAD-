#include "cad/commands/modify/offset_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include <sstream>
#include <cmath>

namespace cad {

OffsetCommand::OffsetCommand() {
    statusMessage_ = "DESPLAZAR | Distancia (número) o dos puntos:";
}

void OffsetCommand::execute(const std::string& input, Engine& engine) {
    // 1. Intentar parsear como coordenada "x,y"
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
        return;
    }

    // 2. Intentar parsear como número (distancia)
    try {
        double value = std::stod(input);
        
        if (step_ == Step::WaitingDistance && value >= 0) {
            distance_ = value;
            hasDistance_ = true;
            step_ = Step::WaitingEntity;
            statusMessage_ = "DESPLAZAR | Seleccionar entidad a desplazar:";
        }
        else {
            statusMessage_ = "DESPLAZAR | Valor no válido.";
        }
    } catch (...) {
        statusMessage_ = "DESPLAZAR | Formato no válido. Usa un número o coordenadas x,y.";
    }
}

void OffsetCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::WaitingDistance) {
        // Primer punto para calcular distancia
        firstPoint_ = point;
        step_ = Step::WaitingDistance;  // Mantener estado
        statusMessage_ = "DESPLAZAR | Segundo punto para definir distancia:";
    }
    else if (statusMessage_.find("Segundo") != std::string::npos) {
        // Segundo punto: calcular distancia
        double dx = point.x - firstPoint_.x;
        double dy = point.y - firstPoint_.y;
        distance_ = std::sqrt(dx * dx + dy * dy);
        hasDistance_ = true;
        step_ = Step::WaitingEntity;
        statusMessage_ = "DESPLAZAR | Seleccionar entidad a desplazar:";
    }
    else if (step_ == Step::WaitingEntity) {
        // Buscar entidad cercana al punto
        double tolerance = 10.0 / engine.viewScale;
        Entity* found = nullptr;
        
        for (auto& entity : engine.doc.entities) {
            if (entity->isNear(point, tolerance)) {
                // Solo permitir líneas, círculos y arcos
                if (dynamic_cast<Line*>(entity.get()) ||
                    dynamic_cast<Circle*>(entity.get()) ||
                    dynamic_cast<Arc*>(entity.get())) {
                    found = entity.get();
                    break;
                }
            }
        }
        
        if (found) {
            selectedEntity_ = found;
            step_ = Step::WaitingSide;
            statusMessage_ = "DESPLAZAR | Indicar lado del desplazamiento:";
        } else {
            statusMessage_ = "DESPLAZAR | No se encontró entidad válida. Intenta de nuevo:";
        }
    }
    else if (step_ == Step::WaitingSide) {
        // Crear entidad desplazada
        createOffsetEntity(selectedEntity_, point, engine);
        
        // Resetear para permitir múltiples desplazamientos
        selectedEntity_ = nullptr;
        step_ = Step::WaitingEntity;
        statusMessage_ = "DESPLAZAR | Seleccionar otra entidad o ESC para terminar:";
    }
}

void OffsetCommand::createOffsetEntity(Entity* entity, const Point2D& sidePoint, Engine& engine) {
    engine.saveState();
    
    if (auto* line = dynamic_cast<Line*>(entity)) {
        // Calcular línea paralela
        double dx = line->p2.x - line->p1.x;
        double dy = line->p2.y - line->p1.y;
        double len = std::sqrt(dx * dx + dy * dy);
        
        if (len > 0) {
            // Vector normal perpendicular
            double nx = -dy / len;
            double ny = dx / len;
            
            // Determinar lado según el punto de clic
            double vx = sidePoint.x - line->p1.x;
            double vy = sidePoint.y - line->p1.y;
            double side = vx * nx + vy * ny;
            double sign = (side >= 0) ? 1.0 : -1.0;
            
            auto newLine = std::make_unique<Line>();
            newLine->p1 = {line->p1.x + nx * distance_ * sign,
                          line->p1.y + ny * distance_ * sign};
            newLine->p2 = {line->p2.x + nx * distance_ * sign,
                          line->p2.y + ny * distance_ * sign};
            newLine->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newLine));
        }
    }
    else if (auto* circle = dynamic_cast<Circle*>(entity)) {
        // Círculo concéntrico
        auto newCircle = std::make_unique<Circle>();
        newCircle->center = circle->center;
        newCircle->radius = circle->radius + distance_;
        if (newCircle->radius < 0) newCircle->radius = std::abs(newCircle->radius);
        newCircle->layerName = engine.doc.currentLayerName;
        engine.doc.addEntity(std::move(newCircle));
    }
    else if (auto* arc = dynamic_cast<Arc*>(entity)) {
        // Arco concéntrico
        auto newArc = std::make_unique<Arc>();
        newArc->center = arc->center;
        newArc->radius = arc->radius + distance_;
        newArc->startAngle = arc->startAngle;
        newArc->endAngle = arc->endAngle;
        if (newArc->radius < 0) newArc->radius = std::abs(newArc->radius);
        newArc->layerName = engine.doc.currentLayerName;
        engine.doc.addEntity(std::move(newArc));
    }
}

void OffsetCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string OffsetCommand::getStatusMessage() const {
    return statusMessage_;
}

bool OffsetCommand::isComplete() const {
    return finished_;
}

} // namespace cad