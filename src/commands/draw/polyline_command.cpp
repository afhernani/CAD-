#include "cad/commands/draw/polyline_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include <sstream>
#include <algorithm>

namespace cad {

    PolylineCommand::PolylineCommand() {
        statusMessage_ = "POLILINEA | Primer punto (Enter=terminar, C=cerrar, U=deshacer):";
    }

    void PolylineCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // Verificar opciones especiales (C, U)
        std::string upperInput = input;
        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
        
        if (upperInput == "C" || upperInput == "CLOSE" || upperInput == "CERRAR") {
            if (points_.size() >= 2) {
                // points_.push_back(points_.front()); // Cerrar polilínea
                auto polyline = std::make_unique<Polyline>();
                polyline->points = points_;
                polyline->closed = true;
                polyline->layerName = engine.doc.currentLayerName;
                
                engine.saveState();
                engine.doc.addEntity(std::move(polyline));
                statusMessage_ = "Polilínea cerrada.";
                finished_ = true;
            } else {
                statusMessage_ = "POLILINEA | Se necesitan al menos 2 puntos para cerrar.";
            }
            return;
        }
        
        if (upperInput == "U" || upperInput == "UNDO") {
            if (!points_.empty()) {
                points_.pop_back();
                statusMessage_ = "POLILINEA | Último punto eliminado. Siguiente punto (Enter=terminar, C=cerrar, U=deshacer):";
            } else {
                statusMessage_ = "POLILINEA | No hay puntos para deshacer.";
            }
            return;
        }

        // Si el input está vacío (Enter), terminar polilínea
        if (input.empty()) {
            if (points_.size() >= 2) {
                auto polyline = std::make_unique<Polyline>();
                polyline->points = points_;
                polyline->layerName = engine.doc.currentLayerName;
                engine.saveState();
                engine.doc.addEntity(std::move(polyline));
                statusMessage_ = "Polilínea terminada (" + std::to_string(points_.size()) + " puntos).";
                finished_ = true;
            } else {
                statusMessage_ = "Polilínea cancelada (puntos insuficientes).";
                finished_ = true;
            }
            return;
        }

        statusMessage_ = "POLILINEA | Formato no válido. Usa coordenadas x,y o C/U:";
    }

    void PolylineCommand::onPoint(const Point2D& point, Engine& engine) {
        points_.push_back(point);
        statusMessage_ = "POLILINEA | Siguiente punto (Enter=terminar, C=cerrar, U=deshacer):";
    }

    void PolylineCommand::onCancel() {
        points_.clear();
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string PolylineCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool PolylineCommand::isComplete() const {
        return finished_;
    }

} // namespace cad