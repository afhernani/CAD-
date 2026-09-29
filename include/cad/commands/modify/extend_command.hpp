#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include <vector>
#include <algorithm>

namespace cad {

class ExtendCommand : public ICommand {
public:
    ExtendCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "ALARGAR"; }

    // Para feedback visual
    const std::vector<Entity*>& getBoundaries() const { return boundaries_; }
    bool isSelectingBoundaries() const { return selectingBoundaries_; }

private:
    enum class Step { 
        SelectingBoundaries, 
        SelectingEntitiesToExtend 
    };
    Step step_ = Step::SelectingBoundaries;
    std::vector<Entity*> boundaries_;
    bool selectingBoundaries_ = true;
    bool finished_ = false;
    std::string statusMessage_;

    void extendEntity(Entity* entity, const Point2D& clickPoint, Engine& engine);
};

} // namespace cad