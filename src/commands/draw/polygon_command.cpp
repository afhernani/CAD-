#include "cad/commands/draw/polygon_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include "cad/core/constants.hpp"
#include <sstream>
#include <cmath>

namespace cad {

    namespace{
        constexpr double PI = 3.14159265358979323846;
    }

    PolygonCommand::PolygonCommand() {
        statusMessage_ = "POLIGONO | Especificar centro:";
    }

    void PolygonCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        try {
            double value = std::stod(input);
            if (step_ == Step::WaitingSides) {
                int numSides = static_cast<int>(value);
                if (numSides < 3) {
                    statusMessage_ = "POLIGONO | Mínimo 3 lados. Intenta de nuevo:";
                    return;
                }
                sides_ = numSides;
                hasSides_ = true;
                step_ = Step::WaitingRadius;
                statusMessage_ = "POLIGONO | Radio (número) o punto:";
            }
            else if (step_ == Step::WaitingRadius && value > 0) {
                auto polygon = std::make_unique<Polygon>();
                polygon->id = Entity::generateId();
                polygon->center = center_;
                polygon->sides = sides_;
                polygon->radius = value;
                polygon->layerName = engine.doc.currentLayerName;
                
                double angleStep = 2.0 * PI / sides_;
                double offset = -PI / 2.0; 
                for (int i = 0; i < sides_; ++i) {
                    double angle = i * angleStep + offset;
                    polygon->points.push_back({
                        center_.x + value * std::cos(angle),
                        center_.y + value * std::sin(angle)
                    });
                }
                
                engine.saveState();
                engine.doc.addEntity(std::move(polygon));
                statusMessage_ = "Polígono creado (" + std::to_string(sides_) + " lados).";
                finished_ = true;
            }
            else {
                statusMessage_ = "POLIGONO | Valor no válido.";
            }
        } catch (...) {
            statusMessage_ = "POLIGONO | Formato no válido. Usa coordenadas x,y o un número.";
        }
    }

    void PolygonCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingCenter) {
            center_ = point;
            hasCenter_ = true;
            step_ = Step::WaitingSides;
            statusMessage_ = "POLIGONO | Número de lados (ej: 6):";
        }
        else if (step_ == Step::WaitingRadius) {
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            if (radius < 0.001) {
                statusMessage_ = "POLIGONO | Radio demasiado pequeño. Intenta de nuevo:";
                return;
            }
            
            auto polygon = std::make_unique<Polygon>();
            polygon->id = Entity::generateId();
            polygon->center = center_;
            polygon->sides = sides_;
            polygon->radius = radius;
            polygon->layerName = engine.doc.currentLayerName;
            
            double angleStep = 2.0 * PI / sides_;
            double offset = -PI / 2.0;
            for (int i = 0; i < sides_; ++i) {
                double angle = i * angleStep + offset;
                polygon->points.push_back({
                    center_.x + radius * std::cos(angle),
                    center_.y + radius * std::sin(angle)
                });
            }
            
            engine.saveState();
            engine.doc.addEntity(std::move(polygon));
            statusMessage_ = "Polígono creado (" + std::to_string(sides_) + " lados).";
            finished_ = true;
        }
    }

    void PolygonCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string PolygonCommand::getStatusMessage() const { return statusMessage_; }
    bool PolygonCommand::isComplete() const { return finished_; }

    void PolygonCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasCenter_) return;

        sf::Color feedbackColor(255, 255, 0, 180);
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        if (step_ == Step::WaitingSides) {
            sf::CircleShape dot(4.0f);
            dot.setFillColor(feedbackColor);
            dot.setOrigin(4.0f, 4.0f);
            dot.setPosition(w2s(center_.x, center_.y));
            window.draw(dot);
        } 
        else if (step_ == Step::WaitingRadius) {
            double dx = mouseWorldPos.x - center_.x;
            double dy = mouseWorldPos.y - center_.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            int sides = sides_;
            double angleStep = 2.0 * PI / sides;
            double offset = -PI / 2.0;
            
            sf::VertexArray va(sf::LineStrip, sides + 1);
            for (int i = 0; i <= sides; ++i) {
                double angle = i * angleStep + offset;
                va[i].position = w2s(center_.x + radius * std::cos(angle), 
                                     center_.y + radius * std::sin(angle));
                va[i].color = feedbackColor;
            }
            window.draw(va);
            
            sf::Vertex guideLine[] = { 
                sf::Vertex(w2s(center_.x, center_.y), feedbackColor), 
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) 
            };
            window.draw(guideLine, 2, sf::Lines);
        }
    }

} // namespace cad