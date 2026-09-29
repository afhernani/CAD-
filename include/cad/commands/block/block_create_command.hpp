#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include "../../core/document/block.hpp" // Asegúrate de que esté este include
#include <vector>

namespace cad {

class BlockCreateCommand : public ICommand {
public:
    BlockCreateCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "BLOQUE"; }

    enum class Step { WaitingName, WaitingBasePoint, SelectingEntities };
    Step getStep() const { return step_; }
    
    const std::string& getBlockName() const { return blockName_; }
    Point2D getBasePoint() const { return basePoint_; }
    const std::vector<Entity*>& getSelectedEntities() const { return selectedEntities_; }

    void addSelectedEntity(Entity* entity) { selectedEntities_.push_back(entity); }
    void setBasePoint(const Point2D& p) { basePoint_ = p; }

private:
    Step step_ = Step::WaitingName;
    std::string blockName_;
    Point2D basePoint_;
    std::vector<Entity*> selectedEntities_;
    bool finished_ = false;
    std::string statusMessage_;

    void executeCreate(Engine& engine);
};

} // namespace cad