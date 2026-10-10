#pragma once
#include "../command.hpp"
#include "cad/core/geometry/point.hpp"
#include <vector>

namespace cad {

class TriangleCommand : public ICommand {
public:
    enum class Step {
        WaitingP1,
        WaitingP2,
        WaitingP3,
        Finished
    };

    TriangleCommand();

    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "TRIANGULO"; }
    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

private:
    Step step_ = Step::WaitingP1;
    std::vector<Point2D> points_; // Almacenará los 3 puntos
    std::string statusMessage_;
    bool finished_ = false;
};

} // namespace cad