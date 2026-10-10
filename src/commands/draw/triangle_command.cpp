#include "cad/commands/draw/triangle_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include "cad/core/constants.hpp"
#include <sstream>
#include <iostream>

namespace cad {

    TriangleCommand::TriangleCommand() {
        statusMessage_ = "TRIANGULO | Especificar primer punto:";
        finished_ = false;
        step_ = Step::WaitingP1;
        points_.clear();
    }

    void TriangleCommand::execute(const std::string& input, Engine& engine) {
        std::string upper = input;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        // Cancelar
        if (upper == "C" || upper == "S" || upper == "CANCELAR" || upper == "EXIT") {
            onCancel();
            return;
        }

        // Intentar parsear coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            onPoint({x, y}, engine);
            return;
        }

        // Si no es coordenada ni cancel, recordamos la instrucción
        if (step_ == Step::WaitingP1) {
            statusMessage_ = "TRIANGULO | Especificar primer punto (clic o x,y):";
        } else if (step_ == Step::WaitingP2) {
            statusMessage_ = "TRIANGULO | Especificar segundo punto:";
        } else if (step_ == Step::WaitingP3) {
            statusMessage_ = "TRIANGULO | Especificar tercer punto:";
        }
    }

    void TriangleCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingP1) {
            points_.push_back(point);
            step_ = Step::WaitingP2;
            statusMessage_ = "TRIANGULO | Especificar segundo punto:";
        }
        else if (step_ == Step::WaitingP2) {
            points_.push_back(point);
            step_ = Step::WaitingP3;
            statusMessage_ = "TRIANGULO | Especificar tercer punto:";
        }
        else if (step_ == Step::WaitingP3) {
            points_.push_back(point);
            
            // Crear la polilínea cerrada
            auto pl = std::make_unique<Polyline>();
            pl->id = Entity::generateId();
            pl->points = points_;
            pl->closed = true; // ¡Clave!
            pl->layerName = engine.doc.currentLayerName;
            
            engine.saveState();
            engine.doc.addEntity(std::move(pl));
            
            statusMessage_ = "TRIANGULO | Triángulo creado. Especificar primer punto o ESC para terminar:";
            
            // Resetear para permitir dibujar múltiples triángulos en el mismo comando
            points_.clear();
            step_ = Step::WaitingP1;
        }
    }

    void TriangleCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "TRIANGULO | Comando cancelado.";
    }

    std::string TriangleCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool TriangleCommand::isComplete() const {
        return finished_;
    }

    void TriangleCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                       const Point2D& mouseWorldPos, sf::Font& font) const {
        if (points_.empty()) return;

        sf::Color feedbackColor(255, 255, 0, 200); // Amarillo
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // Dibujar los puntos ya fijados y las líneas entre ellos
        if (points_.size() == 1) {
            // Esperando P2: dibujar línea desde P1 al ratón
            sf::Vertex line[] = {
                sf::Vertex(w2s(points_[0].x, points_[0].y), feedbackColor),
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor)
            };
            window.draw(line, 2, sf::Lines);
            
            // Marcador del P1
            sf::CircleShape dot(3.0f);
            dot.setFillColor(feedbackColor);
            dot.setOrigin(3.0f, 3.0f);
            dot.setPosition(w2s(points_[0].x, points_[0].y));
            window.draw(dot);
        }
        else if (points_.size() == 2) {
            // Esperando P3: dibujar el triángulo dinámico (P1-P2-Ratón-P1)
            sf::VertexArray triangle(sf::LineStrip, 4); // 4 puntos para cerrar
            triangle[0].position = w2s(points_[0].x, points_[0].y);
            triangle[1].position = w2s(points_[1].x, points_[1].y);
            triangle[2].position = w2s(mouseWorldPos.x, mouseWorldPos.y);
            triangle[3].position = w2s(points_[0].x, points_[0].y); // Cerrar
            
            for (int i = 0; i < 4; ++i) triangle[i].color = feedbackColor;
            window.draw(triangle);
            
            // Marcadores de P1 y P2
            for (const auto& p : points_) {
                sf::CircleShape dot(3.0f);
                dot.setFillColor(feedbackColor);
                dot.setOrigin(3.0f, 3.0f);
                dot.setPosition(w2s(p.x, p.y));
                window.draw(dot);
            }
        }
    }

} // namespace cad