// src/commands/draw/polygon_command.cpp
#include "cad/commands/draw/polygon_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO para view.worldToScreen
#include <SFML/Graphics.hpp>               // OBLIGATORIO para sf::VertexArray, sf::Color
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
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
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // Intentar parsear como número (lados o radio)
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
                // Crear el polígono
                auto polygon = std::make_unique<Polygon>();
                polygon->center = center_;
                polygon->sides = sides_;
                polygon->radius = value;
                polygon->layerName = engine.doc.currentLayerName;
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
            // Calcular radio desde centro hasta este punto
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            if (radius < 0.001) {
                statusMessage_ = "POLIGONO | Radio demasiado pequeño. Intenta de nuevo:";
                return;
            }
            
            auto polygon = std::make_unique<Polygon>();
            polygon->center = center_;
            polygon->sides = sides_;
            polygon->radius = radius;
            polygon->layerName = engine.doc.currentLayerName;
            engine.saveState();
            engine.doc.addEntity(std::move(polygon));
            
            statusMessage_ = "Polígono creado (" + std::to_string(sides_) + " lados).";
            finished_ = true;
        }
    }

    void PolygonCommand::onCancel() {
        step_ = Step::WaitingCenter;
        hasCenter_ = false;
        hasSides_ = false;
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string PolygonCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool PolygonCommand::isComplete() const {
        return finished_;
    }

    void PolygonCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasCenter_) return; // Si no hay centro, no dibujamos nada

        sf::Color feedbackColor(255, 255, 0, 180);
        auto w2s = [&](double x, double y) { 
            return view.worldToScreen(x, y); 
        };

        if (step_ == Step::WaitingSides) {
            // FASE 1: Solo dibujamos un punto en el centro para indicar que esperamos el número de lados
            sf::CircleShape dot(4.0f);
            dot.setFillColor(feedbackColor);
            dot.setOrigin(4.0f, 4.0f);
            dot.setPosition(w2s(center_.x, center_.y));
            window.draw(dot);
        } 
        else if (step_ == Step::WaitingRadius) {
            // FASE 2: Ya tenemos centro y lados. Dibujamos el polígono dinámico.
            
            // 1. Calcular radio dinámico basado en el ratón
            double dx = mouseWorldPos.x - center_.x;
            double dy = mouseWorldPos.y - center_.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            // 2. Dibujar el polígono con segmentos
            int sides = sides_;
            // const double PI = 3.14159265358979323846;
            double angleStep = 2.0 * PI / sides;
            
            // Usamos sides + 1 para cerrar el polígono (el último punto coincide con el primero)
            sf::VertexArray va(sf::LineStrip, sides + 1);
            
            for (int i = 0; i <= sides; ++i) {
                // Restamos PI/2 para que el primer vértice apunte hacia arriba (estándar CAD)
                double angle = i * angleStep - (PI / 2.0);
                va[i].position = w2s(center_.x + radius * std::cos(angle), 
                                     center_.y + radius * std::sin(angle));
                va[i].color = feedbackColor;
            }
            window.draw(va);
            
            // 3. Dibujar línea guía desde el centro hasta el ratón (ayuda visual clásica)
            sf::Vertex guideLine[] = { 
                sf::Vertex(w2s(center_.x, center_.y), feedbackColor), 
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) 
            };
            window.draw(guideLine, 2, sf::Lines);
        }
    }

} // namespace cad