#include "cad/commands/modify/mirror_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <iomanip>
#include <cmath>

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

    void MirrorCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasAxisP1_) return;

        sf::Color originalColor(255, 165, 0, 180); // Naranja translúcido (originales)
        sf::Color axisColor(0, 255, 0, 200);        // Verde para el eje de simetría
        sf::Color ghostColor(0, 200, 255, 120);     // Cian fantasma (reflejado)

        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. Dibujar entidades originales en su posición actual
        for (Entity* e : engine.selectedEntities) {
            e->draw(window, w2s, originalColor, view.getScale());
        }

        // 2. Dibujar el eje de simetría (línea guía)
        sf::Vertex axisLine[] = {
            sf::Vertex(w2s(axisP1_.x, axisP1_.y), axisColor),
            sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor)
        };
        window.draw(axisLine, 2, sf::Lines);

        // Marcador del primer punto del eje
        sf::CircleShape p1Mark(4.0f);
        p1Mark.setFillColor(axisColor);
        p1Mark.setOrigin(4.0f, 4.0f);
        p1Mark.setPosition(w2s(axisP1_.x, axisP1_.y));
        window.draw(p1Mark);

        // 3. Dibujar "fantasmas" (preview de la simetría)
        Point2D axisP2 = {mouseWorldPos.x, mouseWorldPos.y};
        for (Entity* e : engine.selectedEntities) {
            auto ghost = e->clone();
            // Asumiendo que tu clase Entity tiene el método mirror(Point2D p1, Point2D p2)
            ghost->mirror(axisP1_, axisP2); 
            ghost->draw(window, w2s, ghostColor, view.getScale());
        }
    }

} // namespace cad