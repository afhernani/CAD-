#pragma once
#include "../command.hpp"

// Forward declarations
    namespace sf { class RenderWindow; class Font; }
    namespace cad { class View; class Engine; }

namespace cad {

    class MirrorCommand : public ICommand {
    public:
        MirrorCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "SIMETRIA"; }

        // >>> NUEVA FIRMA CON Engine& <<<
        void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                        const Point2D& mouseWorldPos, sf::Font& font) const override;

        // >>> NUEVOS: Para feedback visual
        Point2D getAxisP1() const { return axisP1_; }
        bool hasAxisP1() const { return hasAxisP1_; }

    private:
        enum class Step { FirstAxisPoint, SecondAxisPoint };
        Step step_ = Step::FirstAxisPoint;
        Point2D axisP1_;
        bool hasAxisP1_ = false;
        bool finished_ = false;
        std::string statusMessage_;
    };

} // namespace cad