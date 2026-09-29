#include "cad/commands/modify/stretch_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>
#include <cmath>
#include <algorithm>

namespace cad {

    StretchCommand::StretchCommand() {
        statusMessage_ = "STRETCH | Selecciona entidades con ventana de cruce (primer punto):";
    }

    void StretchCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        statusMessage_ = "STRETCH | Formato no válido. Usa coordenadas x,y.";
    }

    void StretchCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingBasePoint) {
            basePoint_ = point;
            hasBasePoint_ = true;
            step_ = Step::WaitingDestPoint;
            statusMessage_ = "STRETCH | Punto destino:";
        }
        else if (step_ == Step::WaitingDestPoint) {
            destPoint_ = point;
            executeStretch(engine);
            finished_ = true;
        }
    }

    void StretchCommand::executeStretch(Engine& engine) {
        if (selectedEntities_.empty() || !hasWindow_ || !hasBasePoint_) return;
        
        engine.saveState();
        
        double dx = destPoint_.x - basePoint_.x;
        double dy = destPoint_.y - basePoint_.y;
        
        // Calcular límites de la ventana
        double minX = std::min(windowP1_.x, windowP2_.x);
        double maxX = std::max(windowP1_.x, windowP2_.x);
        double minY = std::min(windowP1_.y, windowP2_.y);
        double maxY = std::max(windowP1_.y, windowP2_.y);
        
        int movedVertices = 0;
        
        // Mover solo los vértices que están dentro de la ventana de cruce
        for (Entity* e : selectedEntities_) {
            auto grips = e->getGripPoints();
            for (size_t i = 0; i < grips.size(); ++i) {
                if (grips[i].x >= minX && grips[i].x <= maxX &&
                    grips[i].y >= minY && grips[i].y <= maxY) {
                    e->moveGrip(i, {grips[i].x + dx, grips[i].y + dy});
                    movedVertices++;
                }
            }
        }
        
        statusMessage_ = "STRETCH | " + std::to_string(movedVertices) + " vértices estirados.";
    }

    void StretchCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "STRETCH | Comando cancelado.";
    }

    std::string StretchCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool StretchCommand::isComplete() const {
        return finished_;
    }

} // namespace cad