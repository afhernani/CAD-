// src/commands/draw/arc_command.cpp
#include "cad/commands/draw/arc_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    ArcCommand::ArcCommand() {
        statusMessage_ = "ARCO | Especificar centro:";
    }

    void ArcCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // Si no es coordenada, no lo aceptamos (solo puntos)
        statusMessage_ = "ARCO | Formato no válido. Especifica un punto (x,y):";
    }

    void ArcCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingCenter) {
            center_ = point;
            hasCenter_ = true;
            step_ = Step::WaitingStartPoint;
            statusMessage_ = "ARCO | Especificar punto de inicio (define radio y ángulo inicial):";
        }
        else if (step_ == Step::WaitingStartPoint) {
            startPoint_ = point;
            hasStartPoint_ = true;
            
            // Calcular radio y ángulo inicial
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            radius_ = std::sqrt(dx * dx + dy * dy);
            startAngle_ = std::atan2(dy, dx) * 180.0 / PI;
            if (startAngle_ < 0) startAngle_ += 360.0;
            
            step_ = Step::WaitingEndPoint;
            statusMessage_ = "ARCO | Especificar punto final:";
        }
        else if (step_ == Step::WaitingEndPoint) {
            // Calcular ángulo final
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            double endAngle = std::atan2(dy, dx) * 180.0 / PI;
            if (endAngle < 0) endAngle += 360.0;
            
            // Crear el arco
            auto arc = std::make_unique<Arc>();
            arc->center = center_;
            arc->radius = radius_;
            arc->startAngle = startAngle_;
            arc->endAngle = endAngle;
            arc->layerName = engine.doc.currentLayerName;
            engine.saveState();
            engine.doc.addEntity(std::move(arc));
            
            statusMessage_ = "Arco creado.";
            finished_ = true;
        }
    }

    void ArcCommand::onCancel() {
        step_ = Step::WaitingCenter;
        hasCenter_ = false;
        hasStartPoint_ = false;
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string ArcCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool ArcCommand::isComplete() const {
        return finished_;
    }

} // namespace cad