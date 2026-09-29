#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include "../../core/document/block.hpp" // Necesario para BlockDefinition

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

    // Hacer Step público para que app.cpp pueda consultarlo
    enum class Step { WaitingName, WaitingInsertPoint };
    Step getStep() const { return step_; }

private:
    Step step_ = Step::WaitingName;
    BlockDefinition* definition_ = nullptr;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad