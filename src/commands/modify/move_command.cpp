#include "cad/commands/modify/move_command.hpp"
#include "cad/commands/engine.hpp" // Necesario para acceder a Engine
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>

namespace cad {

    MoveCommand::MoveCommand() {
        statusMessage_ = "MOVER | Especificar punto base:";
    }

    void MoveCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
        } else {
            statusMessage_ = "MOVER | Formato inválido. Usa coordenadas x,y";
        }
    }

    void MoveCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::BasePoint) {
            basePoint_ = point;
            hasBasePoint_ = true;
            step_ = Step::Destination;
            statusMessage_ = "MOVER | Especificar punto destino:";
        } 
        else if (step_ == Step::Destination) {
            // Calcular delta
            double dx = point.x - basePoint_.x;
            double dy = point.y - basePoint_.y;
            
            // Guardar estado para Undo
            engine.saveState();
            
            // Mover entidades seleccionadas
            for (Entity* entity : engine.selectedEntities) {
                entity->move(dx, dy);
            }
            
            statusMessage_ = "Entidades movidas (" + std::to_string(engine.selectedEntities.size()) + ").";
            finished_ = true;
            
            // Limpiar selección al terminar (opcional, estilo AutoCAD)
            engine.selectedEntities.clear();
        }
    }

    void MoveCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string MoveCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool MoveCommand::isComplete() const {
        return finished_;
    }

    void MoveCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                   const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasBasePoint_) return;

        sf::Color originalColor(255, 165, 0, 180); // Naranja translúcido
        sf::Color ghostColor(0, 200, 255, 120);     // Azul fantasma
        sf::Color axisColor(255, 255, 0, 200);      // Amarillo para guías

        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. Dibujar entidades originales en su posición actual
        for (Entity* e : engine.selectedEntities) {
            e->draw(window, w2s, originalColor, view.getScale());
        }

        // 2. Dibujar línea guía y punto base
        sf::Vertex guideLine[] = {
            sf::Vertex(w2s(basePoint_.x, basePoint_.y), axisColor),
            sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor)
        };
        window.draw(guideLine, 2, sf::Lines);

        sf::CircleShape baseMark(4.0f);
        baseMark.setFillColor(axisColor);
        baseMark.setOrigin(4.0f, 4.0f);
        baseMark.setPosition(w2s(basePoint_.x, basePoint_.y));
        window.draw(baseMark);

        // 3. Dibujar "fantasmas" (preview del movimiento)
        double dx = mouseWorldPos.x - basePoint_.x;
        double dy = mouseWorldPos.y - basePoint_.y;

        for (Entity* e : engine.selectedEntities) {
            auto ghost = e->clone();
            ghost->move(dx, dy); // Asumiendo que tu clase Entity tiene el método move()
            ghost->draw(window, w2s, ghostColor, view.getScale());
        }
    }

} // namespace cad