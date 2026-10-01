#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/ellipse.hpp"

// Forward declarations para evitar incluir SFML aquí
namespace sf { class RenderWindow; class Font; }

namespace cad {

class EllipseCommand : public ICommand {
public:
    EllipseCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "ELIPSE"; }

    // >>> NUEVO: Método de feedback visual <<<
        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                          const Point2D& mouseWorldPos, sf::Font& font) const override;
    // Para feedback visual
    Point2D getCenter() const { return center_; }
    Point2D getMajorAxisEnd() const { return majorAxisEnd_; }
    double getMajorRadius() const { return majorRadius_; }
    double getRotationAngle() const { return rotationAngle_; }
    bool hasCenter() const { return hasCenter_; }
    bool hasMajorAxis() const { return hasMajorAxis_; }

private:
    enum class Step { 
        WaitingCenter, 
        WaitingMajorAxis, 
        WaitingMinorRadius 
    };
    Step step_ = Step::WaitingCenter;
    Point2D center_;
    Point2D majorAxisEnd_;
    double majorRadius_ = 0.0;
    double rotationAngle_ = 0.0;
    
    bool hasCenter_ = false;
    bool hasMajorAxis_ = false;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad