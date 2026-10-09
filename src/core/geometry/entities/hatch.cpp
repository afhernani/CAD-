#include "cad/core/geometry/entities/hatch.hpp"
#include "cad/core/document/document.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/entities/polygon.hpp"
#include "cad/core/geometry/entities/circle.hpp"

#include <cmath>
#include <algorithm>
#include <iostream>

namespace cad {

    // Función auxiliar: calcular intersección de dos segmentos
    namespace {
        constexpr double PI = 3.14159265358979323846;

        bool segmentIntersection(const Point2D& p1, const Point2D& p2, 
                                 const Point2D& p3, const Point2D& p4, 
                                 Point2D& intersection) {
            double denom = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
            if (std::abs(denom) < 1e-10) return false; // Paralelos
            
            double ua = ((p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x)) / denom;
            double ub = ((p2.x - p1.x) * (p1.y - p3.y) - (p2.y - p1.y) * (p1.x - p3.x)) / denom;
            
            if (ua >= 0.0 && ua <= 1.0 && ub >= 0.0 && ub <= 1.0) {
                intersection.x = p1.x + ua * (p2.x - p1.x);
                intersection.y = p1.y + ua * (p2.y - p1.y);
                return true;
            }
            return false;
        }
    }

    void Hatch::draw(sf::RenderWindow& window, const WorldToScreenFn& w2s,
                 const sf::Color& color, float viewScale) const {
        if (points.size() < 3) return;

        // 1. Patrón Sólido (caso especial)
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

        // 2. Si la geometría cambió (ej. se movió el borde), la recalculamos AHORA
        if (isGeometryDirty) {
            regenerateGeometry();
        }

        // 3. Dibujar desde el caché (¡Operación ultrarrápida!)
        if (!cachedSegments.empty()) {
            sf::VertexArray lines(sf::Lines, cachedSegments.size() * 2);
            size_t vertexIndex = 0;
            
            for (const auto& segment : cachedSegments) {
                lines[vertexIndex++].position = w2s(segment.first.x, segment.first.y);
                lines[vertexIndex++].color = color;
                
                lines[vertexIndex++].position = w2s(segment.second.x, segment.second.y);
                lines[vertexIndex++].color = color;
            }
            
            window.draw(lines);
        }
    }

