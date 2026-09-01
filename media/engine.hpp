#pragma once
#include "document.hpp"
#include <string>
#include <optional>
#include <string_view>
#include <vector>
#include <iomanip>

namespace cad {

enum class Mode {
    IDLE,
    DRAW_LINE,
    DRAW_CIRCLE,
    DRAW_ARC,
    DRAW_POLYLINE,
    DRAW_POLYGON,
    LAYER_COMMAND,
    MOVE,
    COPY,
    ROTATE,
    SCALE,
    MIRROR,
    MEASURE_DIST,
    TRIM,
    EXTEND
};

class Engine {
public:
    Document doc;
    Mode currentMode = Mode::IDLE;
    std::string statusMessage = "Listo";
    double viewScale = 1.0;

    Point2D tempPoint1;
    Point2D tempPoint2;
    Point2D lastPoint;

    std::vector<Point2D> tempPolylinePoints;
    Point2D tempPolygonCenter;
    int tempPolygonSides = 0;
    double tempArcRadius = 0.0;
    double tempArcStartAngle = 0.0;

    std::vector<Entity*> selectedEntities;
    void clearSelection();
    void selectEntity(const Point2D& clickPoint, double tolerance);
    void deleteSelected();

    Point2D moveBasePoint;
    Point2D copyBasePoint;
    Point2D rotateCenter;
    double rotateAngle = 0.0;
    Point2D scaleBasePoint;
    double scaleFactor = 1.0;
    Point2D mirrorAxisP1;

    std::vector<Entity*> trimBoundaries;
    std::vector<Entity*> extendBoundaries;
    bool trimSelectingBoundaries = true;
    bool extendSelectingBoundaries = true;

    void processInput(std::string_view input);
    void cancelCommand();
    std::string getHelpForTopic(std::string_view topic) {
        return getHelpText(topic);
    }

private:
    void executeCommand(std::string_view cmd);
    void processCoordinate(std::string_view coordStr);
    void processLayerCommand(std::string_view input);
    bool isNumericValue(std::string_view str) const;
    std::string getHelpText(std::string_view topic);
    [[nodiscard]] std::optional<Point2D> parseCoordinate(std::string_view str);
};

} // namespace cad