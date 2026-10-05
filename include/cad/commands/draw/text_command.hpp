#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/text.hpp"

namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

class TextCommand : public ICommand {
public:
    TextCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "TEXTO"; }

    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

private:
    enum class Step { WaitingPoint, WaitingHeight, WaitingRotation, WaitingText };
    Step step_ = Step::WaitingPoint;
    
    Point2D position_;
    double height_ = 1.0;
    double rotation_ = 0.0;
    std::string content_;
    
    bool finished_ = false;
    std::string statusMessage_;

    // Método auxiliar para crear la entidad y no repetir código
    void createTextEntity(Engine& engine);
};

} // namespace cad