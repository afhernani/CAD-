// include/cad/commands/draw/line_command.hpp
#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/line.hpp"

namespace cad {

    class LineCommand : public ICommand {
    public:
        enum class State { 
            WaitingFirstPoint,   // Aún no se ha puesto el primer punto
            WaitingSecondPoint,  // Se puso el primer punto, esperando el segundo
            LineCreated          // Se creó una línea, esperando nuevo punto
        };

        LineCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "LINEA"; }

        // >>> NUEVOS MÉTODOS PARA FEEDBACK VISUAL <<<
        State getState() const { return state_; }
        Point2D getStartPoint() const { return p1_; }

    private:
        State state_ = State::WaitingFirstPoint;
        Point2D p1_;
        std::string statusMessage_;
    };

} // namespace cad