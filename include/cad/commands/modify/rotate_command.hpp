#pragma once
#include "../command.hpp"

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

    class RotateCommand : public ICommand {
    public:
        RotateCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "ROTAR"; }

        // >>> NUEVA FIRMA CON Engine& <<<
        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                        const Point2D& mouseWorldPos, sf::Font& font) const override;

        // >>> Para feedback visual
        Point2D getCenter() const { return basePoint_; }
        bool hasCenter() const { return hasCenter_; }

    private:
        enum class Step { BasePoint, Angle };
        Step step_ = Step::BasePoint;
        Point2D basePoint_;
        bool hasCenter_ = false;
        bool finished_ = false;
        std::string statusMessage_;
    };

} // namespace cad