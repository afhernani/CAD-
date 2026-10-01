#include "cad/render/renderer.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/ellipse.hpp"
#include "cad/core/geometry/entities/dimension.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/commands/draw/line_command.hpp"
#include "cad/commands/draw/circle_command.hpp"
#include "cad/commands/draw/arc_command.hpp"
#include "cad/commands/draw/polyline_command.hpp"
#include "cad/commands/draw/polygon_command.hpp"
#include "cad/commands/draw/ellipse_command.hpp"
#include "cad/commands/modify/move_command.hpp"
#include "cad/commands/modify/copy_command.hpp"
#include "cad/commands/modify/rotate_command.hpp"
#include "cad/commands/modify/scale_command.hpp"
#include "cad/commands/modify/mirror_command.hpp"
#include "cad/commands/modify/offset_command.hpp"
#include "cad/commands/modify/fillet_command.hpp"
#include "cad/commands/modify/chamfer_command.hpp"
#include "cad/commands/modify/trim_command.hpp"
#include "cad/commands/modify/extend_command.hpp"
#include "cad/commands/modify/measure_command.hpp"
#include "cad/commands/modify/array_command.hpp"
#include "cad/commands/modify/stretch_command.hpp"
#include "cad/commands/block/block_create_command.hpp"
#include "cad/commands/block/block_insert_command.hpp"

#include <sstream>
#include <cmath>
#include <iomanip>

namespace cad {

// Helper local para convertir UTF-8 a sf::String
namespace {
    sf::String toSfString(const std::string& utf8Str) {
        return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
    }
}

void Renderer::render(sf::RenderWindow& window, const View& view, Engine& engine, 
                      const sf::Vector2i& mouseScreenPos, const Point2D& mouseWorldPos,
                      sf::Font& font, bool isSnapped, const Point2D& snappedPoint, bool showAxes) const {
    drawGrid(window, view, engine);
    if (showAxes) drawAxes(window, view);
    drawEntities(window, view, engine);
    drawGrips(window, view, engine);
    drawDimensionTexts(window, view, engine, font);
    drawCrosshair(window, view, mouseScreenPos, mouseWorldPos, isSnapped, snappedPoint);
    // drawDrawingFeedback(window, view, engine, mouseWorldPos, font);
    // El renderer no sabe qué comando es, solo le pide que se dibuje a sí mismo.
    if (engine.activeCommand_) {
        engine.activeCommand_->drawFeedback(window, view, mouseWorldPos, font);
    }
}

void Renderer::drawGrid(sf::RenderWindow& window, const View& view, const Engine& engine) const {
    if (!engine.gridEnabled) return;
    
    double currentGridSize = 10.0;
    double pixelSpacing = currentGridSize * view.getScale();
    
    while (pixelSpacing < 30.0) { currentGridSize *= 10.0; pixelSpacing = currentGridSize * view.getScale(); }
    while (pixelSpacing > 150.0) { currentGridSize /= 10.0; pixelSpacing = currentGridSize * view.getScale(); }

    Point2D tl = view.screenToWorld(0, 0);
    Point2D br = view.screenToWorld(static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT));
    
    double left = std::min(tl.x, br.x), right = std::max(tl.x, br.x);
    double top = std::max(tl.y, br.y), bottom = std::min(tl.y, br.y);

    double startX = std::floor(left / currentGridSize) * currentGridSize;
    double startY = std::floor(top / currentGridSize) * currentGridSize;

    sf::Color gridColor(50, 50, 50);
    sf::VertexArray lines(sf::Lines);

    for (double x = startX; x <= right; x += currentGridSize) {
        sf::Vector2f topPos = view.worldToScreen(x, top);
        sf::Vector2f botPos = view.worldToScreen(x, bottom);
        lines.append(sf::Vertex(topPos, gridColor));
        lines.append(sf::Vertex(botPos, gridColor));
    }
    for (double y = startY; y <= bottom; y += currentGridSize) {
        sf::Vector2f leftPos = view.worldToScreen(left, y);
        sf::Vector2f rightPos = view.worldToScreen(right, y);
        lines.append(sf::Vertex(leftPos, gridColor));
        lines.append(sf::Vertex(rightPos, gridColor));
    }
    window.draw(lines);
}

void Renderer::drawAxes(sf::RenderWindow& window, const View& view) const {
    sf::Vertex xAxis[] = {
        sf::Vertex(view.worldToScreen(0, 0), sf::Color::Red),
        sf::Vertex(view.worldToScreen(50, 0), sf::Color::Red)
    };
    window.draw(xAxis, 2, sf::Lines);

    sf::Vertex yAxis[] = {
        sf::Vertex(view.worldToScreen(0, 0), sf::Color::Green),
        sf::Vertex(view.worldToScreen(0, 50), sf::Color::Green)
    };
    window.draw(yAxis, 2, sf::Lines);

    sf::CircleShape origin(3.f);
    origin.setFillColor(sf::Color::White);
    origin.setOrigin(3.f, 3.f);
    origin.setPosition(view.worldToScreen(0, 0));
    window.draw(origin);
}

void Renderer::drawEntities(sf::RenderWindow& window, const View& view, const Engine& engine) const {
    for (const auto& entity : engine.doc.entities) {
        const Layer* layer = engine.doc.getLayer(entity->layerName);
        if (!layer || !layer->visible || layer->frozen) continue;
        
        sf::Color drawColor = layer->color;
        
        // Lambda para transformar coordenadas usando View
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };
        entity->draw(window, w2s, drawColor, view.getScale());
    }
}

void Renderer::drawGrips(sf::RenderWindow& window, const View& view, const Engine& engine) const {
    const float gripSize = 6.0f;
    sf::Color gripColor(0, 100, 255);
    sf::Color activeGripColor(255, 0, 0);

    for (Entity* entity : engine.selectedEntities) {
        auto grips = entity->getGripPoints();
        for (int i = 0; i < grips.size(); ++i) {
            sf::Vector2f screenPos = view.worldToScreen(grips[i].x, grips[i].y);
            bool isActive = (engine.currentMode == Mode::GRIP_EDIT &&
                             engine.activeGripEntity == entity &&
                             engine.activeGripIndex == i);
            float size = isActive ? gripSize * 1.5f : gripSize;
            sf::Color color = isActive ? activeGripColor : gripColor;

            sf::RectangleShape grip(sf::Vector2f(size, size));
            grip.setFillColor(color);
            grip.setOrigin(size / 2.f, size / 2.f);
            grip.setPosition(screenPos);
            window.draw(grip);
        }
    }
}

