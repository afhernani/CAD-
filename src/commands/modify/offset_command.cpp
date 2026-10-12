#include "cad/commands/modify/offset_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/polyline.hpp"
#include "cad/core/geometry/intersections.hpp" 
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;
    }
    
    //1. INICIALIZACIÓN DE LA VARIABLE ESTÁTICA (Valor por defecto real)
    double OffsetCommand::distance_ = 10.0;

    // Helper para formatear números bonito (ej: "10.00")
    std::string formatDouble(double val) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << val;
        return oss.str();
    }

    OffsetCommand::OffsetCommand() {
        // Asumimos que distance_ se inicializa a 10.0 en el .hpp
        statusMessage_ = "DESPLAZAR | Especificar distancia <" + formatDouble(distance_) + "> o [Puntos]:";
    }

    void OffsetCommand::execute(const std::string& input, Engine& engine) {
        
        // ============================================================
        // PASO 1: MANEJAR "ENTER" (VALOR POR DEFECTO)
        // ============================================================
        if (input.empty()) {
            if (step_ == Step::WaitingDistance) {
                // El usuario pulsó Enter sin escribir nada. 
                // Usamos la distancia que ya tiene la variable (default o último valor usado).
                hasDistance_ = true;
                step_ = Step::WaitingEntity;
                statusMessage_ = "DESPLAZAR | Usando distancia <" + formatDouble(distance_) + ">. Seleccionar entidad:";
                return;
            }
            else if (step_ == Step::WaitingDistancePoint2) {
                // El usuario empezó a medir por puntos pero se arrepintió y pulsó Enter.
                // Volvemos a pedir distancia numérica.
                step_ = Step::WaitingDistance;
                statusMessage_ = "DESPLAZAR | Especificar distancia <" + formatDouble(distance_) + "> o [Puntos]:";
                return;
            }
            else if (step_ == Step::WaitingEntity || step_ == Step::WaitingSide) {
                // Si ya tenemos la entidad y el usuario pulsa Enter, termina el comando (estándar CAD).
                finished_ = true;
                statusMessage_ = "DESPLAZAR | Comando terminado.";
                return;
            }
        }

        // ============================================================
        // PASO 2: Intentar parsear como coordenada "x,y" 
        // (Útil si el usuario quiere definir el punto de distancia por teclado)
        // ============================================================
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // ============================================================
        // PASO 3: Intentar parsear como número (distancia por teclado)
        // ============================================================
        if (step_ == Step::WaitingDistance) {
            try {
                double value = std::stod(input);
                if (value >= 0) {
                    distance_ = value; // Actualizamos el valor por defecto para la próxima vez
                    hasDistance_ = true;
                    step_ = Step::WaitingEntity;
                    statusMessage_ = "DESPLAZAR | Seleccionar entidad a desplazar:";
                    return; 
                } else {
                    statusMessage_ = "DESPLAZAR | La distancia debe ser >= 0. Nueva distancia <" + formatDouble(distance_) + ">:";
                    return;
                }
            } catch (...) {
                // Si no es un número, caemos al mensaje de error final
            }
        }
        
        // Si llegamos aquí, el formato no fue reconocido
        statusMessage_ = "DESPLAZAR | Formato no válido. Usa un número, coordenadas x,y, o pulsa Enter para usar <" + formatDouble(distance_) + ">.";
    }

    void OffsetCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingDistance) {
            // Primer punto para calcular distancia por ratón
            firstPoint_ = point;
            step_ = Step::WaitingDistancePoint2;
            statusMessage_ = "DESPLAZAR | Segundo punto para definir distancia:";
        }
        else if (step_ == Step::WaitingDistancePoint2) {
            // Segundo punto: calcular distancia geométrica
            double dx = point.x - firstPoint_.x;
            double dy = point.y - firstPoint_.y;
            distance_ = std::hypot(dx, dy); // Más limpio que sqrt(dx*dx + dy*dy)
            hasDistance_ = true;
            step_ = Step::WaitingEntity;
            statusMessage_ = "DESPLAZAR | Seleccionar entidad a desplazar:";
        }
        else if (step_ == Step::WaitingEntity) {
            // Buscar entidad cercana al punto
            double tolerance = 10.0 / engine.viewScale;
            Entity* found = nullptr;
            
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) ||
                        dynamic_cast<Circle*>(entity.get()) ||
                        dynamic_cast<Arc*>(entity.get())||
                        dynamic_cast<Polyline*>(entity.get())) {
                        found = entity.get();
                        break;
                    }
                }
            }
            
            if (found) {
                selectedEntity_ = found;
                step_ = Step::WaitingSide;
                statusMessage_ = "DESPLAZAR | Indicar lado del desplazamiento:";
            } else {
                statusMessage_ = "DESPLAZAR | No se encontró entidad válida. Intenta de nuevo:";
            }
        }
        else if (step_ == Step::WaitingSide) {
            // Crear entidad desplazada
            createOffsetEntity(selectedEntity_, point, engine);
            
            // Resetear para permitir múltiples desplazamientos con la misma distancia
            selectedEntity_ = nullptr;
            step_ = Step::WaitingEntity;
            statusMessage_ = "DESPLAZAR | Seleccionar otra entidad o Enter para terminar:";
        }
    }

    void OffsetCommand::createOffsetEntity(Entity* entity, const Point2D& sidePoint, Engine& engine) {
        engine.saveState();
        
        if (auto* line = dynamic_cast<Line*>(entity)) {
            double dx = line->p2.x - line->p1.x;
            double dy = line->p2.y - line->p1.y;
            double len = std::sqrt(dx * dx + dy * dy);
            
            if (len > 0) {
                double nx = -dy / len;
                double ny = dx / len;
                
                double vx = sidePoint.x - line->p1.x;
                double vy = sidePoint.y - line->p1.y;
                double side = vx * nx + vy * ny;
                double sign = (side >= 0) ? 1.0 : -1.0;
                
                auto newLine = std::make_unique<Line>();
                newLine->id = Entity::generateId(); // Generar un nuevo ID único
                newLine->p1 = {line->p1.x + nx * distance_ * sign, line->p1.y + ny * distance_ * sign};
                newLine->p2 = {line->p2.x + nx * distance_ * sign, line->p2.y + ny * distance_ * sign};
                newLine->layerName = engine.doc.currentLayerName;
                engine.doc.addEntity(std::move(newLine));
            }
        }
        else if (auto* circle = dynamic_cast<Circle*>(entity)) {
            auto newCircle = std::make_unique<Circle>();
            newCircle->id = Entity::generateId(); // Generar un nuevo ID único
            newCircle->center = circle->center;
            newCircle->radius = circle->radius + distance_;
            if (newCircle->radius < 0) newCircle->radius = std::abs(newCircle->radius);
            newCircle->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newCircle));
        }
        else if (auto* arc = dynamic_cast<Arc*>(entity)) {
            auto newArc = std::make_unique<Arc>();
            newArc->id = Entity::generateId(); // Generar un nuevo ID único
            newArc->center = arc->center;
            newArc->radius = arc->radius + distance_;
            newArc->startAngle = arc->startAngle;
            newArc->endAngle = arc->endAngle;
            if (newArc->radius < 0) newArc->radius = std::abs(newArc->radius);
            newArc->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newArc));
        }
        else if (auto* polyline = dynamic_cast<Polyline*>(entity)) {
            const auto& pts = polyline->points;
            if (pts.size() < 2) return;

            std::vector<Point2D> newPoints;
            std::vector<std::pair<Point2D, Point2D>> shiftedSegments;

            // 1. Determinar el lado del offset usando el PRIMER segmento válido
            double offsetSign = 1.0;
            for (size_t i = 0; i < pts.size() - 1; ++i) {
                double dx = pts[i+1].x - pts[i].x;
                double dy = pts[i+1].y - pts[i].y;
                double len = std::hypot(dx, dy);
                if (len < 0.001) continue;

                // Normal del segmento
                double nx = -dy / len;
                double ny = dx / len;
                
                // Punto medio del segmento
                double midX = (pts[i].x + pts[i+1].x) / 2.0;
                double midY = (pts[i].y + pts[i+1].y) / 2.0;
                
                // Vector del punto medio al punto de clic
                double toClickX = sidePoint.x - midX;
                double toClickY = sidePoint.y - midY;
                
                // Producto punto: si es positivo, el clic está en la dirección de la normal
                double dot = toClickX * nx + toClickY * ny;
                offsetSign = (dot >= 0) ? 1.0 : -1.0;
                break; // Solo necesitamos el primer segmento
            }

            // 2. Desplazar todos los segmentos
            for (size_t i = 0; i < pts.size() - 1; ++i) {
                double dx = pts[i+1].x - pts[i].x;
                double dy = pts[i+1].y - pts[i].y;
                double len = std::hypot(dx, dy);
                if (len < 0.001) continue;

                double nx = -dy / len;
                double ny = dx / len;

                Point2D A_shift = {pts[i].x + nx * distance_ * offsetSign, pts[i].y + ny * distance_ * offsetSign};
                Point2D B_shift = {pts[i+1].x + nx * distance_ * offsetSign, pts[i+1].y + ny * distance_ * offsetSign};
                shiftedSegments.push_back({A_shift, B_shift});
            }

            // 3. Calcular nuevas esquinas
            if (polyline->closed && shiftedSegments.size() > 1) {
                // Polilínea cerrada: añadir segmento de cierre
                double dx = pts[0].x - pts.back().x;
                double dy = pts[0].y - pts.back().y;
                double len = std::hypot(dx, dy);
                if (len > 0.001) {
                    double nx = -dy / len;
                    double ny = dx / len;
                    Point2D A_shift = {pts.back().x + nx * distance_ * offsetSign, pts.back().y + ny * distance_ * offsetSign};
                    Point2D B_shift = {pts[0].x + nx * distance_ * offsetSign, pts[0].y + ny * distance_ * offsetSign};
                    shiftedSegments.push_back({A_shift, B_shift});
                }

                // Todos los vértices son intersecciones
                for (size_t i = 0; i < shiftedSegments.size(); ++i) {
                    size_t prevIdx = (i == 0) ? shiftedSegments.size() - 1 : i - 1;
                    
                    auto inter = lineLineIntersection(
                        shiftedSegments[prevIdx].first, shiftedSegments[prevIdx].second,
                        shiftedSegments[i].first, shiftedSegments[i].second
                    );
                    
                    if (inter.intersects) {
                        newPoints.push_back(inter.point);
                    } else {
                        newPoints.push_back(shiftedSegments[i].first);
                    }
                }
            } else {
                // Polilínea abierta
                if (!shiftedSegments.empty()) {
                    newPoints.push_back(shiftedSegments.front().first);
                    
                    for (size_t i = 1; i < shiftedSegments.size(); ++i) {
                        auto inter = lineLineIntersection(
                            shiftedSegments[i-1].first, shiftedSegments[i-1].second,
                            shiftedSegments[i].first, shiftedSegments[i].second
                        );
                        
                        if (inter.intersects) {
                            newPoints.push_back(inter.point);
                        } else {
                            newPoints.push_back(shiftedSegments[i].first);
                        }
                    }
                    
                    newPoints.push_back(shiftedSegments.back().second);
                }
            }

            // 4. Crear y añadir la nueva entidad
            if (newPoints.size() >= 2) {
                auto newPoly = std::make_unique<Polyline>();
                newPoly->id = Entity::generateId();
                newPoly->points = newPoints;
                newPoly->closed = polyline->closed;
                newPoly->layerName = engine.doc.currentLayerName;
                engine.doc.addEntity(std::move(newPoly));
            }
        }
    }

    void OffsetCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string OffsetCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool OffsetCommand::isComplete() const {
        return finished_;
    }

    void OffsetCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        
        // Lambda para convertir coordenadas. 
        // NOTA: Si ya quitaste el parámetro CANVAS_HEIGHT de tu View::worldToScreen, 
        // cámbialo a: return view.worldToScreen(x, y);
        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. RESALTAR ENTIDAD BAJO EL CURSOR (cuando esperamos seleccionar)
        if (step_ == Step::WaitingEntity && hasDistance_) {
            double tolerance = 10.0 / view.getScale();
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(mouseWorldPos, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) ||
                        dynamic_cast<Circle*>(entity.get()) ||
                        dynamic_cast<Arc*>(entity.get())) {
                        // Dibujar la entidad en verde brillante para indicar que se puede seleccionar
                        entity->draw(window, w2s, sf::Color(0, 255, 0, 150), view.getScale());
                        break;
                    }
                }
            }
        }

        // 2. DIBUJAR PREVIEW DEL DESPLAZAMIENTO (cuando ya tenemos entidad seleccionada)
        if (hasDistance_ && selectedEntity_) {
            sf::Color previewColor(255, 255, 0, 150); // Amarillo translúcido
            Entity* entity = selectedEntity_;
            double distance = distance_;

            if (auto* line = dynamic_cast<Line*>(entity)) {
                double dx = line->p2.x - line->p1.x;
                double dy = line->p2.y - line->p1.y;
                double len = std::sqrt(dx * dx + dy * dy);
                if (len > 0) {
                    double nx = -dy / len;
                    double ny = dx / len;
                    
                    // >>> CORRECCIÓN CLAVE: Calcular el lado basado en la posición del ratón <<<
                    double vx = mouseWorldPos.x - line->p1.x;
                    double vy = mouseWorldPos.y - line->p1.y;
                    double side = vx * nx + vy * ny;
                    double sign = (side >= 0) ? 1.0 : -1.0;
                    
                    sf::Vertex linePreview[] = { 
                        sf::Vertex(w2s(line->p1.x + nx * distance * sign, line->p1.y + ny * distance * sign), previewColor),
                        sf::Vertex(w2s(line->p2.x + nx * distance * sign, line->p2.y + ny * distance * sign), previewColor) 
                    };
                    window.draw(linePreview, 2, sf::Lines);
                }
            }
            else if (auto* circle = dynamic_cast<Circle*>(entity)) {
                double newRadius = circle->radius + distance;
                if (newRadius < 0) newRadius = std::abs(newRadius);
                
                sf::CircleShape circlePreview(static_cast<float>(newRadius * view.getScale()));
                circlePreview.setFillColor(sf::Color::Transparent);
                circlePreview.setOutlineColor(previewColor);
                circlePreview.setOutlineThickness(1.5f);
                circlePreview.setOrigin(static_cast<float>(newRadius * view.getScale()),
                                       static_cast<float>(newRadius * view.getScale()));
                circlePreview.setPosition(w2s(circle->center.x, circle->center.y));
                window.draw(circlePreview);
            }
            else if (auto* arc = dynamic_cast<Arc*>(entity)) {
                double newRadius = arc->radius + distance;
                if (newRadius < 0) newRadius = std::abs(newRadius);
                
                const int numPoints = 64;
                sf::VertexArray va(sf::LineStrip, numPoints);
                double startRad = arc->startAngle * PI / 180.0;
                double endRad = arc->endAngle * PI / 180.0;
                double step = (endRad - startRad) / (numPoints - 1);
                
                for (int i = 0; i < numPoints; ++i) {
                    double angle = startRad + i * step;
                    va[i].position = w2s(arc->center.x + newRadius * std::cos(angle), arc->center.y + newRadius * std::sin(angle));
                    va[i].color = previewColor;
                }
                window.draw(va);
            }
            else if (auto* polyline = dynamic_cast<Polyline*>(entity)) {
                const auto& pts = polyline->points;
                if (pts.size() < 2) return;

                // 1. Calcular el signo del offset (misma lógica que en createOffsetEntity)
                double offsetSign = 1.0;
                for (size_t i = 0; i < pts.size() - 1; ++i) {
                    double dx = pts[i+1].x - pts[i].x;
                    double dy = pts[i+1].y - pts[i].y;
                    double len = std::hypot(dx, dy);
                    if (len < 0.001) continue;

                    double nx = -dy / len;
                    double ny = dx / len;
                    double midX = (pts[i].x + pts[i+1].x) / 2.0;
                    double midY = (pts[i].y + pts[i+1].y) / 2.0;
                    
                    double dot = (mouseWorldPos.x - midX) * nx + (mouseWorldPos.y - midY) * ny;
                    offsetSign = (dot >= 0) ? 1.0 : -1.0;
                    break;
                }

                // 2. Desplazar todos los segmentos
                std::vector<std::pair<Point2D, Point2D>> shiftedSegments;
                for (size_t i = 0; i < pts.size() - 1; ++i) {
                    double dx = pts[i+1].x - pts[i].x;
                    double dy = pts[i+1].y - pts[i].y;
                    double len = std::hypot(dx, dy);
                    if (len < 0.001) continue;

                    double nx = -dy / len;
                    double ny = dx / len;

                    Point2D A_shift = {pts[i].x + nx * distance * offsetSign, pts[i].y + ny * distance * offsetSign};
                    Point2D B_shift = {pts[i+1].x + nx * distance * offsetSign, pts[i+1].y + ny * distance * offsetSign};
                    shiftedSegments.push_back({A_shift, B_shift});
                }

                // 3. Calcular los puntos del preview (con intersecciones en las esquinas)
                std::vector<Point2D> previewPoints;
                
                if (polyline->closed && shiftedSegments.size() > 1) {
                    // Añadir segmento de cierre
                    double dx = pts[0].x - pts.back().x;
                    double dy = pts[0].y - pts.back().y;
                    double len = std::hypot(dx, dy);
                    if (len > 0.001) {
                        double nx = -dy / len;
                        double ny = dx / len;
                        Point2D A_shift = {pts.back().x + nx * distance * offsetSign, pts.back().y + ny * distance * offsetSign};
                        Point2D B_shift = {pts[0].x + nx * distance * offsetSign, pts[0].y + ny * distance * offsetSign};
                        shiftedSegments.push_back({A_shift, B_shift});
                    }

                    // Calcular intersecciones para todos los vértices
                    for (size_t i = 0; i < shiftedSegments.size(); ++i) {
                        size_t prevIdx = (i == 0) ? shiftedSegments.size() - 1 : i - 1;
                        
                        auto inter = lineLineIntersection(
                            shiftedSegments[prevIdx].first, shiftedSegments[prevIdx].second,
                            shiftedSegments[i].first, shiftedSegments[i].second
                        );
                        
                        if (inter.intersects) {
                            previewPoints.push_back(inter.point);
                        } else {
                            previewPoints.push_back(shiftedSegments[i].first);
                        }
                    }
                } else {
                    // Polilínea abierta
                    if (!shiftedSegments.empty()) {
                        previewPoints.push_back(shiftedSegments.front().first);
                        
                        for (size_t i = 1; i < shiftedSegments.size(); ++i) {
                            auto inter = lineLineIntersection(
                                shiftedSegments[i-1].first, shiftedSegments[i-1].second,
                                shiftedSegments[i].first, shiftedSegments[i].second
                            );
                            
                            if (inter.intersects) {
                                previewPoints.push_back(inter.point);
                            } else {
                                previewPoints.push_back(shiftedSegments[i].first);
                            }
                        }
                        
                        previewPoints.push_back(shiftedSegments.back().second);
                    }
                }

                // 4. Dibujar el preview
                if (previewPoints.size() >= 2) {
                    sf::VertexArray previewVA(sf::LineStrip, previewPoints.size());
                    for (size_t i = 0; i < previewPoints.size(); ++i) {
                        previewVA[i].position = w2s(previewPoints[i].x, previewPoints[i].y);
                        previewVA[i].color = previewColor;
                    }
                    window.draw(previewVA);
                }
            }
        }
    }

} // namespace cad