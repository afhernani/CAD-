#pragma once
#include "../command.hpp"

namespace cad {

class ScaleCommand : public ICommand {
public:
    ScaleCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "ESCALAR"; }

    // >>> Para feedback visual
    Point2D getBasePoint() const { return basePoint_; }
    bool hasBasePoint() const { return hasBasePoint_; }

private:
    enum class Step { BasePoint, Factor };
    Step step_ = Step::BasePoint;
    Point2D basePoint_;
    bool hasBasePoint_ = false;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad