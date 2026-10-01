#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/circle.hpp"

// Forward declarations para evitar incluir SFML aquí
namespace sf { class RenderWindow; class Font; }

namespace cad {

    class CircleCommand : public ICommand {
    public:
        CircleCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "CIRCULO"; }

        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                          const Point2D& mouseWorldPos, sf::Font& font) const override;
                          
        Point2D getCenter() const { return center_; }
        bool hasCenter() const { return hasCenter_; }
        bool isWaitingForPoint() const { return step_ == Step::WaitingForPoint; }
        bool isWaitingForRadius() const { return step_ == Step::WaitingRadius; } // Ajusta al nombre de tu enu

    private:
        enum class Step { 
            WaitingCenter, 
            WaitingRadiusOption,  // ← NUEVO: Pregunta R/D/Punto
            WaitingRadius,        // Esperando valor numérico
            WaitingForPoint       // Esperando clic para definir radio
        };
        Step step_ = Step::WaitingCenter;
        Point2D center_;
        bool hasCenter_ = false;
        bool finished_ = false;
        std::string statusMessage_;
    };

} // namespace cad