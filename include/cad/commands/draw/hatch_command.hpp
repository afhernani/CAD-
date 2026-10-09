#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/hatch.hpp"

namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

    class HatchCommand : public ICommand {
    public:
        HatchCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "SOMBREADO"; }

        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                        const Point2D& mouseWorldPos, sf::Font& font) const override;

    private:
        enum class Step { SelectingObject, Confirming,  DefiningAngle, DefiningSpacing, Finished };
    
        Step step_ = Step::SelectingObject;
        bool finished_ = false;
        std::string statusMessage_;
        
        Entity* selectedEntity_ = nullptr;
        std::vector<Point2D> hatchPoints_;
        double angle_ = 0.0;
        double spacing_ = 1.0;
        
        std::vector<Point2D> extractPointsFromEntity(Entity* entity) const;
    };

} // namespace cad