void Renderer::drawDimensionTexts(sf::RenderWindow& window, const View& view, const Engine& engine, sf::Font& font) const {
    const double PI = 3.14159265358979323846;
    for (const auto& entity : engine.doc.entities) {
        if (auto* dim = dynamic_cast<Dimension*>(entity.get())) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << dim->value;
            double textX, textY;
            float rotation = 0.0f;

            if (dim->type == DimType::ALIGNED || dim->isAligned) {
                double dx = dim->p2.x - dim->p1.x;
                double dy = dim->p2.y - dim->p1.y;
                double len = std::sqrt(dx*dx + dy*dy);
                if (len == 0) continue;
                double nx = -dy / len; double ny = dx / len;
                double vx = dim->location.x - dim->p1.x; double vy = dim->location.y - dim->p1.y;
                double offset = vx * nx + vy * ny;
                textX = (dim->p1.x + dim->p2.x) / 2.0 + nx * offset;
                textY = (dim->p1.y + dim->p2.y) / 2.0 + ny * offset;
                rotation = std::atan2(dy, dx) * 180.0f / (float)PI;
                if (rotation > 90.0f && rotation < 270.0f) rotation -= 180.0f;
            }
            else if (dim->type == DimType::ANGULAR) {
                double arcRadius = std::hypot(dim->p3.x - dim->location.x, dim->p3.y - dim->location.y);
                if (arcRadius < 1.0) arcRadius = 10.0;
                double angle1 = std::atan2(dim->p1.y - dim->location.y, dim->p1.x - dim->location.x);
                double angle2 = std::atan2(dim->p2.y - dim->location.y, dim->p2.x - dim->location.x);
                double diff = angle2 - angle1;
                while (diff < 0) diff += 2 * PI;
                while (diff >= 2 * PI) diff -= 2 * PI;
                double midAngle = angle1 + diff / 2.0;
                double textRadius = arcRadius * 1.3;
                textX = dim->location.x + textRadius * std::cos(midAngle);
                textY = dim->location.y + textRadius * std::sin(midAngle);
            }
            else if (dim->type == DimType::RADIUS || dim->type == DimType::DIAMETER) {
                textX = (dim->p1.x + dim->p2.x) / 2.0;
                textY = (dim->p1.y + dim->p2.y) / 2.0;
            }
            else {
                textX = dim->isHorizontal ? (dim->p1.x + dim->p2.x) / 2.0 : dim->location.x;
                textY = dim->isHorizontal ? dim->location.y : (dim->p1.y + dim->p2.y) / 2.0;
            }

            sf::Vector2f screenPos = view.worldToScreen(textX, textY);
            sf::Text text;
            text.setFont(font);
            text.setString(toSfString(oss.str()));
            text.setCharacterSize(12);
            text.setFillColor(sf::Color::White);
            text.setRotation(rotation);

            sf::FloatRect bounds = text.getLocalBounds();
            float padding = 4.0f;
            sf::RectangleShape bg(sf::Vector2f(bounds.width + padding * 2, bounds.height + padding));
            bg.setFillColor(sf::Color(30, 30, 30));
            bg.setOrigin(bounds.width / 2.f + padding, bounds.height / 2.f);
            bg.setPosition(screenPos);
            bg.setRotation(rotation);
            window.draw(bg);

            text.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
            text.setPosition(screenPos);
            window.draw(text);
        }
    }
}

void Renderer::drawCrosshair(sf::RenderWindow& window, const View& view, const sf::Vector2i& mouseScreenPos, 
                             const Point2D& mouseWorldPos, bool isSnapped, const Point2D& snappedPoint) const {
    bool isInCanvas = (mouseScreenPos.x >= 0 && mouseScreenPos.x < static_cast<int>(WINDOW_WIDTH) &&
                       mouseScreenPos.y >= static_cast<int>(MENU_HEIGHT + TOOLBAR_HEIGHT) &&
                       mouseScreenPos.y < static_cast<int>(WINDOW_HEIGHT - COMMAND_HEIGHT - STATUS_HEIGHT));
    if (!isInCanvas) return;

    sf::Vector2f screenPos = { static_cast<float>(mouseScreenPos.x), static_cast<float>(mouseScreenPos.y) };
    sf::Color crosshairColor = isSnapped ? sf::Color(255, 255, 0) : sf::Color(255, 255, 255);
    
    if (isSnapped) screenPos = view.worldToScreen(snappedPoint.x, snappedPoint.y);

    float crosshairSize = 25.0f;
    float thickness = 2.0f;

    auto drawThickLine = [&](sf::Vector2f start, sf::Vector2f end, sf::Color color) {
        sf::Vector2f dir = end - start;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        sf::Vector2f normal = {-dir.y / len, dir.x / len};
        sf::Vertex quad[] = {
            sf::Vertex(start + normal * thickness / 2.f, color),
            sf::Vertex(start - normal * thickness / 2.f, color),
            sf::Vertex(end - normal * thickness / 2.f, color),
            sf::Vertex(end + normal * thickness / 2.f, color)
        };
        window.draw(quad, 4, sf::Quads);
    };

    drawThickLine({screenPos.x - crosshairSize, screenPos.y}, {screenPos.x - 8.f, screenPos.y}, crosshairColor);
    drawThickLine({screenPos.x + 8.f, screenPos.y}, {screenPos.x + crosshairSize, screenPos.y}, crosshairColor);
    drawThickLine({screenPos.x, screenPos.y - crosshairSize}, {screenPos.x, screenPos.y - 8.f}, crosshairColor);
    drawThickLine({screenPos.x, screenPos.y + 8.f}, {screenPos.x, screenPos.y + crosshairSize}, crosshairColor);

    sf::CircleShape centerPoint(3.f);
    centerPoint.setFillColor(crosshairColor);
    centerPoint.setOrigin(3.f, 3.f);
    centerPoint.setPosition(screenPos);
    window.draw(centerPoint);

    if (isSnapped) {
        sf::CircleShape snapCircle(8.f);
        snapCircle.setFillColor(sf::Color::Transparent);
        snapCircle.setOutlineColor(sf::Color::Yellow);
        snapCircle.setOutlineThickness(2.f);
        snapCircle.setOrigin(8.f, 8.f);
        snapCircle.setPosition(screenPos);
        window.draw(snapCircle);
    }
}

