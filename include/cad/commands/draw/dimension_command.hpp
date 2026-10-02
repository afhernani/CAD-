#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/dimension.hpp"
#include "../../core/geometry/entities/circle.hpp"
#include "../../core/geometry/entities/arc.hpp"

namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

class DimensionCommand : public ICommand {
public:
    // >>> SOLO DECLARACIONES (sin cuerpo)
    DimensionCommand(); 
    explicit DimensionCommand(DimType type);
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override;

    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

private:
    enum class Step { WaitingP1, WaitingP2, WaitingLocation, WaitingEntity, WaitingEntityLocation };
    Step step_ = Step::WaitingP1;
    
    DimType type_;
    Point2D p1_, p2_;
    Entity* selectedCircleOrArc_ = nullptr; // Para Radio/Diámetro
    
    bool hasP1_ = false;
    bool hasP2_ = false;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad