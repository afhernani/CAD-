// include/cad/commands/draw/arc_command.hpp
#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/arc.hpp"

namespace cad {

    class ArcCommand : public ICommand {
    public:
        ArcCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "ARCO"; }
        // Para feedback visual
        Point2D getCenter() const { return center_; }
        Point2D getStartPoint() const { return startPoint_; }
        bool hasCenter() const { return hasCenter_; }
        bool hasStartPoint() const { return hasStartPoint_; }
        double getRadius() const { return radius_; }

    private:
        enum class Step { 
            WaitingCenter, 
            WaitingStartPoint, 
            WaitingEndPoint 
        };
        Step step_ = Step::WaitingCenter;
        Point2D center_;
        Point2D startPoint_;
        double radius_ = 0.0;
        double startAngle_ = 0.0;
        
        bool hasCenter_ = false;
        bool hasStartPoint_ = false;
        bool finished_ = false;
        std::string statusMessage_;
    };

} // namespace cad