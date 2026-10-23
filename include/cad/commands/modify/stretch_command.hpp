#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include <vector>
#include <algorithm>

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

    class StretchCommand : public ICommand {
    public:
        StretchCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "ESTIRAR"; }

        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                          const Point2D& mouseWorldPos, sf::Font& font) const override;

    private:
        enum class Step { SelectingWindowP1, SelectingWindowP2, WaitingBasePoint, WaitingDestPoint };
        
        Step step_ = Step::SelectingWindowP1;
        Point2D windowP1_;
        Point2D windowP2_;
        bool hasWindowP1_ = false;
        bool hasWindow_ = false;
        Point2D basePoint_;
        bool hasBasePoint_ = false;
        Point2D destPoint_;
        std::vector<Entity*> selectedEntities_;
        bool finished_ = false;
        std::string statusMessage_;

        void performWindowSelection(Engine& engine);
        void executeStretch(Engine& engine);
    };

} // namespace cad