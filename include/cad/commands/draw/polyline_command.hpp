#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/polyline.hpp"

// Forward declarations para evitar incluir SFML aquí
namespace sf { class RenderWindow; class Font; }

namespace cad {
    class View;

    class PolylineCommand : public ICommand {
    public:
        PolylineCommand();
        
        void execute(const std::string& input, Engine& engine) override;
        void onPoint(const Point2D& point, Engine& engine) override;
        void onCancel() override;
        std::string getStatusMessage() const override;
        bool isComplete() const override;
        std::string getName() const override { return "POLILINEA"; }

        // >>> NUEVO: Método de feedback visual <<<
        void drawFeedback(sf::RenderWindow& window, const View& view, 
                          const Point2D& mouseWorldPos, sf::Font& font) const override;

        // Para feedback visual
        const std::vector<Point2D>& getPoints() const { return points_; }
        Point2D getLastPoint() const { return points_.empty() ? Point2D{0,0} : points_.back(); }
        bool hasPoints() const { return !points_.empty(); }

    private:
        std::vector<Point2D> points_;
        bool finished_ = false;
        std::string statusMessage_;
    };

} // namespace cad