    void Hatch::regenerateGeometry() const {
        cachedSegments.clear();
        if (points.size() < 3) {
            isGeometryDirty = false;
            return;
        }

        if (pattern == HatchPattern::SOLID) {
            isGeometryDirty = false;
            return; // El sólido se dibuja directo, no necesita segmentos de línea
        }

        // 1. Calcular Bounding Box
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

        // Función de recorte (la misma que ya tenías, pero ahora guarda en cachedSegments)
        auto clipLineToPolygon = [&](const Point2D& l1, const Point2D& l2) {
            std::vector<std::pair<double, Point2D>> intersections;
            
            for (size_t i = 0; i < points.size(); ++i) {
                size_t j = (i + 1) % points.size();
                double x1 = l1.x, y1 = l1.y;
                double x2 = l2.x, y2 = l2.y;
                double x3 = points[i].x, y3 = points[i].y;
                double x4 = points[j].x, y4 = points[j].y;
                
                double denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
                if (std::abs(denom) < 1e-10) continue;
                
                double t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
                double u = -((x1 - x3) * (y1 - y2) - (y1 - y3) * (x1 - x2)) / denom;
                
                if (t >= 0.0 && t <= 1.0 && u >= 0.0 && u <= 1.0) {
                    Point2D intersection = {x1 + t * (x2 - x1), y1 + t * (y2 - y1)};
                    intersections.push_back({t, intersection});
                }
            }
            
            if (intersections.size() < 2) return;
            
            std::sort(intersections.begin(), intersections.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });
            
            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                Point2D mid = {(intersections[i].second.x + intersections[i+1].second.x) / 2.0,
                            (intersections[i].second.y + intersections[i+1].second.y) / 2.0};
                
                if (isPointInPolygon(mid)) {
                    // GUARDAMOS EL SEGMENTO EN EL CACHÉ EN LUGAR DE DIBUJARLO
                    cachedSegments.push_back({intersections[i].second, intersections[i+1].second});
                }
            }
        };

        // Generar líneas
        for (double offset = -diagLen * 2.0; offset <= diagLen * 2.0; offset += realSpacing) {
            double perpX = -sinA * offset;
            double perpY =  cosA * offset;
            double lineLength = diagLen * 3.0;
            
            Point2D lp1 = {cx + cosA * (-lineLength) + perpX, cy + sinA * (-lineLength) + perpY};
            Point2D lp2 = {cx + cosA * (lineLength) + perpX, cy + sinA * (lineLength) + perpY};
            
            clipLineToPolygon(lp1, lp2);
        }
        
        isGeometryDirty = false; // Marcamos como limpio
    }

    bool Hatch::isPointInPolygon(const Point2D& p) const {
        bool inside = false;
        for (size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
            if (((points[i].y > p.y) != (points[j].y > p.y)) &&
                (p.x < (points[j].x - points[i].x) * (p.y - points[i].y) / (points[j].y - points[i].y) + points[i].x)) {
                inside = !inside;
            }
        }
        return inside;
    }
    
    bool Hatch::isNear(const Point2D& point, double tolerance) const {
        // Primero comprobamos si está dentro del polígono
        if (isPointInPolygon(point)) return true;
        
        // Si no, comprobamos si está cerca de algún borde (opcional, para seleccionar el contorno)
        for (size_t i = 0; i < points.size(); ++i) {
            size_t j = (i + 1) % points.size();
            // Distancia punto a segmento (simplificada aquí como distancia a los puntos)
            double d1 = std::hypot(point.x - points[i].x, point.y - points[i].y);
            if (d1 < tolerance) return true;
        }
        return false;
    }

    void Hatch::move(double dx, double dy) {
        for (auto& p : points) { p.x += dx; p.y += dy; }
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
    }

    void Hatch::scale(const Point2D& basePoint, double factor) {
        for (auto& p : points) {
            p.x = basePoint.x + (p.x - basePoint.x) * factor;
            p.y = basePoint.y + (p.y - basePoint.y) * factor;
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
    }

    std::vector<Point2D> Hatch::getGripPoints() const { return points; }
    std::vector<Point2D> Hatch::getSnapPoints() const { return points; }
    
    void Hatch::moveGrip(int index, const Point2D& newPos) {
        if (index >= 0 && index < points.size()) points[index] = newPos;
    }

    void Hatch::update(const Document& doc) {
        if (id.empty()) {
            //std::cout << "[HATCH UPDATE] id está vacío. Hatch libre." << std::endl;
            return; 
        }// No tiene borde asociado, es un hatch "libre"
        //std::cout << "[HATCH UPDATE] Buscando entidad con ID: " << id << std::endl;
        //std::cout << "[HATCH UPDATE] Mi propio ID es: " << id << std::endl;
        // 1. Buscar la entidad original por su ID
        for (const auto& entity : doc.entities) {
            //std::cout << "[HATCH UPDATE] Revisando entidad con ID: " << entity->id 
            //      << " (tipo: " << typeid(*entity).name() << ")" << std::endl;

            if (entity->id == boundaryId) {
                //std::cout << "[HATCH UPDATE] ¡ENCONTRADA! Actualizando puntos..." << std::endl;
            
                // 2. Si la encontramos, extraemos sus puntos actualizados
                std::vector<Point2D> newPoints;
                
                if (auto* poly = dynamic_cast<Polyline*>(entity.get())) {
                    //std::cout << "[HATCH UPDATE] Es una Polyline. Cerrada: " << poly->closed 
                    //      << ", puntos: " << poly->points.size() << std::endl;

                    if (poly->closed && poly->points.size() >= 3) newPoints = poly->points;
                }
                else if (auto* polygon = dynamic_cast<Polygon*>(entity.get())) {
                    //std::cout << "[HATCH UPDATE] Es un Polygon." << std::endl;
                    //constexpr double PI = 3.14159265358979323846;
                    int sides = polygon->sides;
                    double angleStep = 2 * PI / sides;
                    double offset = polygon->rotationOffset * PI / 180.0;
                    for (int i = 0; i < sides; ++i) {
                        double angle = i * angleStep + offset;
                        newPoints.push_back({polygon->center.x + polygon->radius * std::cos(angle),
                                             polygon->center.y + polygon->radius * std::sin(angle)});
                    }
                }
                else if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                    // std::cout << "[HATCH UPDATE] Es un Circle." << std::endl;
                    //constexpr double PI = 3.14159265358979323846;
                    int segments = 32;
                    double angleStep = 2 * PI / segments;
                    for (int i = 0; i < segments; ++i) {
                        double angle = i * angleStep;
                        newPoints.push_back({circle->center.x + circle->radius * std::cos(angle),
                                             circle->center.y + circle->radius * std::sin(angle)});
                    }
                }

                // 3. Si la extracción fue válida, actualizamos los puntos del hatch
                if (newPoints.size() >= 3) {
                    //std::cout << "[HATCH UPDATE] Actualizando " << newPoints.size() << " puntos." << std::endl;
                    points = std::move(newPoints);
                    isGeometryDirty = true;
                } else {
                    //std::cout << "[HATCH UPDATE] No se pudieron extraer puntos válidos." << std::endl;
                }
                return; // ¡Actualización completada! Salimos.
            }
        }
        //std::cout << "[HATCH UPDATE] No se encontró la entidad con ID: " << id << std::endl;
        // Si el bucle termina sin encontrar la entidad, significa que el usuario 
        // borró el borde original. El hatch se queda con sus últimos puntos conocidos 
        // (o podrías añadir una lógica para borrarlo o marcarlo como "huérfano").
    }


    void Hatch::copyFrom(const Entity& src) {
        const Hatch* h = dynamic_cast<const Hatch*>(&src);
        if (h) {
            layerName = h->layerName;
            id = h->id; // Copiar el mismo ID, o generar uno nuevo si es necesario
            points = h->points;
            pattern = h->pattern;
            patternScale = h->patternScale;
            angle = h->angle;      // >>> NUEVO
            spacing = h->spacing;  // >>> NUEVO
        }
    }

    nlohmann::json Hatch::toJson() const {
        nlohmann::json j;
        j["type"] = "Hatch";
        j["id"] = id;
        j["layer"] = layerName;
        j["pattern"] = static_cast<int>(pattern);
        j["patternScale"] = patternScale;
        j["angle"] = angle;      // >>> NUEVO
        j["spacing"] = spacing;  // >>> NUEVO
        j["points"] = nlohmann::json::array();
        for (const auto& p : points) {
            j["points"].push_back({{"x", p.x}, {"y", p.y}});
        }
        return j;
    }

    std::unique_ptr<Hatch> Hatch::fromJson(const nlohmann::json& j) {
        auto h = std::make_unique<Hatch>();
        h->layerName = j.value("layer", "0");
        h->id = j.value("id", Entity::generateId());  // Generar un ID único si no existe
        h->pattern = static_cast<HatchPattern>(j.value("pattern", 1));
        h->patternScale = j.value("patternScale", 1.0);
        h->angle = j.value("angle", 0.0);      // >>> NUEVO
        h->spacing = j.value("spacing", 1.0);  // >>> NUEVO
        if (j.contains("points")) {
            for (const auto& p : j["points"]) {
                h->points.push_back({p["x"].get<double>(), p["y"].get<double>()});
            }
        }
        return h;
    }

} // namespace cad