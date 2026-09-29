#include "cad/commands/modify/mirror_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>

namespace cad {

MirrorCommand::MirrorCommand() {
    statusMessage_ = "SIMETRIA | Primer punto del eje:";
}

void MirrorCommand::execute(const std::string& input, Engine& engine) {
    // Intentar parsear como coordenada "x,y"
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
    } else {
        statusMessage_ = "SIMETRIA | Formato inválido. Usa coordenadas x,y";
    }
}

void MirrorCommand::onPoint(const Point2D& point, Engine& engine) {
    if (step_ == Step::FirstAxisPoint) {
        axisP1_ = point;
        hasAxisP1_ = true;
        step_ = Step::SecondAxisPoint;
        statusMessage_ = "SIMETRIA | Segundo punto del eje:";
    } 
    else if (step_ == Step::SecondAxisPoint) {
        Point2D axisP2 = point;
        
        // Guardar estado para Undo
        engine.saveState();
        
        // Para cada entidad seleccionada, crear copia reflejada
        for (Entity* entity : engine.selectedEntities) {
            auto copy = entity->clone();
            copy->mirror(axisP1_, axisP2);
            engine.doc.addEntity(std::move(copy));
        }
        
        statusMessage_ = "Simetría aplicada (" + 
                        std::to_string(engine.selectedEntities.size()) + " entidades).";
        finished_ = true;
        
        // Limpiar selección
        engine.selectedEntities.clear();
    }
}

void MirrorCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "Comando cancelado.";
}

std::string MirrorCommand::getStatusMessage() const {
    return statusMessage_;
}

bool MirrorCommand::isComplete() const {
    return finished_;
}

} // namespace cad