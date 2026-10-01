// src/commands/draw/arc_command.cpp
#include "cad/commands/draw/arc_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO para view.worldToScreen
#include <SFML/Graphics.hpp>               // OBLIGATORIO para sf::VertexArray, sf::Color
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>
#include <algorithm>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }

    ArcCommand::ArcCommand() {
        statusMessage_ = "ARCO | Especificar centro:";
    }

    void ArcCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // Si no es coordenada, no lo aceptamos (solo puntos)
        statusMessage_ = "ARCO | Formato no válido. Especifica un punto (x,y):";
    }

    void ArcCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingCenter) {
            center_ = point;
            hasCenter_ = true;
            step_ = Step::WaitingStartPoint;
            statusMessage_ = "ARCO | Especificar punto de inicio (define radio y ángulo inicial):";
        }
        else if (step_ == Step::WaitingStartPoint) {
            startPoint_ = point;
            hasStartPoint_ = true;
            
            // Calcular radio y ángulo inicial
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            radius_ = std::sqrt(dx * dx + dy * dy);
            startAngle_ = std::atan2(dy, dx) * 180.0 / PI;
            if (startAngle_ < 0) startAngle_ += 360.0;
            
            step_ = Step::WaitingEndPoint;
            statusMessage_ = "ARCO | Especificar punto final:";
        }
        else if (step_ == Step::WaitingEndPoint) {
            // Calcular ángulo final
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            double endAngle = std::atan2(dy, dx) * 180.0 / PI;
            if (endAngle < 0) endAngle += 360.0;
            
            // Crear el arco
            auto arc = std::make_unique<Arc>();
            arc->center = center_;
            arc->radius = radius_;
            arc->startAngle = startAngle_;
            arc->endAngle = endAngle;
            arc->layerName = engine.doc.currentLayerName;
            engine.saveState();
            engine.doc.addEntity(std::move(arc));
            
            statusMessage_ = "Arco creado.";
            finished_ = true;
        }
    }

    void ArcCommand::onCancel() {
        step_ = Step::WaitingCenter;
        hasCenter_ = false;
        hasStartPoint_ = false;
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string ArcCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool ArcCommand::isComplete() const {
        return finished_;
    }

    void ArcCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                  const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasCenter_) return; // Si no hay centro, no dibujamos nada

        sf::Color feedbackColor(255, 255, 0, 180);
        auto w2s = [&](double x, double y) { 
            return view.worldToScreen(x, y); 
        };

        if (step_ == Step::WaitingStartPoint) {
            // FASE 1: Dibujar línea guía desde el centro hasta el ratón (para definir el radio)
            sf::Vertex line[] = { 
                sf::Vertex(w2s(center_.x, center_.y), feedbackColor), 
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) 
            };
            window.draw(line, 2, sf::Lines);
        } 
        else if (step_ == Step::WaitingEndPoint) {
            // FASE 2: Ya tenemos centro y punto de inicio. Dibujamos el arco dinámico.
            
            // 1. Calcular ángulo final basado en el ratón
            double dx = mouseWorldPos.x - center_.x;
            double dy = mouseWorldPos.y - center_.y;
            double endAngle = std::atan2(dy, dx);
            
            // 2. Calcular ángulo inicial basado en el startPoint guardado
            double startDx = startPoint_.x - center_.x;
            double startDy = startPoint_.y - center_.y;
            double startRadAngle = std::atan2(startDy, startDx);
            
            // 3. Dibujar el arco con segmentos
            const int numPoints = 64;
            sf::VertexArray va(sf::LineStrip, numPoints);
            const double PI = 3.14159265358979323846;
            
            // Asegurar que el arco se dibuje en sentido antihorario (estándar CAD)
            double diff = endAngle - startRadAngle;
            while (diff < 0) diff += 2 * PI; 
            while (diff >= 2 * PI) diff -= 2 * PI;
            
            double step = diff / (numPoints - 1);
            for (int i = 0; i < numPoints; ++i) {
                double angle = startRadAngle + i * step;
                va[i].position = w2s(center_.x + radius_ * std::cos(angle), 
                                     center_.y + radius_ * std::sin(angle));
                va[i].color = feedbackColor;
            }
            window.draw(va);
            
            // 4. Dibujar línea guía desde el centro hasta el ratón (ayuda visual clásica de AutoCAD)
            sf::Vertex guideLine[] = { 
                sf::Vertex(w2s(center_.x, center_.y), feedbackColor), 
                sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) 
            };
            window.draw(guideLine, 2, sf::Lines);
        }
    }

} // namespace cad