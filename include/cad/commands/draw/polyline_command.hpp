#pragma once
#include "../command.hpp"
#include "../../core/geometry/entities/polyline.hpp"

namespace cad {

class PolylineCommand : public ICommand {
public:
    PolylineCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "POLILINEA"; }

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