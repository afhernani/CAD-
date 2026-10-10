#include "cad/core/geometry/entities/hatch.hpp"
#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;

        bool segmentIntersect(const Point2D& p1, const Point2D& p2,
                              const Point2D& p3, const Point2D& p4,
                              Point2D& result, double& t) {
            double d = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
            if (std::abs(d) < 1e-10) return false;
            
            double ua = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x)) / d;
            double ub = ((p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x)) / d;
            
            if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
                result.x = p1.x + ua * (p2.x - p1.x);
                result.y = p1.y + ua * (p2.y - p1.y);
                t = ua;
                return true;
            }
            return false;
        }
    }

    bool Hatch::isPointInPolygon(const Point2D& p, const std::vector<Point2D>& poly) const {
        if (poly.size() < 3) return false;
        bool inside = false;
        for (size_t i = 0, j = poly.size() - 1; i < poly.size(); j = i++) {
            if (((poly[i].y > p.y) != (poly[j].y > p.y)) &&
                (p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / (poly[j].y - poly[i].y) + poly[i].x)) {
                inside = !inside;
            }
        }
        return inside;
    }

    bool Hatch::isPointInAnyIsland(const Point2D& p) const {
        for (const auto& island : islands) {
            if (isPointInPolygon(p, island)) return true;
        }
        return false;
    }

    void Hatch::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                     const sf::Color& color, float viewScale) const {
        if (points.size() < 3) return;

        if (pattern == HatchPattern::SOLID) {
            sf::ConvexShape shape;
            shape.setPointCount(points.size());
            for (size_t i = 0; i < points.size(); ++i) {
                shape.setPoint(i, w2s(points[i].x, points[i].y));
            }
            sf::Color fillColor = color;
            fillColor.a = 150; 
            shape.setFillColor(fillColor);
            window.draw(shape);
            return;
        }

        double minX = points[0].x, maxX = points[0].x;
        double minY = points[0].y, maxY = points[0].y;
        for (const auto& p : points) {
            minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
            minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
        }
        
        double cx = (minX + maxX) / 2.0;
        double cy = (minY + maxY) / 2.0;
        double diagLen = std::hypot(maxX - minX, maxY - minY);

        double rad = angle * PI / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        
        double realSpacing = spacing;
        if (realSpacing <= 0.0) realSpacing = 1.0;

        sf::VertexArray lines(sf::Lines);

        for (double offset = -diagLen; offset <= diagLen; offset += realSpacing) {
            double perpX = -sinA * offset;
            double perpY = cosA * offset;
            
            double lineLen = diagLen * 2.0;
            Point2D lineStart = {cx + cosA * (-lineLen) + perpX, cy + sinA * (-lineLen) + perpY};
            Point2D lineEnd = {cx + cosA * lineLen + perpX, cy + sinA * lineLen + perpY};

            // 1. Intersecciones con borde exterior
            std::vector<std::pair<double, Point2D>> outerIntersections;
            for (size_t i = 0; i < points.size(); ++i) {
                size_t j = (i + 1) % points.size();
                Point2D intersection;
                double t;
                if (segmentIntersect(lineStart, lineEnd, points[i], points[j], intersection, t)) {
                    outerIntersections.push_back({t, intersection});
                }
            }

            if (outerIntersections.size() < 2) continue;

            std::sort(outerIntersections.begin(), outerIntersections.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            
            // 2. Procesar segmentos dentro del borde exterior
            for (size_t i = 0; i + 1 < outerIntersections.size(); i += 2) {
                Point2D segStart = outerIntersections[i].second;
                Point2D segEnd = outerIntersections[i+1].second;
                
                Point2D midOuter = {(segStart.x + segEnd.x)/2, (segStart.y + segEnd.y)/2};
                if (!isPointInPolygon(midOuter, points)) continue;

                // 3. Si hay islas, recortar
                if (!islands.empty()) {
                    std::vector<double> tValues = {0.0, 1.0};
                    
                    for (const auto& island : islands) {
                        for (size_t k = 0; k < island.size(); ++k) {
                            size_t l = (k + 1) % island.size();
                            Point2D intersection;
                            double tIsland;
                            if (segmentIntersect(segStart, segEnd, island[k], island[l], intersection, tIsland)) {
                                tValues.push_back(tIsland);
                            }
                        }
                    }
                    
                    std::sort(tValues.begin(), tValues.end());
                    
                    for (size_t k = 0; k + 1 < tValues.size(); k += 2) {
                        double t1 = tValues[k];
                        double t2 = tValues[k+1];
                        double tMid = (t1 + t2) / 2.0;
                        
                        Point2D p1 = {segStart.x + t1 * (segEnd.x - segStart.x), segStart.y + t1 * (segEnd.y - segStart.y)};
                        Point2D p2 = {segStart.x + t2 * (segEnd.x - segStart.x), segStart.y + t2 * (segEnd.y - segStart.y)};
                        Point2D pMid = {segStart.x + tMid * (segEnd.x - segStart.x), segStart.y + tMid * (segEnd.y - segStart.y)};
                        
                        if (!isPointInAnyIsland(pMid)) {
                            lines.append(sf::Vertex(w2s(p1.x, p1.y), color));
                            lines.append(sf::Vertex(w2s(p2.x, p2.y), color));
                        }
                    }
                } else {
                    lines.append(sf::Vertex(w2s(segStart.x, segStart.y), color));
                    lines.append(sf::Vertex(w2s(segEnd.x, segEnd.y), color));
                }
            }
        }
        
        window.draw(lines);
    }

    bool Hatch::isNear(const Point2D& point, double tolerance) const {
        if (isPointInPolygon(point, points)) return true;
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            double d1 = std::hypot(point.x - points[i].x, point.y - points[i].y);
            if (d1 < tolerance) return true;
        }
        return false;
    }

    void Hatch::move(double dx, double dy) {
        for (auto& p : points) { p.x += dx; p.y += dy; }
        for (auto& island : islands) {
            for (auto& p : island) { p.x += dx; p.y += dy; }
        }
    }

    std::unique_ptr<Entity> Hatch::clone() const {
        auto h = std::make_unique<Hatch>();
        h->copyFrom(*this);
        return h;
    }

    void Hatch::rotate(const Point2D& center, double angleDeg) {
        double rad = angleDeg * PI / 180.0;
        double cosA = std::cos(rad), sinA = std::sin(rad);
        for (auto& p : points) {
            double dx = p.x - center.x, dy = p.y - center.y;
            p.x = center.x + dx * cosA - dy * sinA;
            p.y = center.y + dx * sinA + dy * cosA;
        }
        for (auto& island : islands) {
            for (auto& p : island) {
                double dx = p.x - center.x, dy = p.y - center.y;
                p.x = center.x + dx * cosA - dy * sinA;
                p.y = center.y + dx * sinA + dy * cosA;
            }
        }
    }

    void Hatch::scale(const Point2D& basePoint, double factor) {
        for (auto& p : points) {
            p.x = basePoint.x + (p.x - basePoint.x) * factor;
            p.y = basePoint.y + (p.y - basePoint.y) * factor;
        }
        for (auto& island : islands) {
            for (auto& p : island) {
                p.x = basePoint.x + (p.x - basePoint.x) * factor;
                p.y = basePoint.y + (p.y - basePoint.y) * factor;
            }
        }
    }

    void Hatch::mirror(const Point2D& axisP1, const Point2D& axisP2) {
        double dx = axisP2.x - axisP1.x, dy = axisP2.y - axisP1.y;
        double len2 = dx*dx + dy*dy;
        if (len2 == 0) return;
        for (auto& p : points) {
            double t = ((p.x - axisP1.x)*dx + (p.y - axisP1.y)*dy) / len2;
            double projX = axisP1.x + t*dx, projY = axisP1.y + t*dy;
            p.x = 2*projX - p.x;
            p.y = 2*projY - p.y;
        }
        for (auto& island : islands) {
            for (auto& p : island) {
                double t = ((p.x - axisP1.x)*dx + (p.y - axisP1.y)*dy) / len2;
                double projX = axisP1.x + t*dx, projY = axisP1.y + t*dy;
                p.x = 2*projX - p.x;
                p.y = 2*projY - p.y;
            }
        }
    }

    std::vector<Point2D> Hatch::getGripPoints() const { return points; }
    std::vector<Point2D> Hatch::getSnapPoints() const { return points; }
    
    void Hatch::moveGrip(int index, const Point2D& newPos) {
        if (index >= 0 && index < points.size()) points[index] = newPos;
    }

    void Hatch::update(const Document& doc) {
        // Actualizar borde exterior
        if (!boundaryId.empty()) {
            for (const auto& entity : doc.entities) {
                if (entity->id == boundaryId) {
                    std::vector<Point2D> newPoints;
                    if (auto* poly = dynamic_cast<Polyline*>(entity.get())) {
                        if (poly->closed && poly->points.size() >= 3) newPoints = poly->points;
                    }
                    else if (auto* polygon = dynamic_cast<Polygon*>(entity.get())) {
                        if (polygon->points.size() >= 3) newPoints = polygon->points;
                    }
                    else if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                        int segments = 32;
                        double angleStep = 2 * PI / segments;
                        for (int i = 0; i < segments; ++i) {
                            double angle = i * angleStep;
                            newPoints.push_back({circle->center.x + circle->radius * std::cos(angle),
                                                 circle->center.y + circle->radius * std::sin(angle)});
                        }
                    }
                    if (newPoints.size() >= 3) points = std::move(newPoints);
                    break;
                }
            }
        }

        // Actualizar islas
        for (size_t i = 0; i < islandIds.size(); ++i) {
            const std::string& islandId = islandIds[i];
            if (islandId.empty()) continue;
            
            for (const auto& entity : doc.entities) {
                if (entity->id == islandId) {
                    std::vector<Point2D> newPoints;
                    if (auto* poly = dynamic_cast<Polyline*>(entity.get())) {
                        if (poly->closed && poly->points.size() >= 3) newPoints = poly->points;
                    }
                    else if (auto* polygon = dynamic_cast<Polygon*>(entity.get())) {
                        if (polygon->points.size() >= 3) newPoints = polygon->points;
                    }
                    else if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                        int segments = 32;
                        double angleStep = 2 * PI / segments;
                        for (int j = 0; j < segments; ++j) {
                            double angle = j * angleStep;
                            newPoints.push_back({circle->center.x + circle->radius * std::cos(angle),
                                                 circle->center.y + circle->radius * std::sin(angle)});
                        }
                    }
                    if (newPoints.size() >= 3 && i < islands.size()) {
                        islands[i] = std::move(newPoints);
                    }
                    break;
                }
            }
        }
    }

    void Hatch::copyFrom(const Entity& src) {
        const Hatch* h = dynamic_cast<const Hatch*>(&src);
        if (h) {
            layerName = h->layerName;
            id = h->id;
            points = h->points;
            islands = h->islands;
            boundaryId = h->boundaryId;
            islandIds = h->islandIds;
            pattern = h->pattern;
            patternScale = h->patternScale;
            angle = h->angle;
            spacing = h->spacing;
        }
    }

    nlohmann::json Hatch::toJson() const {
        nlohmann::json j;
        j["type"] = "Hatch";
        j["id"] = id;
        j["layer"] = layerName;
        j["pattern"] = static_cast<int>(pattern);
        j["patternScale"] = patternScale;
        j["angle"] = angle;
        j["spacing"] = spacing;
        j["boundaryId"] = boundaryId;
        
        j["points"] = nlohmann::json::array();
        for (const auto& p : points) {
            j["points"].push_back({{"x", p.x}, {"y", p.y}});
        }
        
        j["islandIds"] = nlohmann::json::array();
        for (const auto& id : islandIds) {
            j["islandIds"].push_back(id);
        }
        
        j["islands"] = nlohmann::json::array();
        for (const auto& island : islands) {
            nlohmann::json islandArr = nlohmann::json::array();
            for (const auto& p : island) {
                islandArr.push_back({{"x", p.x}, {"y", p.y}});
            }
            j["islands"].push_back(islandArr);
        }
        
        return j;
    }

    std::unique_ptr<Hatch> Hatch::fromJson(const nlohmann::json& j) {
        auto h = std::make_unique<Hatch>();
        h->layerName = j.value("layer", "0");
        h->id = j.value("id", Entity::generateId());
        h->pattern = static_cast<HatchPattern>(j.value("pattern", 1));
        h->patternScale = j.value("patternScale", 1.0);
        h->angle = j.value("angle", 0.0);
        h->spacing = j.value("spacing", 1.0);
        h->boundaryId = j.value("boundaryId", "");
        
        if (j.contains("points")) {
            for (const auto& p : j["points"]) {
                h->points.push_back({p["x"].get<double>(), p["y"].get<double>()});
            }
        }
        
        if (j.contains("islandIds")) {
            for (const auto& id : j["islandIds"]) {
                h->islandIds.push_back(id.get<std::string>());
            }
        }
        
        if (j.contains("islands")) {
            for (const auto& islandArr : j["islands"]) {
                std::vector<Point2D> island;
                for (const auto& p : islandArr) {
                    island.push_back({p["x"].get<double>(), p["y"].get<double>()});
                }
                h->islands.push_back(island);
            }
        }
        
        return h;
    }

} // namespace cad