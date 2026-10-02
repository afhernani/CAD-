#include "cad/commands/modify/rotate_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    // Helper local para convertir UTF-8 a sf::String
    namespace {
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
    }

    RotateCommand::RotateCommand() {
        statusMessage_ = "ROTAR | Centro de rotación:";
    }

    void RotateCommand::execute(const std::string& input, Engine& engine) {
        // 1. Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Intentar parsear como ángulo (número en grados)
        try {
            double angle = std::stod(input);
            
            if (step_ == Step::Angle) {
                // Guardar estado para Undo
                engine.saveState();
                
                // Aplicar rotación a las entidades seleccionadas
                for (Entity* entity : engine.selectedEntities) {
                    entity->rotate(basePoint_, angle);
                }
                
                statusMessage_ = "Entidades rotadas " + std::to_string(angle) + "°.";
                finished_ = true;
                
                // Limpiar selección
                engine.selectedEntities.clear();
            } else {
                statusMessage_ = "ROTAR | Primero especifica el centro de rotación.";
            }
        } catch (...) {
            statusMessage_ = "ROTAR | Formato inválido. Usa coordenadas x,y o un ángulo en grados.";
        }
    }

    void RotateCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::BasePoint) {
            basePoint_ = point;
            hasCenter_ = true;
            step_ = Step::Angle;
            statusMessage_ = "ROTAR | Ángulo de rotación (grados) o punto:";
        } 
        else if (step_ == Step::Angle) {
            // Calcular ángulo entre el punto base y el punto actual
            double dx = point.x - basePoint_.x;
            double dy = point.y - basePoint_.y;
            double angle = std::atan2(dy, dx) * 180.0 / PI;
            
            // Guardar estado para Undo
            engine.saveState();
            
            // Aplicar rotación
            for (Entity* entity : engine.selectedEntities) {
                entity->rotate(basePoint_, angle);
            }
            
            statusMessage_ = "Entidades rotadas " + std::to_string(angle) + "°.";
            finished_ = true;
            
            // Limpiar selección
            engine.selectedEntities.clear();
        }
    }

    void RotateCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string RotateCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool RotateCommand::isComplete() const {
        return finished_;
    }

    void RotateCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasCenter_) return;

        sf::Color originalColor(255, 165, 0, 180); // Naranja translúcido
        sf::Color ghostColor(0, 200, 255, 120);     // Cian fantasma
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

        sf::CircleShape centerMark(5.0f);
        centerMark.setFillColor(axisColor);
        centerMark.setOrigin(5.0f, 5.0f);
        centerMark.setPosition(w2s(basePoint_.x, basePoint_.y));
        window.draw(centerMark);

        // 3. Dibujar "fantasmas" rotados y calcular ángulo
        double dx = mouseWorldPos.x - basePoint_.x;
        double dy = mouseWorldPos.y - basePoint_.y;
        double angle = std::atan2(dy, dx) * 180.0 / PI;

        for (Entity* e : engine.selectedEntities) {
            auto ghost = e->clone();
            ghost->rotate(basePoint_, angle); // Asumiendo que tu clase Entity tiene el método rotate()
            ghost->draw(window, w2s, ghostColor, view.getScale());
        }

        // 4. Dibujar texto del ángulo cerca del cursor
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << angle << "°";
        
        sf::Text angleText;
        angleText.setFont(font);
        // Usamos sf::String estándar para evitar dependencias de helpers locales
        angleText.setString(toSfString(oss.str()));
        //angleText.setString(oss.str());
        angleText.setCharacterSize(14);
        angleText.setFillColor(sf::Color::Yellow);
        angleText.setStyle(sf::Text::Bold);
        
        sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y);
        angleText.setPosition(mouseScreen.x + 12.f, mouseScreen.y - 25.f);
        window.draw(angleText);
    }

} // namespace cad