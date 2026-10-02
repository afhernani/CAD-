#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/line.hpp"

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

class FilletCommand : public ICommand {
public:
    FilletCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "EMPALME"; }

    // >>> NUEVO: Para feedback visual
    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

    // >>> NUEVOS: Para feedback visual
    double getRadius() const { return radius_; }
    bool hasRadius() const { return hasRadius_; }
    Line* getLine1() const { return line1_; }
    Line* getLine2() const { return line2_; }
    bool hasLine1() const { return hasLine1_; }

private:
    enum class Step { 
        WaitingRadius, 
        WaitingLine1, 
        WaitingLine2 
    };
    Step step_ = Step::WaitingRadius;
    double radius_ = 0.0;
    Line* line1_ = nullptr;
    Line* line2_ = nullptr;
    
    bool hasRadius_ = false;
    bool hasLine1_ = false;
    bool finished_ = false;
    std::string statusMessage_;

    // Método auxiliar para crear el empalme
    void createFillet(Engine& engine);
};

} // namespace cad