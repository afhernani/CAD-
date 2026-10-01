#include "cad/commands/draw/ellipse_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO para view.worldToScreen
#include <SFML/Graphics.hpp>               // OBLIGATORIO para sf::VertexArray, sf::Color
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    EllipseCommand::EllipseCommand() {
        statusMessage_ = "ELIPSE | Especificar centro:";
    }

    void EllipseCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // Intentar parsear como número (radio menor)
        try {
            double value = std::stod(input);
            
            if (step_ == Step::WaitingMinorRadius && value > 0) {
                // Crear la elipse
                auto ellipse = std::make_unique<Ellipse>();
                ellipse->center = center_;
                ellipse->majorRadius = majorRadius_;
                ellipse->minorRadius = value;
                ellipse->rotationAngle = rotationAngle_;
                ellipse->layerName = engine.doc.currentLayerName;
                engine.saveState();
                engine.doc.addEntity(std::move(ellipse));
                
                statusMessage_ = "Elipse creada.";
                finished_ = true;
            }
            else {
                statusMessage_ = "ELIPSE | Valor no válido.";
            }
        } catch (...) {
            statusMessage_ = "ELIPSE | Formato no válido. Usa coordenadas x,y o un número.";
        }
    }

    void EllipseCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingCenter) {
            center_ = point;
            hasCenter_ = true;
            step_ = Step::WaitingMajorAxis;
            statusMessage_ = "ELIPSE | Punto final del eje mayor:";
        }
        else if (step_ == Step::WaitingMajorAxis) {
            majorAxisEnd_ = point;
            hasMajorAxis_ = true;
            
            // Calcular radio mayor y rotación
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            majorRadius_ = std::sqrt(dx * dx + dy * dy);
            rotationAngle_ = std::atan2(dy, dx);
            
            step_ = Step::WaitingMinorRadius;
            statusMessage_ = "ELIPSE | Radio del eje menor (número) o punto:";
        }
        else if (step_ == Step::WaitingMinorRadius) {
            // Calcular radio menor desde centro hasta este punto
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            double minorRadius = std::sqrt(dx * dx + dy * dy);
            
            if (minorRadius < 0.001) {
                statusMessage_ = "ELIPSE | Radio demasiado pequeño. Intenta de nuevo:";
                return;
            }
            
            auto ellipse = std::make_unique<Ellipse>();
            ellipse->center = center_;
            ellipse->majorRadius = majorRadius_;
            ellipse->minorRadius = minorRadius;
            ellipse->rotationAngle = rotationAngle_;
            ellipse->layerName = engine.doc.currentLayerName;
            engine.saveState();
            engine.doc.addEntity(std::move(ellipse));
            
            statusMessage_ = "Elipse creada.";
            finished_ = true;
        }
    }

    void EllipseCommand::onCancel() {
        step_ = Step::WaitingCenter;
        hasCenter_ = false;
        hasMajorAxis_ = false;
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string EllipseCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool EllipseCommand::isComplete() const {
        return finished_;
    }

    void EllipseCommand::drawFeedback(sf::RenderWindow& window, const View& view, 
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasCenter_) return; // Si no hay centro, no dibujamos nada

        sf::Color feedbackColor(255, 255, 0, 180);
        auto w2s = [&](double x, double y) { 
            return view.worldToScreen(x, y); 
        };

        if (step_ == Step::WaitingMajorAxis) {
            // FASE 1: Dibujar línea guía desde el centro hasta el ratón (define eje mayor y rotación)
            sf::Vertex line[] = { 
                sf::Vertex(w2s(center_.x, center_.y), feedbackColor), 
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) 
            };
            window.draw(line, 2, sf::Lines);
        } 
        else if (step_ == Step::WaitingMinorRadius) {
            // FASE 2: Ya tenemos centro y eje mayor. Dibujamos la elipse dinámica.
            
            // 1. Calcular radio menor dinámico basado en el ratón
            double dx = mouseWorldPos.x - center_.x;
            double dy = mouseWorldPos.y - center_.y;
            double minorRadius = std::sqrt(dx * dx + dy * dy);
            
            // 2. Dibujar la elipse con segmentos
            const int numPoints = 64;
            sf::VertexArray va(sf::LineStrip, numPoints + 1);
            const double PI = 3.14159265358979323846;
            double angleStep = 2.0 * PI / numPoints;
            
            for (int i = 0; i <= numPoints; ++i) {
                double angle = i * angleStep;
                double rotatedAngle = angle + rotationAngle_;
                va[i].position = w2s(center_.x + majorRadius_ * std::cos(rotatedAngle), 
                                     center_.y + minorRadius * std::sin(rotatedAngle));
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