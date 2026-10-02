#include "cad/commands/modify/scale_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    // Helper local para convertir UTF-8 a sf::String (igual que en renderer.cpp)
    namespace {
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
    }

    ScaleCommand::ScaleCommand() {
        statusMessage_ = "ESCALAR | Punto base:";
    }

    void ScaleCommand::execute(const std::string& input, Engine& engine) {
        // 1. Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Intentar parsear como factor de escala (número)
        try {
            double factor = std::stod(input);
            
            if (step_ == Step::Factor && factor > 0) {
                // Guardar estado para Undo
                engine.saveState();
                
                // Aplicar escala a las entidades seleccionadas
                for (Entity* entity : engine.selectedEntities) {
                    entity->scale(basePoint_, factor);
                }
                
                statusMessage_ = "Entidades escaladas (factor: " + std::to_string(factor) + ").";
                finished_ = true;
                
                // Limpiar selección
                engine.selectedEntities.clear();
            } else {
                statusMessage_ = "ESCALAR | Factor inválido. Debe ser mayor que 0.";
            }
        } catch (...) {
            statusMessage_ = "ESCALAR | Formato inválido. Usa coordenadas x,y o un número.";
        }
    }

    void ScaleCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::BasePoint) {
            basePoint_ = point;
            hasBasePoint_ = true;
            step_ = Step::Factor;
            statusMessage_ = "ESCALAR | Factor de escala (número) o punto:";
        } 
        else if (step_ == Step::Factor) {
            // Calcular factor como distancia desde el punto base hasta el punto actual
            double dx = point.x - basePoint_.x;
            double dy = point.y - basePoint_.y;
            double factor = std::sqrt(dx * dx + dy * dy);
            
            if (factor < 0.001) {
                statusMessage_ = "ESCALAR | Factor demasiado pequeño. Intenta de nuevo:";
                return;
            }
            
            // Guardar estado para Undo
            engine.saveState();
            
            // Aplicar escala
            for (Entity* entity : engine.selectedEntities) {
                entity->scale(basePoint_, factor);
            }
            
            statusMessage_ = "Entidades escaladas (factor: " + std::to_string(factor) + ").";
            finished_ = true;
            
            // Limpiar selección
            engine.selectedEntities.clear();
        }
    }

    void ScaleCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string ScaleCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool ScaleCommand::isComplete() const {
        return finished_;
    }

    void ScaleCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                    const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasBasePoint_) return;

        sf::Color originalColor(255, 165, 0, 180); // Naranja translúcido
        sf::Color ghostColor(255, 100, 255, 120);  // Magenta fantasma
        sf::Color axisColor(255, 255, 0, 200);     // Amarillo para guías

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

        sf::CircleShape baseMark(5.0f);
        baseMark.setFillColor(axisColor);
        baseMark.setOrigin(5.0f, 5.0f);
        baseMark.setPosition(w2s(basePoint_.x, basePoint_.y));
        window.draw(baseMark);

        // 3. Dibujar "fantasmas" escalados y calcular factor
        double dx = mouseWorldPos.x - basePoint_.x;
        double dy = mouseWorldPos.y - basePoint_.y;
        double factor = std::sqrt(dx * dx + dy * dy);

        for (Entity* e : engine.selectedEntities) {
            auto ghost = e->clone();
            ghost->scale(basePoint_, factor); 
            ghost->draw(window, w2s, ghostColor, view.getScale());
        }

        // 4. Dibujar texto del factor cerca del cursor
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << factor << "x";
        
        sf::Text factorText;
        factorText.setFont(font);
        factorText.setString(toSfString(oss.str()));
        factorText.setCharacterSize(14);
        factorText.setFillColor(sf::Color::Magenta);
        factorText.setStyle(sf::Text::Bold);
        
        sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y);
        
        // Fondo semitransparente para mejor legibilidad
        sf::FloatRect bounds = factorText.getLocalBounds();
        sf::RectangleShape bg(sf::Vector2f(bounds.width + 8, bounds.height + 4));
        bg.setFillColor(sf::Color(0, 0, 0, 180));
        bg.setPosition(mouseScreen.x + 13.f, mouseScreen.y - 32.f);
        window.draw(bg);
        
        factorText.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 30.f);
        window.draw(factorText);
    }

} // namespace cad