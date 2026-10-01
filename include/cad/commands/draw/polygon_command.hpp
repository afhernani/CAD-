#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/polygon.hpp"

// Forward declarations para evitar incluir SFML aquí
namespace sf { class RenderWindow; class Font; }

namespace cad {

class PolygonCommand : public ICommand {
public:
    PolygonCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "POLIGONO"; }

    // >>> NUEVO: Método de feedback visual <<<
        void drawFeedback(sf::RenderWindow& window, const View& view, 
                          const Point2D& mouseWorldPos, sf::Font& font) const override;

    // Para feedback visual
    Point2D getCenter() const { return center_; }
    int getSides() const { return sides_; }
    bool hasCenter() const { return hasCenter_; }
    bool hasSides() const { return hasSides_; }

private:
    enum class Step { 
        WaitingCenter, 
        WaitingSides, 
        WaitingRadius 
    };
    Step step_ = Step::WaitingCenter;
    Point2D center_;
    int sides_ = 0;
    
    bool hasCenter_ = false;
    bool hasSides_ = false;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad