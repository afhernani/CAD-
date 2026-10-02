#pragma once
#include "../command.hpp"

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }


namespace cad {

    class MeasureCommand : public ICommand {
    public:
        MeasureCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "MEDIR"; }

        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                          const Point2D& mouseWorldPos, sf::Font& font) const override;

        // >>> NUEVOS: Para feedback visual
        Point2D getFirstPoint() const { return firstPoint_; }
        bool hasFirstPoint() const { return hasFirstPoint_; }

    private:
        enum class Step { WaitingFirstPoint, WaitingSecondPoint };
        Step step_ = Step::WaitingFirstPoint;
        Point2D firstPoint_;
        bool hasFirstPoint_ = false;
        bool finished_ = false;
        std::string statusMessage_;
    };

} // namespace cad