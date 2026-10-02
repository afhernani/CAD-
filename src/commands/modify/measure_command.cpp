#include "cad/commands/modify/measure_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>
#include <iomanip>
#include <numbers>

namespace cad {

    namespace{
        constexpr double PI = 3.14159265358979323846;
    }

    // Helper local para convertir UTF-8 a sf::String (por si no está en un header compartido)
    namespace {
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
    }

    MeasureCommand::MeasureCommand() {
        statusMessage_ = "DIST | Especificar primer punto:";
    }

    void MeasureCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        statusMessage_ = "DIST | Formato no válido. Usa coordenadas x,y.";
    }

    void MeasureCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingFirstPoint) {
            firstPoint_ = point;
            hasFirstPoint_ = true;
            step_ = Step::WaitingSecondPoint;
            statusMessage_ = "DIST | Especificar segundo punto:";
        }
        else if (step_ == Step::WaitingSecondPoint) {
            double dx = point.x - firstPoint_.x;
            double dy = point.y - firstPoint_.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            double angle = std::atan2(dy, dx) * 180.0 / PI;
            
            std::ostringstream ossDist;
            ossDist << std::fixed << std::setprecision(2);
            ossDist << "Distancia: " << dist
                    << ", Angulo XY: " << angle << " deg"
                    << ", DX: " << dx
                    << ", DY: " << dy;
            
            statusMessage_ = ossDist.str();
            finished_ = true;
        }
    }

    void MeasureCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string MeasureCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool MeasureCommand::isComplete() const {
        return finished_;
    }

    void MeasureCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasFirstPoint_) return;

        sf::Color lineColor(255, 255, 0, 200);   // Amarillo para la línea
        sf::Color textColor(255, 255, 255, 255); // Blanco para el texto

        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. Dibujar línea guía desde el primer punto hasta el cursor
        sf::Vertex guideLine[] = {
            sf::Vertex(w2s(firstPoint_.x, firstPoint_.y), lineColor),
            sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), lineColor)
        };
        window.draw(guideLine, 2, sf::Lines);

        // 2. Calcular la distancia
        double dx = mouseWorldPos.x - firstPoint_.x;
        double dy = mouseWorldPos.y - firstPoint_.y;
        double distance = std::sqrt(dx * dx + dy * dy);

        // 3. Dibujar el texto de la distancia
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << "Dist: " << distance;

        sf::Text distText;
        distText.setFont(font);
        distText.setString(toSfString(oss.str()));
        distText.setCharacterSize(14);
        distText.setFillColor(textColor);
        distText.setStyle(sf::Text::Bold);

        sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y);

        // Fondo semitransparente para que el texto se lea sobre cualquier entidad
        sf::FloatRect bounds = distText.getLocalBounds();
        sf::RectangleShape bg(sf::Vector2f(bounds.width + 8, bounds.height + 4));
        bg.setFillColor(sf::Color(0, 0, 0, 180));
        bg.setPosition(mouseScreen.x + 13.f, mouseScreen.y - 32.f);
        window.draw(bg);

        distText.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 30.f);
        window.draw(distText);
    }

} // namespace cad