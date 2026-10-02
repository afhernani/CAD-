#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include "../../core/document/block.hpp" // Necesario para BlockDefinition

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

class BlockInsertCommand : public ICommand {
public:
    BlockInsertCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "INSERTAR"; }

    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

    // Hacer Step público para que app.cpp pueda consultarlo
    enum class Step { WaitingName, WaitingInsertPoint };
    Step getStep() const { return step_; }
    // Getter necesario para el feedback
    BlockDefinition* getDefinition() const { return definition_; }

private:
    Step step_ = Step::WaitingName;
    BlockDefinition* definition_ = nullptr;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad