#include "cad/commands/draw/circle_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO para usar view.worldToScreen
#include <SFML/Graphics.hpp>               // OBLIGATORIO para sf::VertexArray, sf::Color
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    namespace{
        constexpr double PI = 3.14159265358979323846;
    }

    CircleCommand::CircleCommand() {
        statusMessage_ = "CIRCULO | Especificar centro:";
    }

    void CircleCommand::execute(const std::string& input, Engine& engine) {
        // 1. Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Si no es coordenada, verificar si es opción R/D
        std::string upperInput = input;
        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
        
        if (step_ == Step::WaitingRadiusOption) {
            if (upperInput == "R" || upperInput == "RADIO") {
                step_ = Step::WaitingRadius;
                statusMessage_ = "CIRCULO | Valor del radio:";
                return;
            }
            else if (upperInput == "D" || upperInput == "DIAMETRO") {
                step_ = Step::WaitingRadius;
                statusMessage_ = "CIRCULO | Valor del diámetro:";
                return;
            }
            else {
                // Intentar como número (asumir radio por defecto)
                try {
                    double value = std::stod(input);
                    if (value > 0) {
                        double radius = value;
                        // Si el usuario escribió directamente un número sin R/D, asumir radio
                        auto circle = std::make_unique<Circle>();
                        circle->id = Entity::generateId(); // Generar un ID único
                        circle->center = center_;
                        circle->radius = radius;
                        circle->layerName = engine.doc.currentLayerName;
                        engine.saveState();
                        engine.doc.addEntity(std::move(circle));
                        
                        statusMessage_ = "Círculo creado (radio: " + std::to_string(radius) + ")";
                        finished_ = true;
                    } else {
                        statusMessage_ = "CIRCULO | Valor inválido.";
                    }
                } catch (...) {
                    statusMessage_ = "CIRCULO | Opción no válida. Usa R (Radio), D (Diámetro) o punto:";
                }
                return;
            }
        }
        
        // 3. Si estamos esperando valor numérico de radio/diámetro
        if (step_ == Step::WaitingRadius) {
            try {
                double value = std::stod(input);
                if (value <= 0) {
                    statusMessage_ = "CIRCULO | Valor inválido.";
                    return;
                }
                
                double radius = value;
                // Si el mensaje dice "diámetro", dividir entre 2
                if (statusMessage_.find("diámetro") != std::string::npos ||
                    statusMessage_.find("Diámetro") != std::string::npos) {
                    radius = value / 2.0;
                }
                
                auto circle = std::make_unique<Circle>();
                circle->id = Entity::generateId(); // Generar un ID único
                circle->center = center_;
                circle->radius = radius;
                circle->layerName = engine.doc.currentLayerName;
                engine.doc.addEntity(std::move(circle));
                
                statusMessage_ = "Círculo creado (radio: " + std::to_string(radius) + ")";
                finished_ = true;
            } catch (...) {
                statusMessage_ = "CIRCULO | Valor numérico inválido.";
            }
            return;
        }
        
        statusMessage_ = "CIRCULO | Formato inválido.";
    }

    void CircleCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingCenter) {
            center_ = point;
            hasCenter_ = true;
            step_ = Step::WaitingRadiusOption;
            statusMessage_ = "CIRCULO | Radio/Diámetro/Punto <Radio>:";
        }
        else if (step_ == Step::WaitingRadiusOption || step_ == Step::WaitingForPoint) {
            // Calcular radio desde centro hasta este punto
            double dx = point.x - center_.x;
            double dy = point.y - center_.y;
            double radius = std::sqrt(dx * dx + dy * dy);
            
            if (radius < 0.001) {
                statusMessage_ = "CIRCULO | Radio demasiado pequeño. Intenta de nuevo:";
                return;
            }
            
            auto circle = std::make_unique<Circle>();
            circle->id = Entity::generateId(); // Generar un ID único
            circle->center = center_;
            circle->radius = radius;
            circle->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(circle));
            
            statusMessage_ = "Círculo creado (radio: " + std::to_string(radius) + ")";
            finished_ = true;
        }
    }

    void CircleCommand::onCancel() {
        step_ = Step::WaitingCenter;
        hasCenter_ = false;
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string CircleCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool CircleCommand::isComplete() const {
        return finished_;
    }

    void CircleCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        // Solo dibujamos feedback cuando tenemos el centro y estamos esperando 
        // que el usuario defina el radio haciendo clic (WaitingForPoint)
        if (hasCenter_ && step_ != Step::WaitingCenter) {
            sf::Color feedbackColor(255, 255, 0, 180);
            
            // Lambda para convertir coordenadas
            auto w2s = [&](double x, double y) { 
                return view.worldToScreen(x, y); 
            };

            // Calcular radio dinámico basado en la posición del ratón
            double dx = mouseWorldPos.x - center_.x;
            double dy = mouseWorldPos.y - center_.y;
            double radius = std::sqrt(dx * dx + dy * dy);

            // Dibujar el círculo aproximado con segmentos
            const int numPoints = 64;
            sf::VertexArray va(sf::LineStrip, numPoints + 1);
            // const double PI = 3.14159265358979323846;
            double angleStep = 2.0 * PI / numPoints;

            for (int i = 0; i <= numPoints; ++i) {
                double angle = i * angleStep;
                va[i].position = w2s(center_.x + radius * std::cos(angle), 
                                     center_.y + radius * std::sin(angle));
                va[i].color = feedbackColor;
            }
            
            window.draw(va);
        }
    }

} // namespace cad