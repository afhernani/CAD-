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

    // =====================================================================
        // Intersección de dos RECTAS INFINITAS (no segmentos)
        // Recta 1: pasa por A y B
        // Recta 2: pasa por C y D
        // Devuelve true si se intersectan (no son paralelas) y el punto en 'out'
        // =====================================================================
        bool intersectLines(const Point2D& A, const Point2D& B,
                            const Point2D& C, const Point2D& D,
                            Point2D& out) {
            // Vectores directores
            double dx1 = B.x - A.x;
            double dy1 = B.y - A.y;
            double dx2 = D.x - C.x;
            double dy2 = D.y - C.y;

            // Producto cruzado de los vectores directores
            double cross = dx1 * dy2 - dy1 * dx2;

            // Si el producto cruzado es ~0, las rectas son paralelas (o coincidentes)
            if (std::abs(cross) < 1e-9) {
                return false;
            }

            // Resolver el sistema:
            // A + t*(B-A) = C + s*(D-C)
            // 
            // t = ((C.x - A.x)*dy2 - (C.y - A.y)*dx2) / cross
            
            double t = ((C.x - A.x) * dy2 - (C.y - A.y) * dx2) / cross;

            // El punto de intersección (pertenece a AMBAS rectas)
            out.x = A.x + t * dx1;
            out.y = A.y + t * dy1;

            return true;
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

            engine.saveState();

            // =====================================================================
            // 1. ORIENTACIÓN DE LA POLILÍNEA (área con signo)
            // =====================================================================
            double signedArea = 0.0;
            for (size_t i = 0; i < pts.size(); ++i) {
                size_t j = (i + 1) % pts.size();
                signedArea += pts[i].x * pts[j].y;
                signedArea -= pts[j].x * pts[i].y;
            }
            signedArea *= 0.5;
            bool isCCW = (signedArea > 0);

            // =====================================================================
            // 2. ¿Clic dentro o fuera? (Ray Casting)
            // =====================================================================
            auto isPointInPolygon = [](const Point2D& p, const std::vector<Point2D>& poly) -> bool {
                bool inside = false;
                size_t n = poly.size();
                for (size_t i = 0, j = n - 1; i < n; j = i++) {
                    if (((poly[i].y > p.y) != (poly[j].y > p.y)) &&
                        (p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / 
                            (poly[j].y - poly[i].y) + poly[i].x)) {
                        inside = !inside;
                    }
                }
                return inside;
            };
            bool clickInside = isPointInPolygon(sidePoint, pts);

            // Signo del offset: 1 = lado de la normal izquierda, -1 = lado derecho
            double sign = 1.0;
            if (isCCW) sign = clickInside ? -1.0 : 1.0;   // CCW: normal izq = exterior
            else       sign = clickInside ?  1.0 : -1.0;  // CW:  normal izq = interior

            // =====================================================================
            // 3. CONSTRUIR LAS RECTAS PARALELAS (offset de cada segmento)
            // =====================================================================
            int n = pts.size();
            int numSegs = polyline->closed ? n : n - 1;
            
            // shiftedSegs[i] = recta paralela al segmento i
            std::vector<std::pair<Point2D, Point2D>> shiftedSegs;
            shiftedSegs.reserve(numSegs);

            for (int i = 0; i < numSegs; ++i) {
                int nextIdx = (i + 1) % n;
                if (!polyline->closed && nextIdx >= n) break;

                double dx = pts[nextIdx].x - pts[i].x;
                double dy = pts[nextIdx].y - pts[i].y;
                double len = std::hypot(dx, dy);
                if (len < 1e-6) continue;

                // Normal unitaria "izquierda" del segmento
                double nx = -dy / len;
                double ny =  dx / len;

                Point2D A = {pts[i].x     + nx * distance_ * sign,
                            pts[i].y     + ny * distance_ * sign};
                Point2D B = {pts[nextIdx].x + nx * distance_ * sign,
                            pts[nextIdx].y + ny * distance_ * sign};
                shiftedSegs.push_back({A, B});
            }

            // =====================================================================
            // 4. PARA CADA VÉRTICE: INTERSECTAR LAS DOS RECTAS PARALELAS ADYACENTES
            // =====================================================================
            std::vector<Point2D> newPoints;
            newPoints.reserve(n);

            if (polyline->closed && (int)shiftedSegs.size() == n) {
                // --- CERRADA: todos los vértices son intersecciones ---
                for (int i = 0; i < n; ++i) {
                    int prevIdx = (i - 1 + n) % n;

                    Point2D inter;
                    if (intersectLines(shiftedSegs[prevIdx].first, shiftedSegs[prevIdx].second,
                                    shiftedSegs[i].first,      shiftedSegs[i].second,
                                    inter)) {
                        // Miter limit: si la intersección está muy lejos → bisel
                        double dist = std::hypot(inter.x - pts[i].x, inter.y - pts[i].y);
                        if (dist < distance_ * 4.0) {
                            newPoints.push_back(inter);
                        } else {
                            // Bisel: unir extremos de los segmentos desplazados
                            newPoints.push_back(shiftedSegs[prevIdx].second);
                            newPoints.push_back(shiftedSegs[i].first);
                        }
                    } else {
                        // Rectas paralelas: usar traslación directa
                        newPoints.push_back(shiftedSegs[i].first);
                    }
                }
            }
            else {
                // --- ABIERTA: extremos son traslaciones, medios son intersecciones ---
                if (!shiftedSegs.empty()) {
                    newPoints.push_back(shiftedSegs.front().first);

                    for (size_t i = 1; i < shiftedSegs.size(); ++i) {
                        Point2D inter;
                        if (intersectLines(shiftedSegs[i-1].first, shiftedSegs[i-1].second,
                                        shiftedSegs[i].first,   shiftedSegs[i].second,
                                        inter)) {
                            double dist = std::hypot(inter.x - pts[i].x, inter.y - pts[i].y);
                            if (dist < distance_ * 4.0) {
                                newPoints.push_back(inter);
                            } else {
                                newPoints.push_back(shiftedSegs[i-1].second);
                                newPoints.push_back(shiftedSegs[i].first);
                            }
                        } else {
                            newPoints.push_back(shiftedSegs[i].first);
                        }
                    }
                    newPoints.push_back(shiftedSegs.back().second);
                }
            }

            // =====================================================================
            // 5. CREAR LA NUEVA ENTIDAD
            // =====================================================================
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

                // 1. Orientación
                double signedArea = 0.0;
                for (size_t i = 0; i < pts.size(); ++i) {
                    size_t j = (i + 1) % pts.size();
                    signedArea += pts[i].x * pts[j].y;
                    signedArea -= pts[j].x * pts[i].y;
                }
                bool isCCW = (signedArea * 0.5 > 0);

                // 2. ¿Ratón dentro o fuera?
                auto isPointInPolygon = [](const Point2D& p, const std::vector<Point2D>& poly) -> bool {
                    bool inside = false;
                    size_t n = poly.size();
                    for (size_t i = 0, j = n - 1; i < n; j = i++) {
                        if (((poly[i].y > p.y) != (poly[j].y > p.y)) &&
                            (p.x < (poly[j].x - poly[i].x) * (p.y - poly[i].y) / 
                                (poly[j].y - poly[i].y) + poly[i].x)) {
                            inside = !inside;
                        }
                    }
                    return inside;
                };
                bool clickInside = isPointInPolygon(mouseWorldPos, pts);

                double sign = 1.0;
                if (isCCW) sign = clickInside ? -1.0 : 1.0;
                else       sign = clickInside ?  1.0 : -1.0;

                // 3. Construir rectas paralelas
                std::vector<std::pair<Point2D, Point2D>> shiftedSegs;
                int n = pts.size();
                int numSegs = polyline->closed ? n : n - 1;
                for (int i = 0; i < numSegs; ++i) {
                    int nextIdx = (i + 1) % n;
                    if (!polyline->closed && nextIdx >= n) break;
                    double dx = pts[nextIdx].x - pts[i].x;
                    double dy = pts[nextIdx].y - pts[i].y;
                    double len = std::hypot(dx, dy);
                    if (len < 1e-6) continue;
                    double nx = -dy / len, ny = dx / len;
                    shiftedSegs.push_back({
                        {pts[i].x + nx*distance*sign, pts[i].y + ny*distance*sign},
                        {pts[nextIdx].x + nx*distance*sign, pts[nextIdx].y + ny*distance*sign}
                    });
                }

                // 4. Calcular puntos del preview (intersecciones)
                std::vector<Point2D> previewPoints;
                if (polyline->closed && (int)shiftedSegs.size() == n) {
                    for (int i = 0; i < n; ++i) {
                        int prevIdx = (i - 1 + n) % n;
                        Point2D inter;
                        if (intersectLines(shiftedSegs[prevIdx].first, shiftedSegs[prevIdx].second,
                                        shiftedSegs[i].first, shiftedSegs[i].second, inter)) {
                            double dist = std::hypot(inter.x - pts[i].x, inter.y - pts[i].y);
                            if (dist < distance * 4.0) previewPoints.push_back(inter);
                            else {
                                previewPoints.push_back(shiftedSegs[prevIdx].second);
                                previewPoints.push_back(shiftedSegs[i].first);
                            }
                        } else {
                            previewPoints.push_back(shiftedSegs[i].first);
                        }
                    }
                } else {
                    if (!shiftedSegs.empty()) {
                        previewPoints.push_back(shiftedSegs.front().first);
                        for (size_t i = 1; i < shiftedSegs.size(); ++i) {
                            Point2D inter;
                            if (intersectLines(shiftedSegs[i-1].first, shiftedSegs[i-1].second,
                                            shiftedSegs[i].first, shiftedSegs[i].second, inter)) {
                                double dist = std::hypot(inter.x - pts[i].x, inter.y - pts[i].y);
                                if (dist < distance * 4.0) previewPoints.push_back(inter);
                                else {
                                    previewPoints.push_back(shiftedSegs[i-1].second);
                                    previewPoints.push_back(shiftedSegs[i].first);
                                }
                            } else {
                                previewPoints.push_back(shiftedSegs[i].first);
                            }
                        }
                        previewPoints.push_back(shiftedSegs.back().second);
                    }
                }

                // 5. Dibujar
                if (previewPoints.size() >= 2) {
                    sf::VertexArray va(sf::LineStrip, previewPoints.size());
                    for (size_t i = 0; i < previewPoints.size(); ++i) {
                        va[i].position = w2s(previewPoints[i].x, previewPoints[i].y);
                        va[i].color = previewColor;
                    }
                    window.draw(va);
                }
            }
        }
    }

} // namespace cad