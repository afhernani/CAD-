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
// #include "cad/commands/draw/line_command.hpp"
// #include "cad/commands/draw/circle_command.hpp"
// #include "cad/commands/draw/arc_command.hpp"
// #include "cad/commands/draw/polygon_command.hpp"
// #include "cad/commands/draw/ellipse_command.hpp"
// #include "cad/commands/draw/polyline_command.hpp"
// #include "cad/commands/modify/move_command.hpp"
//#include "cad/commands/modify/copy_command.hpp"
//#include "cad/commands/modify/rotate_command.hpp"
//#include "cad/commands/modify/scale_command.hpp"
//#include "cad/commands/modify/mirror_command.hpp"
//#include "cad/commands/modify/fillet_command.hpp"
// #include "cad/commands/modify/chamfer_command.hpp"
// #include "cad/commands/modify/trim_command.hpp"
// #include "cad/commands/modify/extend_command.hpp"
// #include "cad/commands/modify/measure_command.hpp"
// #include "cad/commands/modify/offset_command.hpp"
// #include "cad/commands/modify/stretch_command.hpp"
// #include "cad/commands/modify/array_command.hpp"
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
    drawDrawingFeedback(window, view, engine, mouseWorldPos, font);
    
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
    if (engine.activeCommand_) {
        // >>> ACTUALIZADO: Ahora pasamos 'engine' como tercer argumento
        engine.activeCommand_->drawFeedback(window, view, engine, mouseWorldPos, font);
    }
    // procedimiento antiguo.
    sf::Color feedbackColor(255, 255, 0, 180);
    auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

    // --- COTA (DIMENSION) ---
    if (engine.currentMode == Mode::DRAW_DIMENSION) {
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