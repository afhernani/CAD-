#include "cad/commands/modify/measure_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>
#include <cmath>
#include <iomanip>
#include <numbers>

namespace cad {

    namespace{
        constexpr double PI = 3.14159265358979323846;
    }

    MeasureCommand::MeasureCommand() {
        statusMessage_ = "DIST | Especificar primer punto:";
    }

    void MeasureCommand::execute(const std::string& input, Engine& engine) {
        // Intentar parsear como coordenada "x,y"
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        statusMessage_ = "DIST | Formato no válido. Usa coordenadas x,y.";
    }

    void MeasureCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingFirstPoint) {
            firstPoint_ = point;
            hasFirstPoint_ = true;
            step_ = Step::WaitingSecondPoint;
            statusMessage_ = "DIST | Especificar segundo punto:";
        }
        else if (step_ == Step::WaitingSecondPoint) {
            double dx = point.x - firstPoint_.x;
            double dy = point.y - firstPoint_.y;
            double dist = std::sqrt(dx * dx + dy * dy);
            double angle = std::atan2(dy, dx) * 180.0 / PI;
            
            std::ostringstream ossDist;
            ossDist << std::fixed << std::setprecision(2);
            ossDist << "Distancia: " << dist
                    << ", Angulo XY: " << angle << " deg"
                    << ", DX: " << dx
                    << ", DY: " << dy;
            
            statusMessage_ = ossDist.str();
            finished_ = true;
        }
    }

    void MeasureCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string MeasureCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool MeasureCommand::isComplete() const {
        return finished_;
    }

} // namespace cad