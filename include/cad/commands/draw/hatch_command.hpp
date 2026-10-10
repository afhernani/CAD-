#pragma once
#include "../command.hpp"
#include "cad/core/geometry/entities/hatch.hpp"
#include <vector>

namespace cad {

    class HatchCommand : public ICommand {
    public:
        enum class Step {
            SelectingOuter,
            SelectingIslands,
            DefiningAngle,
            DefiningSpacing,
            Finished
        };

        HatchCommand();

        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                        const Point2D& mouseWorldPos, sf::Font& font) const override;

        // ✅ NUEVO: Implementación obligatoria de getName()
        std::string getName() const override { return "HATCH"; }

    private:
        std::vector<Point2D> extractPointsFromEntity(Entity* entity) const;

        Step step_ = Step::SelectingOuter;
        std::vector<Entity*> selectedEntities_; // Primera = exterior, resto = islas
        std::vector<Point2D> hatchPoints_;
        double angle_ = 0.0;
        double spacing_ = 1.0;
        std::string statusMessage_;
        bool finished_ = false; // ✅ NUEVO: Variable de estado del comando
    };

} // namespace cad