void Renderer::drawDrawingFeedback(sf::RenderWindow& window, const View& view, Engine& engine, 
                                   const Point2D& mouseWorldPos, sf::Font& font) const {
    sf::Color feedbackColor(255, 255, 0, 180);
    auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

    // --- LÍNEA ---
    if (engine.currentMode == Mode::DRAW_LINE && engine.activeCommand_) {
        if (auto* lineCmd = dynamic_cast<LineCommand*>(engine.activeCommand_.get())) {
            if (lineCmd->getState() == LineCommand::State::WaitingSecondPoint) {
                Point2D startPoint = lineCmd->getStartPoint();
                sf::Vertex line[] = { sf::Vertex(w2s(startPoint.x, startPoint.y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(line, 2, sf::Lines);
            }
        }
    }
    // --- CÍRCULO ---
    else if (engine.currentMode == Mode::DRAW_CIRCLE && engine.activeCommand_) {
        if (auto* circleCmd = dynamic_cast<CircleCommand*>(engine.activeCommand_.get())) {
            if (circleCmd->hasCenter() && (circleCmd->isWaitingForPoint() || engine.statusMessage.find("Radio/Diámetro") != std::string::npos)) {
                double dx = mouseWorldPos.x - circleCmd->getCenter().x;
                double dy = mouseWorldPos.y - circleCmd->getCenter().y;
                double radius = std::sqrt(dx * dx + dy * dy);
                const int numPoints = 64;
                sf::VertexArray va(sf::LineStrip, numPoints + 1);
                const double PI = 3.14159265358979323846;
                double angleStep = 2.0 * PI / numPoints;
                for (int i = 0; i <= numPoints; ++i) {
                    double angle = i * angleStep;
                    va[i].position = w2s(circleCmd->getCenter().x + radius * std::cos(angle), circleCmd->getCenter().y + radius * std::sin(angle));
                    va[i].color = feedbackColor;
                }
                window.draw(va);
            }
        }
    }
    // --- ARCO ---
    else if (engine.currentMode == Mode::DRAW_ARC && engine.activeCommand_) {
        if (auto* arcCmd = dynamic_cast<ArcCommand*>(engine.activeCommand_.get())) {
            if (arcCmd->hasCenter() && !arcCmd->hasStartPoint()) {
                sf::Vertex line[] = { sf::Vertex(w2s(arcCmd->getCenter().x, arcCmd->getCenter().y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(line, 2, sf::Lines);
            }
            else if (arcCmd->hasCenter() && arcCmd->hasStartPoint()) {
                double dx = mouseWorldPos.x - arcCmd->getCenter().x;
                double dy = mouseWorldPos.y - arcCmd->getCenter().y;
                double endAngle = std::atan2(dy, dx);
                double startDx = arcCmd->getStartPoint().x - arcCmd->getCenter().x;
                double startDy = arcCmd->getStartPoint().y - arcCmd->getCenter().y;
                double startRadAngle = std::atan2(startDy, startDx);
                const int numPoints = 64;
                sf::VertexArray va(sf::LineStrip, numPoints);
                const double PI = 3.14159265358979323846;
                double diff = endAngle - startRadAngle;
                while (diff < 0) diff += 2 * PI; while (diff >= 2 * PI) diff -= 2 * PI;
                double step = diff / (numPoints - 1);
                for (int i = 0; i < numPoints; ++i) {
                    double angle = startRadAngle + i * step;
                    va[i].position = w2s(arcCmd->getCenter().x + arcCmd->getRadius() * std::cos(angle), arcCmd->getCenter().y + arcCmd->getRadius() * std::sin(angle));
                    va[i].color = feedbackColor;
                }
                window.draw(va);
                sf::Vertex guideLine[] = { sf::Vertex(w2s(arcCmd->getCenter().x, arcCmd->getCenter().y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(guideLine, 2, sf::Lines);
            }
        }
    }
    // --- POLILÍNEA ---
    else if (engine.currentMode == Mode::DRAW_POLYLINE && engine.activeCommand_) {
        if (auto* polyCmd = dynamic_cast<PolylineCommand*>(engine.activeCommand_.get())) {
            if (polyCmd->hasPoints()) {
                const auto& points = polyCmd->getPoints();
                if (points.size() >= 2) {
                    sf::VertexArray segments(sf::LineStrip, points.size());
                    for (size_t i = 0; i < points.size(); ++i) { segments[i].position = w2s(points[i].x, points[i].y); segments[i].color = feedbackColor; }
                    window.draw(segments);
                }
                sf::Vertex line[] = { sf::Vertex(w2s(polyCmd->getLastPoint().x, polyCmd->getLastPoint().y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(line, 2, sf::Lines);
            }
        }
    }
    // --- POLÍGONO ---
    else if (engine.currentMode == Mode::DRAW_POLYGON && engine.activeCommand_) {
        if (auto* polyCmd = dynamic_cast<PolygonCommand*>(engine.activeCommand_.get())) {
            if (polyCmd->hasCenter() && !polyCmd->hasSides()) {
                sf::CircleShape dot(4.0f); dot.setFillColor(feedbackColor); dot.setOrigin(4.0f, 4.0f); dot.setPosition(w2s(polyCmd->getCenter().x, polyCmd->getCenter().y)); window.draw(dot);
            }
            else if (polyCmd->hasCenter() && polyCmd->hasSides()) {
                double dx = mouseWorldPos.x - polyCmd->getCenter().x; double dy = mouseWorldPos.y - polyCmd->getCenter().y;
                double radius = std::sqrt(dx * dx + dy * dy); int sides = polyCmd->getSides();
                const double PI = 3.14159265358979323846; double angleStep = 2.0 * PI / sides;
                sf::VertexArray va(sf::LineStrip, sides + 1);
                for (int i = 0; i <= sides; ++i) { double angle = i * angleStep - PI / 2.0; va[i].position = w2s(polyCmd->getCenter().x + radius * std::cos(angle), polyCmd->getCenter().y + radius * std::sin(angle)); va[i].color = feedbackColor; }
                window.draw(va);
                sf::Vertex line[] = { sf::Vertex(w2s(polyCmd->getCenter().x, polyCmd->getCenter().y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(line, 2, sf::Lines);
            }
        }
    }
    // --- ELIPSE ---
    else if (engine.currentMode == Mode::DRAW_ELLIPSE && engine.activeCommand_) {
        if (auto* ellipseCmd = dynamic_cast<EllipseCommand*>(engine.activeCommand_.get())) {
            if (ellipseCmd->hasCenter() && !ellipseCmd->hasMajorAxis()) {
                sf::Vertex line[] = { sf::Vertex(w2s(ellipseCmd->getCenter().x, ellipseCmd->getCenter().y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(line, 2, sf::Lines);
            }
            else if (ellipseCmd->hasCenter() && ellipseCmd->hasMajorAxis()) {
                double dx = mouseWorldPos.x - ellipseCmd->getCenter().x; double dy = mouseWorldPos.y - ellipseCmd->getCenter().y;
                double minorRadius = std::sqrt(dx * dx + dy * dy); const int numPoints = 64;
                sf::VertexArray va(sf::LineStrip, numPoints + 1); const double PI = 3.14159265358979323846; double angleStep = 2.0 * PI / numPoints;
                double rotationAngle = ellipseCmd->getRotationAngle();
                for (int i = 0; i <= numPoints; ++i) { double angle = i * angleStep; double rotatedAngle = angle + rotationAngle; va[i].position = w2s(ellipseCmd->getCenter().x + ellipseCmd->getMajorRadius() * std::cos(rotatedAngle), ellipseCmd->getCenter().y + minorRadius * std::sin(rotatedAngle)); va[i].color = feedbackColor; }
                window.draw(va);
                sf::Vertex guideLine[] = { sf::Vertex(w2s(ellipseCmd->getCenter().x, ellipseCmd->getCenter().y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
                window.draw(guideLine, 2, sf::Lines);
            }
        }
    }
    // --- COTA (DIMENSION) ---
    else if (engine.currentMode == Mode::DRAW_DIMENSION) {
        if (engine.statusMessage.find("Segundo") != std::string::npos) {
            sf::Vertex line[] = { sf::Vertex(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
            window.draw(line, 2, sf::Lines);
        }
        else if (engine.statusMessage.find("Ubicación") != std::string::npos || engine.statusMessage.find("ubicación") != std::string::npos) {
            double cx = (engine.tempDimP1.x + engine.tempDimP2.x) / 2.0; double cy = (engine.tempDimP1.y + engine.tempDimP2.y) / 2.0;
            bool isHoriz = std::abs(mouseWorldPos.y - cy) > std::abs(mouseWorldPos.x - cx);
            Point2D ext1, ext2, lineStart, lineEnd;
            if (isHoriz) { double y = mouseWorldPos.y; ext1 = {engine.tempDimP1.x, y}; ext2 = {engine.tempDimP2.x, y}; lineStart = {engine.tempDimP1.x, y}; lineEnd = {engine.tempDimP2.x, y}; } 
            else { double x = mouseWorldPos.x; ext1 = {x, engine.tempDimP1.y}; ext2 = {x, engine.tempDimP2.y}; lineStart = {x, engine.tempDimP1.y}; lineEnd = {x, engine.tempDimP2.y}; }
            sf::Color extColor(255, 255, 0, 100);
            sf::Vertex extLine1[] = { sf::Vertex(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y), extColor), sf::Vertex(view.worldToScreen(ext1.x, ext1.y), extColor) };
            sf::Vertex extLine2[] = { sf::Vertex(view.worldToScreen(engine.tempDimP2.x, engine.tempDimP2.y), extColor), sf::Vertex(view.worldToScreen(ext2.x, ext2.y), extColor) };
            window.draw(extLine1, 2, sf::Lines); window.draw(extLine2, 2, sf::Lines);
            sf::Vertex dimLine[] = { sf::Vertex(view.worldToScreen(lineStart.x, lineStart.y), feedbackColor), sf::Vertex(view.worldToScreen(lineEnd.x, lineEnd.y), feedbackColor) };
            window.draw(dimLine, 2, sf::Lines);
        }
    }
    // --- COTA ALINEADA ---
    else if (engine.currentMode == Mode::DRAW_DIM_ALIGNED) {
        if (engine.statusMessage.find("Segundo") != std::string::npos) {
            sf::Vertex line[] = { sf::Vertex(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
            window.draw(line, 2, sf::Lines);
        }
        else if (engine.statusMessage.find("Ubicación") != std::string::npos) {
            double dx = engine.tempDimP2.x - engine.tempDimP1.x; double dy = engine.tempDimP2.y - engine.tempDimP1.y; double len = std::sqrt(dx*dx + dy*dy);
            if (len > 0) {
                double nx = -dy / len; double ny = dx / len; double vx = mouseWorldPos.x - engine.tempDimP1.x; double vy = mouseWorldPos.y - engine.tempDimP1.y; double offset = vx * nx + vy * ny;
                sf::Vector2f sPos = view.worldToScreen(engine.tempDimP1.x + nx * offset, engine.tempDimP1.y + ny * offset);
                sf::Vector2f ePos = view.worldToScreen(engine.tempDimP2.x + nx * offset, engine.tempDimP2.y + ny * offset);
                sf::Vertex line[] = { sf::Vertex(sPos, feedbackColor), sf::Vertex(ePos, feedbackColor) };
                window.draw(line, 2, sf::Lines);
            }
        }
    }
    // --- COTA RADIO ---
    else if (engine.currentMode == Mode::DRAW_DIM_RADIUS) {
        if (engine.statusMessage.find("Selecciona") != std::string::npos || engine.statusMessage.find("Centro") != std::string::npos) {
            sf::CircleShape dot(4.0f); dot.setFillColor(feedbackColor); dot.setOrigin(4.0f, 4.0f); dot.setPosition(w2s(mouseWorldPos.x, mouseWorldPos.y)); window.draw(dot);
        }
        else {
            double radius = std::sqrt(std::pow(mouseWorldPos.x - engine.tempDimP1.x, 2) + std::pow(mouseWorldPos.y - engine.tempDimP1.y, 2));
            sf::CircleShape circle(static_cast<float>(radius * view.getScale())); circle.setFillColor(sf::Color::Transparent); circle.setOutlineColor(feedbackColor); circle.setOutlineThickness(1.5f);
            circle.setOrigin(static_cast<float>(radius * view.getScale()), static_cast<float>(radius * view.getScale())); circle.setPosition(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y)); window.draw(circle);
            sf::Vertex line[] = { sf::Vertex(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y), feedbackColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), feedbackColor) };
            window.draw(line, 2, sf::Lines);
            std::ostringstream oss; oss << std::fixed << std::setprecision(2) << "R=" << radius;
            sf::Text txt; txt.setFont(font); txt.setString(toSfString(oss.str())); txt.setCharacterSize(14); txt.setFillColor(sf::Color::Yellow);
            sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y); txt.setPosition(mouseScreen.x + 10.f, mouseScreen.y - 20.f); window.draw(txt);
        }
    }
    // --- COTA DIÁMETRO ---
    else if (engine.currentMode == Mode::DRAW_DIM_DIAMETER) {
        if (engine.statusMessage.find("Selecciona") != std::string::npos) {
            sf::CircleShape indicator(5.0f); indicator.setFillColor(feedbackColor); indicator.setOrigin(5.0f, 5.0f); indicator.setPosition(w2s(mouseWorldPos.x, mouseWorldPos.y)); window.draw(indicator);
        }
        else {
            double radius = std::sqrt(std::pow(mouseWorldPos.x - engine.tempDimP1.x, 2) + std::pow(mouseWorldPos.y - engine.tempDimP1.y, 2));
            sf::CircleShape circle(static_cast<float>(radius * view.getScale())); circle.setFillColor(sf::Color::Transparent); circle.setOutlineColor(feedbackColor); circle.setOutlineThickness(1.5f);
            circle.setOrigin(static_cast<float>(radius * view.getScale()), static_cast<float>(radius * view.getScale())); circle.setPosition(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y)); window.draw(circle);
            double dx = mouseWorldPos.x - engine.tempDimP1.x; double dy = mouseWorldPos.y - engine.tempDimP1.y; double len = std::hypot(dx, dy);
            double nx = (len > 0) ? (dx / len) : 1.0; double ny = (len > 0) ? (dy / len) : 0.0;
            sf::Vertex line[] = { sf::Vertex(view.worldToScreen(engine.tempDimP1.x - nx * radius, engine.tempDimP1.y - ny * radius), feedbackColor), sf::Vertex(view.worldToScreen(engine.tempDimP1.x + nx * radius, engine.tempDimP1.y + ny * radius), feedbackColor) };
            window.draw(line, 2, sf::Lines);
            std::ostringstream oss; oss << std::fixed << std::setprecision(2) << "Ø=" << (radius * 2.0);
            sf::Text txt; txt.setFont(font); txt.setString(toSfString(oss.str())); txt.setCharacterSize(14); txt.setFillColor(sf::Color::Yellow);
            sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y); txt.setPosition(mouseScreen.x + 10.f, mouseScreen.y - 20.f); window.draw(txt);
        }
    }
    // --- COTA ANGULAR ---
    else if (engine.currentMode == Mode::DRAW_DIM_ANGULAR) {
        const double PI = 3.14159265358979323846;
        if (engine.statusMessage.find("segunda") != std::string::npos) {
            sf::Vertex line1[] = { sf::Vertex(view.worldToScreen(engine.tempDimP1.x, engine.tempDimP1.y), feedbackColor), sf::Vertex(view.worldToScreen(engine.tempDimP2.x, engine.tempDimP2.y), feedbackColor) };
            window.draw(line1, 2, sf::Lines);
            auto inter = lineLineIntersection(engine.tempDimP1, engine.tempDimP2, engine.tempDimP1, mouseWorldPos);
            Point2D dynamicVertex = inter.intersects ? inter.point : engine.tempDimP1;
            double angle1 = std::atan2(engine.tempDimP2.y - dynamicVertex.y, engine.tempDimP2.x - dynamicVertex.x);
            double angle2 = std::atan2(mouseWorldPos.y - dynamicVertex.y, mouseWorldPos.x - dynamicVertex.x);
            double distToMouse = std::hypot(mouseWorldPos.x - dynamicVertex.x, mouseWorldPos.y - dynamicVertex.y);
            double arcRadius = std::min(distToMouse * 0.3, 50.0);
            sf::Color guideColor(255, 255, 0, 100); double extLen = arcRadius * 2.0;
            sf::Vertex guide1[] = { sf::Vertex(view.worldToScreen(dynamicVertex.x, dynamicVertex.y), guideColor), sf::Vertex(view.worldToScreen(dynamicVertex.x + std::cos(angle1) * extLen, dynamicVertex.y + std::sin(angle1) * extLen), guideColor) };
            sf::Vertex guide2[] = { sf::Vertex(view.worldToScreen(dynamicVertex.x, dynamicVertex.y), guideColor), sf::Vertex(view.worldToScreen(dynamicVertex.x + std::cos(angle2) * extLen, dynamicVertex.y + std::sin(angle2) * extLen), guideColor) };
            window.draw(guide1, 2, sf::Lines); window.draw(guide2, 2, sf::Lines);
            const int numPoints = 32; sf::VertexArray arc(sf::LineStrip, numPoints);
            double diff = angle2 - angle1; while (diff < 0) diff += 2 * PI; while (diff >= 2 * PI) diff -= 2 * PI; double step = diff / (numPoints - 1);
            for (int i = 0; i < numPoints; ++i) { double angle = angle1 + i * step; arc[i].position = view.worldToScreen(dynamicVertex.x + arcRadius * std::cos(angle), dynamicVertex.y + arcRadius * std::sin(angle)); arc[i].color = sf::Color(255, 255, 0); }
            window.draw(arc);
        }
        else if (engine.statusMessage.find("Ubicación") != std::string::npos) {
            Point2D vertex = engine.tempDimP1;
            double arcRadius = std::hypot(mouseWorldPos.x - vertex.x, mouseWorldPos.y - vertex.y);
            double angle1 = std::atan2(engine.tempDimP2.y - vertex.y, engine.tempDimP2.x - vertex.x);
            double angle2 = std::atan2(engine.tempDimP2_line2.y - vertex.y, engine.tempDimP2_line2.x - vertex.x);
            sf::Color guideColor(255, 255, 0, 100); double extLen = arcRadius * 1.5;
            sf::Vertex guide1[] = { sf::Vertex(view.worldToScreen(vertex.x, vertex.y), guideColor), sf::Vertex(view.worldToScreen(vertex.x + std::cos(angle1) * extLen, vertex.y + std::sin(angle1) * extLen), guideColor) };
            sf::Vertex guide2[] = { sf::Vertex(view.worldToScreen(vertex.x, vertex.y), guideColor), sf::Vertex(view.worldToScreen(vertex.x + std::cos(angle2) * extLen, vertex.y + std::sin(angle2) * extLen), guideColor) };
            window.draw(guide1, 2, sf::Lines); window.draw(guide2, 2, sf::Lines);
            const int numPoints = 64; sf::VertexArray arc(sf::LineStrip, numPoints);
            double diff = angle2 - angle1; while (diff < 0) diff += 2 * PI; while (diff >= 2 * PI) diff -= 2 * PI; double step = diff / (numPoints - 1);
            for (int i = 0; i < numPoints; ++i) { double angle = angle1 + i * step; arc[i].position = view.worldToScreen(vertex.x + arcRadius * std::cos(angle), vertex.y + arcRadius * std::sin(angle)); arc[i].color = sf::Color(255, 255, 0); }
            window.draw(arc);
            float arrowSize = 3.0f;
            sf::Vector2f p1Screen = view.worldToScreen(vertex.x + arcRadius * std::cos(angle1), vertex.y + arcRadius * std::sin(angle1));
            sf::CircleShape arrowStart(arrowSize); arrowStart.setFillColor(sf::Color(255, 255, 0)); arrowStart.setOrigin(arrowSize, arrowSize); arrowStart.setPosition(p1Screen); window.draw(arrowStart);
            sf::Vector2f p2Screen = view.worldToScreen(vertex.x + arcRadius * std::cos(angle2), vertex.y + arcRadius * std::sin(angle2));
            sf::CircleShape arrowEnd(arrowSize); arrowEnd.setFillColor(sf::Color(255, 255, 0)); arrowEnd.setOrigin(arrowSize, arrowSize); arrowEnd.setPosition(p2Screen); window.draw(arrowEnd);
            std::ostringstream oss; oss << std::fixed << std::setprecision(2) << engine.tempDimAngle << "°";
            sf::Text txt; txt.setFont(font); txt.setString(toSfString(oss.str())); txt.setCharacterSize(14); txt.setFillColor(sf::Color(255, 255, 0));
            sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y); txt.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 25.f); window.draw(txt);
        }
    }
    // --- SIMETRIA (MIRROR) ---
    else if (engine.currentMode == Mode::MIRROR && engine.activeCommand_) {
        if (auto* mirrorCmd = dynamic_cast<MirrorCommand*>(engine.activeCommand_.get())) {
            sf::Color axisColor(0, 255, 0, 180); sf::Color ghostColor(0, 200, 255, 100);
            for (Entity* e : engine.selectedEntities) { auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; e->draw(window, w2s_l, sf::Color(255, 165, 0, 150), view.getScale()); }
            if (mirrorCmd->hasAxisP1()) {
                sf::Vertex axisLine[] = { sf::Vertex(w2s(mirrorCmd->getAxisP1().x, mirrorCmd->getAxisP1().y), axisColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor) }; window.draw(axisLine, 2, sf::Lines);
                sf::CircleShape p1(4.0f); p1.setFillColor(axisColor); p1.setOrigin(4.0f, 4.0f); p1.setPosition(w2s(mirrorCmd->getAxisP1().x, mirrorCmd->getAxisP1().y)); window.draw(p1);
                Point2D axisP2 = {mouseWorldPos.x, mouseWorldPos.y};
                for (Entity* e : engine.selectedEntities) { auto copy = e->clone(); copy->mirror(mirrorCmd->getAxisP1(), axisP2); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; copy->draw(window, w2s_l, ghostColor, view.getScale()); }
            }
        }
    }
    // --- MOVER ---
    if (engine.currentMode == Mode::MOVE && engine.activeCommand_) {
        if (auto* moveCmd = dynamic_cast<MoveCommand*>(engine.activeCommand_.get())) {
            sf::Color originalColor(255, 165, 0, 180); sf::Color ghostColor(0, 200, 255, 120); sf::Color axisColor(255, 255, 0, 200);
            for (Entity* e : engine.selectedEntities) { auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; e->draw(window, w2s_l, originalColor, view.getScale()); }
            if (moveCmd->hasBasePoint()) {
                sf::Vertex guideLine[] = { sf::Vertex(w2s(moveCmd->getBasePoint().x, moveCmd->getBasePoint().y), axisColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor) }; window.draw(guideLine, 2, sf::Lines);
                sf::CircleShape baseMark(4.0f); baseMark.setFillColor(axisColor); baseMark.setOrigin(4.0f, 4.0f); baseMark.setPosition(w2s(moveCmd->getBasePoint().x, moveCmd->getBasePoint().y)); window.draw(baseMark);
                double dx = mouseWorldPos.x - moveCmd->getBasePoint().x; double dy = mouseWorldPos.y - moveCmd->getBasePoint().y;
                for (Entity* e : engine.selectedEntities) { auto ghost = e->clone(); ghost->move(dx, dy); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); }
            }
        }
    }
    // --- COPIAR ---
    if (engine.currentMode == Mode::COPY && engine.activeCommand_) {
        if (auto* copyCmd = dynamic_cast<CopyCommand*>(engine.activeCommand_.get())) {
            sf::Color originalColor(255, 165, 0, 180); sf::Color ghostColor(0, 255, 0, 120); sf::Color axisColor(255, 255, 0, 200);
            for (Entity* e : engine.selectedEntities) { auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; e->draw(window, w2s_l, originalColor, view.getScale()); }
            if (copyCmd->hasBasePoint()) {
                sf::Vertex guideLine[] = { sf::Vertex(w2s(copyCmd->getBasePoint().x, copyCmd->getBasePoint().y), axisColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor) }; window.draw(guideLine, 2, sf::Lines);
                sf::CircleShape baseMark(4.0f); baseMark.setFillColor(axisColor); baseMark.setOrigin(4.0f, 4.0f); baseMark.setPosition(w2s(copyCmd->getBasePoint().x, copyCmd->getBasePoint().y)); window.draw(baseMark);
                double dx = mouseWorldPos.x - copyCmd->getBasePoint().x; double dy = mouseWorldPos.y - copyCmd->getBasePoint().y;
                for (Entity* e : engine.selectedEntities) { auto ghost = e->clone(); ghost->move(dx, dy); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); }
            }
        }
    }
    // --- ROTAR ---
    if (engine.currentMode == Mode::ROTATE && engine.activeCommand_) {
        if (auto* rotCmd = dynamic_cast<RotateCommand*>(engine.activeCommand_.get())) {
            sf::Color originalColor(255, 165, 0, 180); sf::Color ghostColor(0, 200, 255, 120); sf::Color axisColor(255, 255, 0, 200);
            for (Entity* e : engine.selectedEntities) { auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; e->draw(window, w2s_l, originalColor, view.getScale()); }
            if (rotCmd->hasCenter()) {
                sf::Vertex guideLine[] = { sf::Vertex(w2s(rotCmd->getCenter().x, rotCmd->getCenter().y), axisColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor) }; window.draw(guideLine, 2, sf::Lines);
                sf::CircleShape centerMark(5.0f); centerMark.setFillColor(axisColor); centerMark.setOrigin(5.0f, 5.0f); centerMark.setPosition(w2s(rotCmd->getCenter().x, rotCmd->getCenter().y)); window.draw(centerMark);
                double dx = mouseWorldPos.x - rotCmd->getCenter().x; double dy = mouseWorldPos.y - rotCmd->getCenter().y;
                double angle = std::atan2(dy, dx) * 180.0 / 3.14159265358979323846;
                for (Entity* e : engine.selectedEntities) { auto ghost = e->clone(); ghost->rotate(rotCmd->getCenter(), angle); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); }
                std::ostringstream oss; oss << std::fixed << std::setprecision(1) << angle << "°";
                sf::Text angleText; angleText.setFont(font); angleText.setString(toSfString(oss.str())); angleText.setCharacterSize(14); angleText.setFillColor(sf::Color::Yellow);
                sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y); angleText.setPosition(mouseScreen.x + 12.f, mouseScreen.y - 25.f); window.draw(angleText);
            }
        }
    }
    // --- ESCALAR ---
    if (engine.currentMode == Mode::SCALE && engine.activeCommand_) {
        if (auto* scaleCmd = dynamic_cast<ScaleCommand*>(engine.activeCommand_.get())) {
            sf::Color originalColor(255, 165, 0, 180); sf::Color ghostColor(255, 100, 255, 120); sf::Color axisColor(255, 255, 0, 200);
            for (Entity* e : engine.selectedEntities) { auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; e->draw(window, w2s_l, originalColor, view.getScale()); }
            if (scaleCmd->hasBasePoint()) {
                sf::Vertex guideLine[] = { sf::Vertex(w2s(scaleCmd->getBasePoint().x, scaleCmd->getBasePoint().y), axisColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), axisColor) }; window.draw(guideLine, 2, sf::Lines);
                sf::CircleShape baseMark(5.0f); baseMark.setFillColor(axisColor); baseMark.setOrigin(5.0f, 5.0f); baseMark.setPosition(w2s(scaleCmd->getBasePoint().x, scaleCmd->getBasePoint().y)); window.draw(baseMark);
                double dx = mouseWorldPos.x - scaleCmd->getBasePoint().x; double dy = mouseWorldPos.y - scaleCmd->getBasePoint().y;
                double factor = std::sqrt(dx * dx + dy * dy);
                for (Entity* e : engine.selectedEntities) { auto ghost = e->clone(); ghost->scale(scaleCmd->getBasePoint(), factor); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); }
                std::ostringstream oss; oss << std::fixed << std::setprecision(2) << factor << "x";
                sf::Text factorText; factorText.setFont(font); factorText.setString(toSfString(oss.str())); factorText.setCharacterSize(14); factorText.setFillColor(sf::Color::Magenta);
                sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y); factorText.setPosition(mouseScreen.x + 12.f, mouseScreen.y - 25.f); window.draw(factorText);
            }
        }
    }
    // --- OFFSET ---
    if (engine.currentMode == Mode::OFFSET && engine.activeCommand_) {
        if (auto* offsetCmd = dynamic_cast<OffsetCommand*>(engine.activeCommand_.get())) {
            sf::Color previewColor(255, 255, 0, 150);
            if (offsetCmd->hasDistance() && offsetCmd->getSelectedEntity()) {
                Entity* entity = offsetCmd->getSelectedEntity(); double distance = offsetCmd->getDistance();
                if (auto* line = dynamic_cast<Line*>(entity)) {
                    double dx = line->p2.x - line->p1.x; double dy = line->p2.y - line->p1.y; double len = std::sqrt(dx * dx + dy * dy);
                    if (len > 0) {
                        double nx = -dy / len; double ny = dx / len; double vx = mouseWorldPos.x - line->p1.x; double vy = mouseWorldPos.y - line->p1.y;
                        double side = vx * nx + vy * ny; double sign = (side >= 0) ? 1.0 : -1.0;
                        sf::Vertex linePreview[] = { sf::Vertex(w2s(line->p1.x + nx * distance * sign, line->p1.y + ny * distance * sign), previewColor), sf::Vertex(w2s(line->p2.x + nx * distance * sign, line->p2.y + ny * distance * sign), previewColor) };
                        window.draw(linePreview, 2, sf::Lines);
                    }
                }
                else if (auto* circle = dynamic_cast<Circle*>(entity)) {
                    double newRadius = circle->radius + distance; if (newRadius < 0) newRadius = std::abs(newRadius);
                    sf::CircleShape circlePreview(static_cast<float>(newRadius * view.getScale())); circlePreview.setFillColor(sf::Color::Transparent); circlePreview.setOutlineColor(previewColor); circlePreview.setOutlineThickness(1.5f);
                    circlePreview.setOrigin(static_cast<float>(newRadius * view.getScale()), static_cast<float>(newRadius * view.getScale())); circlePreview.setPosition(view.worldToScreen(circle->center.x, circle->center.y)); window.draw(circlePreview);
                }
                else if (auto* arc = dynamic_cast<Arc*>(entity)) {
                    const double PI = 3.14159265358979323846; double newRadius = arc->radius + distance; if (newRadius < 0) newRadius = std::abs(newRadius);
                    const int numPoints = 64; sf::VertexArray va(sf::LineStrip, numPoints);
                    double startRad = arc->startAngle * PI / 180.0; double endRad = arc->endAngle * PI / 180.0; double step = (endRad - startRad) / (numPoints - 1);
                    for (int i = 0; i < numPoints; ++i) { double angle = startRad + i * step; va[i].position = w2s(arc->center.x + newRadius * std::cos(angle), arc->center.y + newRadius * std::sin(angle)); va[i].color = previewColor; }
                    window.draw(va);
                }
            }
        }
    }
    // --- FILLET ---
    if (engine.currentMode == Mode::FILLET && engine.activeCommand_) {
        if (auto* filletCmd = dynamic_cast<FilletCommand*>(engine.activeCommand_.get())) {
            sf::Color highlightColor(0, 255, 0, 150); sf::Color previewColor(255, 255, 0, 200);
            if (filletCmd->hasLine1() && filletCmd->getLine1()) {
                Line* l1 = filletCmd->getLine1();
                sf::Vertex line1[] = { sf::Vertex(w2s(l1->p1.x, l1->p1.y), highlightColor), sf::Vertex(w2s(l1->p2.x, l1->p2.y), highlightColor) }; window.draw(line1, 2, sf::Lines);
                Line* hoverLine = nullptr; double tolerance = 10.0 / view.getScale();
                for (auto& entity : engine.doc.entities) { if (auto* line = dynamic_cast<Line*>(entity.get())) { if (line->isNear(mouseWorldPos, tolerance) && line != l1) { hoverLine = line; break; } } }
                if (hoverLine) {
                    auto inter = lineLineIntersection(l1->p1, l1->p2, hoverLine->p1, hoverLine->p2);
                    if (inter.intersects && filletCmd->getRadius() > 0) {
                        Point2D I = inter.point;
                        auto normalize = [](Point2D a, Point2D b) { double dx = b.x - a.x, dy = b.y - a.y; double len = std::sqrt(dx * dx + dy * dy); return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0}; };
                        double d1a = std::hypot(l1->p1.x - I.x, l1->p1.y - I.y); double d1b = std::hypot(l1->p2.x - I.x, l1->p2.y - I.y); Point2D end1 = (d1a < d1b) ? l1->p1 : l1->p2;
                        double d2a = std::hypot(hoverLine->p1.x - I.x, hoverLine->p1.y - I.y); double d2b = std::hypot(hoverLine->p2.x - I.x, hoverLine->p2.y - I.y); Point2D end2 = (d2a < d2b) ? hoverLine->p1 : hoverLine->p2;
                        Point2D v1 = normalize(I, end1); Point2D v2 = normalize(I, end2);
                        double cosAngle = v1.x * v2.x + v1.y * v2.y; if (cosAngle > 1.0) cosAngle = 1.0; if (cosAngle < -1.0) cosAngle = -1.0;
                        double angle = std::acos(cosAngle);
                        if (angle > 0.001) {
                            double d = filletCmd->getRadius() / std::tan(angle / 2.0);
                            Point2D T1 = {I.x + v1.x * d, I.y + v1.y * d}; Point2D T2 = {I.x + v2.x * d, I.y + v2.y * d};
                            Point2D bisector = {v1.x + v2.x, v1.y + v2.y}; double bisLen = std::sqrt(bisector.x * bisector.x + bisector.y * bisector.y);
                            if (bisLen > 0) {
                                bisector.x /= bisLen; bisector.y /= bisLen; double h = filletCmd->getRadius() / std::sin(angle / 2.0);
                                Point2D center = {I.x + bisector.x * h, I.y + bisector.y * h};
                                const int numPoints = 32; sf::VertexArray arc(sf::LineStrip, numPoints);
                                double a1 = std::atan2(T1.y - center.y, T1.x - center.x); double a2 = std::atan2(T2.y - center.y, T2.x - center.x);
                                double diff = a2 - a1; while (diff < 0) diff += 2 * 3.14159265; while (diff >= 2 * 3.14159265) diff -= 2 * 3.14159265;
                                if (diff > 3.14159265) std::swap(a1, a2); diff = a2 - a1; while (diff < 0) diff += 2 * 3.14159265; double step = diff / (numPoints - 1);
                                for (int i = 0; i < numPoints; ++i) { double a = a1 + i * step; arc[i].position = w2s(center.x + filletCmd->getRadius() * std::cos(a), center.y + filletCmd->getRadius() * std::sin(a)); arc[i].color = previewColor; }
                                window.draw(arc);
                            }
                        }
                    }
                }
            }
        }
    }
    // --- CHAMFER ---
    if (engine.currentMode == Mode::CHAMFER && engine.activeCommand_) {
        if (auto* chamferCmd = dynamic_cast<ChamferCommand*>(engine.activeCommand_.get())) {
            sf::Color highlightColor(0, 255, 0, 150); sf::Color previewColor(255, 255, 0, 200);
            if (chamferCmd->hasLine1() && chamferCmd->getLine1()) {
                Line* l1 = chamferCmd->getLine1();
                sf::Vertex line1[] = { sf::Vertex(w2s(l1->p1.x, l1->p1.y), highlightColor), sf::Vertex(w2s(l1->p2.x, l1->p2.y), highlightColor) }; window.draw(line1, 2, sf::Lines);
                Line* hoverLine = nullptr; double tolerance = 10.0 / view.getScale();
                for (auto& entity : engine.doc.entities) { if (auto* line = dynamic_cast<Line*>(entity.get())) { if (line->isNear(mouseWorldPos, tolerance) && line != l1) { hoverLine = line; break; } } }
                if (hoverLine) {
                    auto inter = lineLineIntersection(l1->p1, l1->p2, hoverLine->p1, hoverLine->p2);
                    if (inter.intersects) {
                        Point2D I = inter.point;
                        auto normalize = [](Point2D a, Point2D b) { double dx = b.x - a.x, dy = b.y - a.y; double len = std::sqrt(dx*dx + dy*dy); return len > 0 ? Point2D{dx/len, dy/len} : Point2D{0,0}; };
                        double d1a = std::hypot(l1->p1.x - I.x, l1->p1.y - I.y); double d1b = std::hypot(l1->p2.x - I.x, l1->p2.y - I.y); Point2D end1 = (d1a < d1b) ? l1->p1 : l1->p2;
                        double d2a = std::hypot(hoverLine->p1.x - I.x, hoverLine->p1.y - I.y); double d2b = std::hypot(hoverLine->p2.x - I.x, hoverLine->p2.y - I.y); Point2D end2 = (d2a < d2b) ? hoverLine->p1 : hoverLine->p2;
                        Point2D v1 = normalize(I, end1); Point2D v2 = normalize(I, end2);
                        Point2D T1 = {I.x + v1.x * chamferCmd->getDist1(), I.y + v1.y * chamferCmd->getDist1()}; Point2D T2 = {I.x + v2.x * chamferCmd->getDist2(), I.y + v2.y * chamferCmd->getDist2()};
                        sf::Vertex chamferLine[] = { sf::Vertex(w2s(T1.x, T1.y), previewColor), sf::Vertex(w2s(T2.x, T2.y), previewColor) }; window.draw(chamferLine, 2, sf::Lines);
                    }
                }
            }
        }
    }
    // --- TRIM ---
    if (engine.currentMode == Mode::TRIM && engine.activeCommand_) {
        if (auto* trimCmd = dynamic_cast<TrimCommand*>(engine.activeCommand_.get())) {
            sf::Color boundaryColor(0, 255, 0, 150);
            for (Entity* boundary : trimCmd->getBoundaries()) { if (auto* line = dynamic_cast<Line*>(boundary)) { sf::Vertex lineVerts[] = { sf::Vertex(w2s(line->p1.x, line->p1.y), boundaryColor), sf::Vertex(w2s(line->p2.x, line->p2.y), boundaryColor) }; window.draw(lineVerts, 2, sf::Lines); } }
        }
    }
    // --- EXTEND ---
    if (engine.currentMode == Mode::EXTEND && engine.activeCommand_) {
        if (auto* extendCmd = dynamic_cast<ExtendCommand*>(engine.activeCommand_.get())) {
            sf::Color boundaryColor(0, 255, 0, 150);
            for (Entity* boundary : extendCmd->getBoundaries()) { if (auto* line = dynamic_cast<Line*>(boundary)) { sf::Vertex lineVerts[] = { sf::Vertex(w2s(line->p1.x, line->p1.y), boundaryColor), sf::Vertex(w2s(line->p2.x, line->p2.y), boundaryColor) }; window.draw(lineVerts, 2, sf::Lines); } }
        }
    }
    // --- ARRAY ---
    else if (engine.currentMode == Mode::ARRAY && engine.activeCommand_) {
        if (auto* arrayCmd = dynamic_cast<ArrayCommand*>(engine.activeCommand_.get())) {
            sf::Color highlightColor(255, 165, 0, 150); sf::Color ghostColor(0, 200, 255, 120); sf::Color axisColor(255, 255, 0, 200);
            for (Entity* e : arrayCmd->getSelectedEntities()) { auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; e->draw(window, w2s_l, highlightColor, view.getScale()); }
            if (!arrayCmd->isSelectingEntities()) {
                if (arrayCmd->getType() == ArrayCommand::Type::Rectangular) {
                    if (arrayCmd->getRows() > 0 && arrayCmd->getCols() > 0 && arrayCmd->getRowSpacing() != 0.0 && arrayCmd->getColSpacing() != 0.0) {
                        for (int r = 0; r < arrayCmd->getRows(); ++r) { for (int c = 0; c < arrayCmd->getCols(); ++c) { if (r == 0 && c == 0) continue; double dx = c * arrayCmd->getColSpacing(); double dy = r * arrayCmd->getRowSpacing(); for (Entity* e : arrayCmd->getSelectedEntities()) { auto ghost = e->clone(); ghost->move(dx, dy); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); } } }
                    }
                }
                else if (arrayCmd->getType() == ArrayCommand::Type::Polar && arrayCmd->hasPolarCenter()) {
                    Point2D center = arrayCmd->getPolarCenter(); int count = arrayCmd->getPolarCount(); double angle = arrayCmd->getPolarAngle();
                    if (count > 1 && angle != 0.0) { double angleStep = angle / count; for (int i = 1; i < count; ++i) { double ang = i * angleStep; for (Entity* e : arrayCmd->getSelectedEntities()) { auto ghost = e->clone(); ghost->rotate(center, ang); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); } } }
                    sf::CircleShape centerMark(5.0f); centerMark.setFillColor(axisColor); centerMark.setOrigin(5.0f, 5.0f); centerMark.setPosition(view.worldToScreen(center.x, center.y)); window.draw(centerMark);
                }
                else if (arrayCmd->getType() == ArrayCommand::Type::Polar && !arrayCmd->hasPolarCenter()) {
                    Point2D center = {mouseWorldPos.x, mouseWorldPos.y}; int count = arrayCmd->getPolarCount(); double angle = arrayCmd->getPolarAngle();
                    if (count > 1 && angle != 0.0) { double angleStep = angle / count; for (int i = 1; i < count; ++i) { double ang = i * angleStep; for (Entity* e : arrayCmd->getSelectedEntities()) { auto ghost = e->clone(); ghost->rotate(center, ang); auto w2s_l = [&](double x, double y){ return view.worldToScreen(x,y); }; ghost->draw(window, w2s_l, ghostColor, view.getScale()); } } }
                    sf::CircleShape centerMark(5.0f); centerMark.setFillColor(axisColor); centerMark.setOrigin(5.0f, 5.0f); centerMark.setPosition(view.worldToScreen(center.x, center.y)); window.draw(centerMark);
                }
            }
        }
    }
    // --- STRETCH ---
    if (engine.currentMode == Mode::STRETCH && engine.activeCommand_) {
        if (auto* stretchCmd = dynamic_cast<StretchCommand*>(engine.activeCommand_.get())) {
            if (stretchCmd->getStep() == StretchCommand::Step::SelectingWindowP2 && stretchCmd->hasWindowP1()) {
                sf::Color windowColor(0, 255, 0, 100); sf::RectangleShape windowRect;
                double x1 = stretchCmd->getWindowP1().x; double y1 = stretchCmd->getWindowP1().y;
                double x2 = mouseWorldPos.x; double y2 = mouseWorldPos.y;
                float w = static_cast<float>(std::abs(x2 - x1) * view.getScale()); float h = static_cast<float>(std::abs(y2 - y1) * view.getScale());
                windowRect.setSize(sf::Vector2f(w, h)); windowRect.setFillColor(windowColor); windowRect.setOutlineColor(sf::Color(0, 255, 0, 200)); windowRect.setOutlineThickness(1.0f);
                windowRect.setPosition(view.worldToScreen(std::min(x1, x2), std::max(y1, y2))); window.draw(windowRect);
            }
        }
    }
    // --- BLOCK (Feedback visual de creación) ---
    else if (engine.currentMode == Mode::BLOCK_CREATE && engine.activeCommand_) {
        if (auto* blockCmd = dynamic_cast<BlockCreateCommand*>(engine.activeCommand_.get())) {
            if (!blockCmd->getSelectedEntities().empty()) {
                sf::Color highlightColor(0, 255, 255, 200); // Cian brillante
                
                // Lambda para transformar coordenadas usando la nueva clase View
                auto w2s_local = [&](double x, double y) { 
                    return view.worldToScreen(x, y); 
                };

                // Dibujar las entidades seleccionadas con color destacado
                for (Entity* e : blockCmd->getSelectedEntities()) {
                    e->draw(window, w2s_local, highlightColor, view.getScale());
                }
                
                // Mostrar contador de entidades seleccionadas
                std::ostringstream oss;
                oss << "Entidades seleccionadas: " << blockCmd->getSelectedEntities().size();
                
                sf::Text countText;
                countText.setFont(font); // O font_ si lo tienes como miembro en Renderer
                countText.setString(toSfString(oss.str()));
                countText.setCharacterSize(14);
                countText.setFillColor(sf::Color(0, 255, 255));
                countText.setPosition(10, MENU_HEIGHT + TOOLBAR_HEIGHT + 10);
                window.draw(countText);
            }
        }
    }
}

void Renderer::drawSelectionRect(sf::RenderWindow& window, const View& view,
                                 const Point2D& startPoint, const Point2D& endPoint) const {
    auto startScreen = view.worldToScreen(startPoint.x, startPoint.y);
    auto endScreen = view.worldToScreen(endPoint.x, endPoint.y);
    
    sf::RectangleShape selectionRect;
    selectionRect.setPosition(std::min(startScreen.x, endScreen.x), 
                              std::min(startScreen.y, endScreen.y));
    selectionRect.setSize({std::abs(endScreen.x - startScreen.x), 
                          std::abs(endScreen.y - startScreen.y)});
    selectionRect.setOutlineThickness(1.0f);
    
    // Determinar color según dirección del arrastre
    if (startPoint.x < endPoint.x) {
        // WINDOW: Azul (izquierda → derecha)
        selectionRect.setFillColor(sf::Color(0, 100, 255, 40));   // Azul translúcido
        selectionRect.setOutlineColor(sf::Color::Blue);
    } else {
        // CROSSING: Verde (derecha → izquierda)
        selectionRect.setFillColor(sf::Color(0, 200, 100, 40));   // Verde translúcido
        selectionRect.setOutlineColor(sf::Color::Green);
    }
    
    window.draw(selectionRect);
}

} // namespace cad