#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/line.hpp"

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

class ChamferCommand : public ICommand {
public:
    ChamferCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "CHAFLAN"; }

    // >>> NUEVO: Para feedback visual
    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

    // Para feedback visual
    double getDist1() const { return dist1_; }
    double getDist2() const { return dist2_; }
    Line* getLine1() const { return line1_; }
    bool hasLine1() const { return hasLine1_; }

private:
    enum class Step { 
        WaitingDist1, 
        WaitingDist2, 
        WaitingLine1, 
        WaitingLine2 
    };
    Step step_ = Step::WaitingDist1;
    double dist1_ = 0.0;
    double dist2_ = 0.0;
    Line* line1_ = nullptr;
    Line* line2_ = nullptr;  // ← AÑADIR: guardar la segunda línea
    
    bool hasLine1_ = false;
    bool finished_ = false;
    std::string statusMessage_;

    void createChamfer(Engine& engine);
};

} // namespace cad