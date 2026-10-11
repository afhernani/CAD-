#pragma once
#include "../command.hpp"

namespace cad {

class ImageCommand : public ICommand {
public:
    ImageCommand(const std::string& preselectedPath = "");

    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "IMAGEN"; }
    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

private:
    std::string imagePath_;
    double aspectRatio_ = 1.0;
    bool hasPath_ = false;
    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad