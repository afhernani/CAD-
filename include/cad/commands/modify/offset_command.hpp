#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp" 

namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

    class OffsetCommand : public ICommand {
    public:
        OffsetCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "DESPLAZAR"; }

        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                        const Point2D& mouseWorldPos, sf::Font& font) const override;

        double getDistance() const { return distance_; }
        bool hasDistance() const { return hasDistance_; }
        Entity* getSelectedEntity() const { return selectedEntity_; }

    private:
        enum class Step { 
            WaitingDistance, 
            WaitingDistancePoint2,  // <<< NUEVO: Para diferenciar el segundo clic
            WaitingEntity, 
            WaitingSide 
        };
        Step step_ = Step::WaitingDistance;
        double distance_ = 0.0;
        Entity* selectedEntity_ = nullptr;
        Point2D firstPoint_;
        
        bool hasDistance_ = false;
        bool finished_ = false;
        std::string statusMessage_;

        void createOffsetEntity(Entity* entity, const Point2D& sidePoint, Engine& engine);
    };

} // namespace cad