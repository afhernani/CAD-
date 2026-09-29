#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include <vector>
#include <algorithm>

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

        // Getters para feedback visual y app.cpp
        enum class Step { SelectingWindowP1, SelectingWindowP2, WaitingBasePoint, WaitingDestPoint };
        Step getStep() const { return step_; }
        Point2D getWindowP1() const { return windowP1_; }
        Point2D getWindowP2() const { return windowP2_; }
        bool hasWindowP1() const { return hasWindowP1_; }
        bool hasWindow() const { return hasWindow_; }
        const std::vector<Entity*>& getSelectedEntities() const { return selectedEntities_; }

        // Métodos para que app.cpp gestione la ventana
        void setWindowP1(const Point2D& p) { windowP1_ = p; hasWindowP1_ = true; }
        void setWindowP2(const Point2D& p) { windowP2_ = p; hasWindow_ = true; }
        void setSelectedEntities(const std::vector<Entity*>& entities) { selectedEntities_ = entities; }
        void advanceToBasePoint() { step_ = Step::WaitingBasePoint; }
        void resetWindow() { windowP1_ = {0,0}; windowP2_ = {0,0}; hasWindowP1_ = false; hasWindow_ = false; }

    private:
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

        void executeStretch(Engine& engine);
    };

} // namespace cad