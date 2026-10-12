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

            // 1. Determinar el lado global del desplazamiento
            double globalSign = 1.0;
            for (size_t i = 0; i < pts.size(); ++i) {
                size_t nextIdx = (i + 1) % pts.size(); // Para polilíneas cerradas, el último conecta con el primero
                if (!polyline->closed && nextIdx >= pts.size()) break;
                
                double dx = pts[nextIdx].x - pts[i].x;
                double dy = pts[nextIdx].y - pts[i].y;
                double len = std::hypot(dx, dy);
                if (len < 0.001) continue;

                double nx = -dy / len;
                double ny = dx / len;
                double midX = (pts[i].x + pts[nextIdx].x) / 2.0;
                double midY = (pts[i].y + pts[nextIdx].y) / 2.0;
                
                double dot = (sidePoint.x - midX) * nx + (sidePoint.y - midY) * ny;
                globalSign = (dot >= 0) ? 1.0 : -1.0;
                break;
            }

            // 2. Desplazar TODOS los segmentos (incluyendo el de cierre si es cerrada)
            for (size_t i = 0; i < pts.size(); ++i) {
                size_t nextIdx = (i + 1) % pts.size();
                if (!polyline->closed && nextIdx >= pts.size()) break;
                
                double dx = pts[nextIdx].x - pts[i].x;
                double dy = pts[nextIdx].y - pts[i].y;
                double len = std::hypot(dx, dy);
                if (len < 0.001) continue;

                double nx = -dy / len;
                double ny = dx / len;

                Point2D A_shift = {pts[i].x + nx * distance_ * globalSign, pts[i].y + ny * distance_ * globalSign};
                Point2D B_shift = {pts[nextIdx].x + nx * distance_ * globalSign, pts[nextIdx].y + ny * distance_ * globalSign};
                shiftedSegments.push_back({A_shift, B_shift});
            }

            // 3. Calcular nuevas esquinas (intersecciones de segmentos adyacentes)
            // Para polilíneas cerradas: cada vértice es la intersección del segmento i-1 con el segmento i
            for (size_t i = 0; i < shiftedSegments.size(); ++i) {
                size_t prevIdx = (i == 0) ? shiftedSegments.size() - 1 : i - 1;
                
                auto inter = lineLineIntersection(
                    shiftedSegments[prevIdx].first, shiftedSegments[prevIdx].second,
                    shiftedSegments[i].first, shiftedSegments[i].second
                );
                
                if (inter.intersects) {
                    newPoints.push_back(inter.point);
                } else {
                    // Fallback: si son paralelos, usar el punto inicial del segmento
                    newPoints.push_back(shiftedSegments[i].first);
                }
            }

            // 4. Para polilíneas ABIERTAS: añadir el último punto sin intersección
            if (!polyline->closed && !shiftedSegments.empty()) {
                // El primer punto ya está (es el inicio del primer segmento)
                // Añadimos el último punto (fin del último segmento)
                newPoints.insert(newPoints.begin(), shiftedSegments.front().first);
            }

            // 5. Crear y añadir la nueva entidad
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
                if (pts.size() >= 2) {
                    // Reutilizamos la lógica de cálculo de lado para el preview
                    double globalSign = 1.0;
                    double dx0 = pts[1].x - pts[0].x;
                    double dy0 = pts[1].y - pts[0].y;
                    double len0 = std::hypot(dx0, dy0);
                    if (len0 > 0.001) {
                        double nx0 = -dy0 / len0;
                        double ny0 = dx0 / len0;
                        double midX = (pts[0].x + pts[1].x) / 2.0;
                        double midY = (pts[0].y + pts[1].y) / 2.0;
                        double dot = (mouseWorldPos.x - midX) * nx0 + (mouseWorldPos.y - midY) * ny0;
                        globalSign = (dot >= 0) ? 1.0 : -1.0;
                    }

                    sf::VertexArray previewVA(sf::LineStrip, pts.size());
                    for (size_t i = 0; i < pts.size(); ++i) {
                        // Nota: Para un preview perfecto en esquinas, idealmente calcularíamos las intersecciones como en createOffsetEntity.
                        // Para mantener el código simple y rápido en el renderizado, desplazamos los puntos originales directamente.
                        // En polilíneas simples se ve muy bien.
                        double px = pts[i].x;
                        double py = pts[i].y;
                        
                        // Aplicar desplazamiento aproximado al punto (suficiente para feedback visual)
                        if (i > 0) {
                            double dx = pts[i].x - pts[i-1].x;
                            double dy = pts[i].y - pts[i-1].y;
                            double len = std::hypot(dx, dy);
                            if (len > 0.001) {
                                px += (-dy / len) * distance * globalSign;
                                py += (dx / len) * distance * globalSign;
                            }
                        }
                        previewVA[i].position = w2s(px, py);
                        previewVA[i].color = previewColor;
                    }
                    window.draw(previewVA);
                }
            }
        }
    }

} // namespace cad