#include "cad/commands/draw/line_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"      // 1. NECESARIO: Para sf::Color, sf::Vertex, sf::RenderWindow
#include "cad/core/constants.hpp"         // 2. NECESARIO: Para que el compilador conozca CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    LineCommand::LineCommand() {
        statusMessage_ = "LINEA | Especificar primer punto:";
    }

    void LineCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
        } else {
            statusMessage_ = "LINEA | Formato inválido. Usa: x,y";
        }
    }

    void LineCommand::onPoint(const Point2D& point, Engine& engine) {
        if (state_ == State::WaitingFirstPoint || state_ == State::LineCreated) {
            // Primer punto (o nuevo punto después de crear una línea)
            p1_ = point;
            state_ = State::WaitingSecondPoint;
            statusMessage_ = "LINEA | Especificar siguiente punto:";
        } else {
            // Segundo punto: crear la línea
            auto line = std::make_unique<Line>();
            line->p1 = p1_;
            line->p2 = point;
            line->layerName = engine.doc.currentLayerName;
            engine.saveState();
            engine.doc.addEntity(std::move(line));

            statusMessage_ = "LINEA | Línea creada. Especificar siguiente punto (o Enter para terminar):";
            state_ = State::LineCreated;  // ← NUEVO: no actualizar p1_ todavía
        }
    }

    void LineCommand::onCancel() {
        state_ = State::WaitingFirstPoint;
        statusMessage_ = "Comando cancelado.";
    }

    std::string LineCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool LineCommand::isComplete() const {
        return false;
    }

    void LineCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                               const Point2D& mouseWorldPos, sf::Font& font) const {
        // Dibujamos feedback si estamos esperando el segundo punto o en modo línea continua
        if (state_ == State::WaitingSecondPoint) {
            sf::Color feedbackColor(255, 255, 0, 180);
            
            // Lambda para convertir coordenadas. 
            // ¡Ahora CANVAS_HEIGHT es visible gracias a constants.hpp!
            auto w2s = [&](double x, double y) { 
                return view.worldToScreen(x, y); 
            };
            
            sf::Vertex line[] = { 
                // 3. CORRECCIÓN CLAVE: Usar p1_ (tu variable real) en lugar de startPoint_
                sf::Vertex(w2s(p1_.x, p1_.y), feedbackColor), 
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) 
            };
            window.draw(line, 2, sf::Lines);
        }
    }

} // namespace cad