#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp" // Cambiado de line.hpp a entity.hpp

namespace sf { class RenderWindow; class Font; }
namespace cad { class View; class Engine; }

namespace cad {

class FilletCommand : public ICommand {
public:
    FilletCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "EMPALME"; }

    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override;

    double getRadius() const { return radius_; }
    bool hasRadius() const { return hasRadius_; }
    Entity* getEntity1() const { return entity1_; }
    Entity* getEntity2() const { return entity2_; }

private:
    enum class Step { WaitingRadius, WaitingEntity1, WaitingEntity2 };
    Step step_ = Step::WaitingRadius;
    
    double radius_ = 0.0;
    Entity* entity1_ = nullptr;
    Entity* entity2_ = nullptr;
    
    bool hasRadius_ = false;
    bool finished_ = false;
    std::string statusMessage_;
    Point2D lastClickPoint_; // Para recordar dónde hizo clic el usuario y decidir el lado del empalme

    void createFillet(Engine& engine, const Point2D& clickPoint);
};

} // namespace cad