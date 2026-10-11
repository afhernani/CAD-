#pragma once
#include "../command.hpp" // Asegúrate de que la ruta sea correcta (command.hpp o icommand.hpp según tu proyecto)
#include "../../core/geometry/entity.hpp"
#include <vector>
#include <memory>

namespace cad {

class ExplodeCommand : public ICommand {
public:
    ExplodeCommand();

    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override { (void)point; (void)engine; } // No usado en pre-selección
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "EXPLODE"; }
    void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                      const Point2D& mouseWorldPos, sf::Font& font) const override { (void)window; (void)view; (void)engine; (void)mouseWorldPos; (void)font; }

private:
    // Función auxiliar para descomponer una entidad en primitivas
    void explodeEntity(Entity* entity, std::vector<std::unique_ptr<Entity>>& newEntities);

    bool finished_ = false;
    std::string statusMessage_;
};

} // namespace cad