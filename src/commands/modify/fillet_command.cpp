#include "cad/commands/modify/fillet_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/core/geometry/entities/line.hpp"
#include "cad/core/geometry/entities/arc.hpp"
#include "cad/core/geometry/entities/circle.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include <sstream>
#include <cmath>
#include <algorithm>

namespace cad {

    namespace {
        constexpr double PI = 3.14159265358979323846;

        // Helper: Normalizar ángulo a [0, 2*PI)
        double normalizeAngle(double angle) {
            while (angle < 0.0) angle += 2.0 * PI;
            while (angle >= 2.0 * PI) angle -= 2.0 * PI;
            return angle;
        }

        // Helper: Obtener el punto final de un arco (inicio o fin) más cercano al click
        Point2D getClosestArcEndpoint(const Arc* arc, const Point2D& clickPt) {
            double sRad = arc->startAngle * PI / 180.0;
            double eRad = arc->endAngle * PI / 180.0;
            Point2D pStart = {arc->center.x + arc->radius * std::cos(sRad), arc->center.y + arc->radius * std::sin(sRad)};
            Point2D pEnd = {arc->center.x + arc->radius * std::cos(eRad), arc->center.y + arc->radius * std::sin(eRad)};
            
            double dStart = std::hypot(pStart.x - clickPt.x, pStart.y - clickPt.y);
            double dEnd = std::hypot(pEnd.x - clickPt.x, pEnd.y - clickPt.y);
            return (dStart < dEnd) ? pStart : pEnd;
        }
    }

    FilletCommand::FilletCommand() {
        statusMessage_ = "EMPALME | Especificar radio (0 para esquina viva):";
    }

    void FilletCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input);
        double x, y; char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        try {
            double value = std::stod(input);
            if (value >= 0) {
                radius_ = value;
                hasRadius_ = true;
                step_ = Step::WaitingEntity1;
                statusMessage_ = "EMPALME | Seleccionar primera entidad (Línea o Arco):";
            } else {
                statusMessage_ = "EMPALME | Radio no válido. Debe ser >= 0.";
            }
        } catch (...) {
            statusMessage_ = "EMPALME | Formato no válido. Usa un número.";
        }
    }

    void FilletCommand::onPoint(const Point2D& point, Engine& engine) {
        double tolerance = 10.0 / engine.viewScale;
        
        if (step_ == Step::WaitingEntity1) {
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) || dynamic_cast<Arc*>(entity.get()) || dynamic_cast<Circle*>(entity.get())) {
                        entity1_ = entity.get();
                        lastClickPoint_ = point;
                        step_ = Step::WaitingEntity2;
                        statusMessage_ = "EMPALME | Seleccionar segunda entidad:";
                        return;
                    }
                }
            }
            statusMessage_ = "EMPALME | Entidad no válida. Usa Línea o Arco.";
        }
        else if (step_ == Step::WaitingEntity2) {
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance) && entity.get() != entity1_) {
                    if (dynamic_cast<Line*>(entity.get()) || dynamic_cast<Arc*>(entity.get()) || dynamic_cast<Circle*>(entity.get())) {
                        entity2_ = entity.get();
                        lastClickPoint_ = point;
                        createFillet(engine, point);
                        
                        // Reset para múltiples empalmes
                        entity1_ = nullptr;
                        entity2_ = nullptr;
                        step_ = Step::WaitingEntity1;
                        statusMessage_ = "EMPALME | Seleccionar primera entidad (o ESC para terminar):";
                        return;
                    }
                }
            }
            statusMessage_ = "EMPALME | Entidad no válida o es la misma que la primera.";
        }
    }

    void FilletCommand::createFillet(Engine& engine, const Point2D& clickPoint) {
        if (!entity1_ || !entity2_) return;

        // CASO 0: Radio 0 (Solo recortar/alargar hasta la intersección)
        if (radius_ < 1e-5) {
            // (Implementación simplificada: delegamos en la lógica de Trim/Extend o mostramos mensaje)
            statusMessage_ = "EMPALME | Radio 0 no soportado aún para Arcos. Usa un radio > 0.";
            return;
        }

        engine.saveState();

        // --- LÓGICA UNIVERSAL BASADA EN OFFSETS ---
        Point2D filletCenter{0, 0};
        Point2D t1{0, 0}, t2{0, 0};
        bool success = false;

        auto isLine = [](Entity* e) { return dynamic_cast<Line*>(e) != nullptr; };
        auto isArc = [](Entity* e) { return dynamic_cast<Arc*>(e) != nullptr || dynamic_cast<Circle*>(e) != nullptr; };

        // 1. LINEA - LINEA
        if (isLine(entity1_) && isLine(entity2_)) {
            auto* l1 = dynamic_cast<Line*>(entity1_);
            auto* l2 = dynamic_cast<Line*>(entity2_);
            auto inter = lineLineIntersection(l1->p1, l1->p2, l2->p1, l2->p2);
            if (inter.intersects) {
                Point2D I = inter.point;
                auto normalize = [](Point2D a, Point2D b) {
                    double dx = b.x - a.x, dy = b.y - a.y;
                    double len = std::hypot(dx, dy);
                    return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0};
                };
                Point2D end1 = (std::hypot(l1->p1.x - I.x, l1->p1.y - I.y) < std::hypot(l1->p2.x - I.x, l1->p2.y - I.y)) ? l1->p1 : l1->p2;
                Point2D end2 = (std::hypot(l2->p1.x - I.x, l2->p1.y - I.y) < std::hypot(l2->p2.x - I.x, l2->p2.y - I.y)) ? l2->p1 : l2->p2;
                
                Point2D v1 = normalize(I, end1);
                Point2D v2 = normalize(I, end2);
                double cosAngle = std::clamp(v1.x * v2.x + v1.y * v2.y, -1.0, 1.0);
                double angle = std::acos(cosAngle);
                
                if (angle > 0.001) {
                    double d = radius_ / std::tan(angle / 2.0);
                    t1 = {I.x + v1.x * d, I.y + v1.y * d};
                    t2 = {I.x + v2.x * d, I.y + v2.y * d};
                    
                    Point2D bisector = {v1.x + v2.x, v1.y + v2.y};
                    double bisLen = std::hypot(bisector.x, bisector.y);
                    if (bisLen > 0) {
                        bisector.x /= bisLen; bisector.y /= bisLen;
                        double h = radius_ / std::sin(angle / 2.0);
                        filletCenter = {I.x + bisector.x * h, I.y + bisector.y * h};
                        success = true;
                    }
                }
            }
        }
                // 2. LINEA - ARCO (o Círculo)
        else if ((isLine(entity1_) && isArc(entity2_)) || (isArc(entity1_) && isLine(entity2_))) {
            auto* line = isLine(entity1_) ? dynamic_cast<Line*>(entity1_) : dynamic_cast<Line*>(entity2_);
            auto* arc = isArc(entity1_) ? dynamic_cast<Arc*>(entity1_) : dynamic_cast<Arc*>(entity2_);
            
            double arcRadius = arc->radius;
            Point2D arcCenter = arc->center;

            // Calcular vector normal a la línea
            double dx = line->p2.x - line->p1.x;
            double dy = line->p2.y - line->p1.y;
            double len = std::hypot(dx, dy);
            double nx = -dy / len, ny = dx / len; 
            
            // Determinar el lado del desplazamiento basado en el punto de clic
            double cross = (clickPoint.x - line->p1.x) * nx + (clickPoint.y - line->p1.y) * ny;
            double sign = (cross >= 0) ? 1.0 : -1.0;
            
            // ✅ CORRECCIÓN: Usar Point2D directamente en lugar de instanciar un objeto Line
            Point2D offsetP1 = {line->p1.x + sign * radius_ * nx, line->p1.y + sign * radius_ * ny};
            Point2D offsetP2 = {line->p2.x + sign * radius_ * nx, line->p2.y + sign * radius_ * ny};

            // Intersección de la línea offset con el círculo offset (radio_arco + radio_fillet)
            auto intersections = lineCircleIntersection(offsetP1, offsetP2, arcCenter, arcRadius + radius_);
            
            // Elegir la intersección más cercana al punto de clic
            double minDist = std::numeric_limits<double>::max();
            for (const auto& pt : intersections) {
                double dist = std::hypot(pt.x - clickPoint.x, pt.y - clickPoint.y);
                if (dist < minDist) {
                    minDist = dist;
                    filletCenter = pt;
                }
            }

            if (minDist < std::numeric_limits<double>::max()) {
                // Tangente en la línea: proyección del centro del fillet sobre la línea original
                double t = ((filletCenter.x - line->p1.x) * dx + (filletCenter.y - line->p1.y) * dy) / (len * len);
                t1 = {line->p1.x + t * dx, line->p1.y + t * dy};
                
                // Tangente en el arco: línea desde el centro del arco hasta el centro del fillet
                double dirX = filletCenter.x - arcCenter.x;
                double dirY = filletCenter.y - arcCenter.y;
                double dirLen = std::hypot(dirX, dirY);
                t2 = {arcCenter.x + (dirX / dirLen) * arcRadius, arcCenter.y + (dirY / dirLen) * arcRadius};
                success = true;
            }
        }
        // 3. ARCO - ARCO (o Círculo)
        else if (isArc(entity1_) && isArc(entity2_)) {
            auto* a1 = dynamic_cast<Arc*>(entity1_);
            auto* a2 = dynamic_cast<Arc*>(entity2_);
            
            auto intersections = circleCircleIntersection(a1->center, a1->radius + radius_, a2->center, a2->radius + radius_);
            
            double minDist = std::numeric_limits<double>::max();
            for (const auto& pt : intersections) {
                double dir1X = pt.x - a1->center.x, dir1Y = pt.y - a1->center.y;
                double dir1Len = std::hypot(dir1X, dir1Y);
                Point2D tempT1 = {a1->center.x + (dir1X / dir1Len) * a1->radius, a1->center.y + (dir1Y / dir1Len) * a1->radius};
                
                double dir2X = pt.x - a2->center.x, dir2Y = pt.y - a2->center.y;
                double dir2Len = std::hypot(dir2X, dir2Y);
                Point2D tempT2 = {a2->center.x + (dir2X / dir2Len) * a2->radius, a2->center.y + (dir2Y / dir2Len) * a2->radius};
                
                double distToClick = std::hypot((tempT1.x + tempT2.x)/2.0 - clickPoint.x, (tempT1.y + tempT2.y)/2.0 - clickPoint.y);
                if (distToClick < minDist) {
                    minDist = distToClick;
                    filletCenter = pt;
                    t1 = tempT1;
                    t2 = tempT2;
                }
            }
            if (minDist < std::numeric_limits<double>::max()) success = true;
        }

        if (!success) {
            statusMessage_ = "EMPALME | No se pudo calcular el empalme (geometría inválida o radio demasiado grande).";
            return;
        }

        // --- APLICAR CAMBIOS AL DOCUMENTO ---
        
        // 1. Actualizar Entidad 1
        if (auto* l = dynamic_cast<Line*>(entity1_)) {
            double d1 = std::hypot(l->p1.x - clickPoint.x, l->p1.y - clickPoint.y);
            double d2 = std::hypot(l->p2.x - clickPoint.x, l->p2.y - clickPoint.y);
            if (d1 < d2) l->p1 = t1; else l->p2 = t1;
        } else if (auto* a = dynamic_cast<Arc*>(entity1_)) {
            double dStart = std::hypot(a->center.x + a->radius * std::cos(a->startAngle * PI / 180.0) - clickPoint.x,
                                       a->center.y + a->radius * std::sin(a->startAngle * PI / 180.0) - clickPoint.y);
            double dEnd = std::hypot(a->center.x + a->radius * std::cos(a->endAngle * PI / 180.0) - clickPoint.x,
                                     a->center.y + a->radius * std::sin(a->endAngle * PI / 180.0) - clickPoint.y);
            double tAngle = std::atan2(t1.y - a->center.y, t1.x - a->center.x) * 180.0 / PI;
            if (tAngle < 0) tAngle += 360.0;
            if (dStart < dEnd) a->startAngle = tAngle; else a->endAngle = tAngle;
        }

        // 2. Actualizar Entidad 2
        if (auto* l = dynamic_cast<Line*>(entity2_)) {
            double d1 = std::hypot(l->p1.x - clickPoint.x, l->p1.y - clickPoint.y);
            double d2 = std::hypot(l->p2.x - clickPoint.x, l->p2.y - clickPoint.y);
            if (d1 < d2) l->p1 = t2; else l->p2 = t2;
        } else if (auto* a = dynamic_cast<Arc*>(entity2_)) {
            double dStart = std::hypot(a->center.x + a->radius * std::cos(a->startAngle * PI / 180.0) - clickPoint.x,
                                       a->center.y + a->radius * std::sin(a->startAngle * PI / 180.0) - clickPoint.y);
            double dEnd = std::hypot(a->center.x + a->radius * std::cos(a->endAngle * PI / 180.0) - clickPoint.x,
                                     a->center.y + a->radius * std::sin(a->endAngle * PI / 180.0) - clickPoint.y);
            double tAngle = std::atan2(t2.y - a->center.y, t2.x - a->center.x) * 180.0 / PI;
            if (tAngle < 0) tAngle += 360.0;
            if (dStart < dEnd) a->startAngle = tAngle; else a->endAngle = tAngle;
        }

        // 3. Crear el Arco de Empalme
        double a1Rad = std::atan2(t1.y - filletCenter.y, t1.x - filletCenter.x);
        double a2Rad = std::atan2(t2.y - filletCenter.y, t2.x - filletCenter.x);
        
        // Asegurar sentido antihorario
        double diff = a2Rad - a1Rad;
        while (diff < 0) diff += 2.0 * PI;
        while (diff >= 2.0 * PI) diff -= 2.0 * PI;
        if (diff > PI) std::swap(a1Rad, a2Rad); // Si es > 180°, invertimos para hacer el empalme corto

        auto newArc = std::make_unique<Arc>();
        newArc->id = Entity::generateId();
        newArc->center = filletCenter;
        newArc->radius = radius_;
        newArc->startAngle = a1Rad * 180.0 / PI;
        newArc->endAngle = a2Rad * 180.0 / PI;
        newArc->layerName = entity1_->layerName; // Heredar capa
        
        engine.doc.addEntity(std::move(newArc));
        statusMessage_ = "EMPALME | Empalme creado correctamente.";
    }

    void FilletCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "EMPALME | Comando cancelado.";
    }

    std::string FilletCommand::getStatusMessage() const { return statusMessage_; }
    bool FilletCommand::isComplete() const { return finished_; }

    void FilletCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!entity1_) return;

        sf::Color highlightColor(0, 255, 0, 150);
        sf::Color previewColor(255, 255, 0, 200);
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // Dibujar entidad 1 resaltada
        if (auto* l = dynamic_cast<Line*>(entity1_)) {
            sf::Vertex verts[] = { sf::Vertex(w2s(l->p1.x, l->p1.y), highlightColor), sf::Vertex(w2s(l->p2.x, l->p2.y), highlightColor) };
            window.draw(verts, 2, sf::Lines);
        } else if (auto* a = dynamic_cast<Arc*>(entity1_)) {
            // (Podrías añadir el dibujo del arco aquí si lo deseas para el feedback)
        }

        // Intentar calcular el empalme en tiempo real para el feedback
        if (hasRadius_ && radius_ > 0) {
            // Simulamos una segunda entidad bajo el ratón
            Entity* hoverEntity = nullptr;
            double tolerance = 10.0 / view.getScale();
            for (auto& entity : engine.doc.entities) {
                if (entity.get() != entity1_ && entity->isNear(mouseWorldPos, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) || dynamic_cast<Arc*>(entity.get())) {
                        hoverEntity = entity.get();
                        break;
                    }
                }
            }

            if (hoverEntity) {
                // Copiamos la lógica de cálculo de forma simplificada solo para obtener el centro y dibujar
                // (En una app de producción, esto se refactorizaría en una función 'calculateFillet' pura)
                Point2D tempCenter{0,0}, tempT1{0,0}, tempT2{0,0};
                bool tempSuccess = false;
                
                // ... (Aquí iría una versión simplificada de la lógica de createFillet que no modifica el documento) ...
                // Por brevedad, si es Línea-Línea, dibujamos el arco de preview:
                if (auto* l1 = dynamic_cast<Line*>(entity1_)) {
                    if (auto* l2 = dynamic_cast<Line*>(hoverEntity)) {
                        auto inter = lineLineIntersection(l1->p1, l1->p2, l2->p1, l2->p2);
                        if (inter.intersects) {
                            Point2D I = inter.point;
                            auto normalize = [](Point2D a, Point2D b) {
                                double dx = b.x - a.x, dy = b.y - a.y;
                                double len = std::hypot(dx, dy);
                                return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0};
                            };
                            Point2D end1 = (std::hypot(l1->p1.x - I.x, l1->p1.y - I.y) < std::hypot(l1->p2.x - I.x, l1->p2.y - I.y)) ? l1->p1 : l1->p2;
                            Point2D end2 = (std::hypot(l2->p1.x - I.x, l2->p1.y - I.y) < std::hypot(l2->p2.x - I.x, l2->p2.y - I.y)) ? l2->p1 : l2->p2;
                            
                            Point2D v1 = normalize(I, end1);
                            Point2D v2 = normalize(I, end2);
                            double cosAngle = std::clamp(v1.x * v2.x + v1.y * v2.y, -1.0, 1.0);
                            double angle = std::acos(cosAngle);
                            
                            if (angle > 0.001) {
                                double d = radius_ / std::tan(angle / 2.0);
                                tempT1 = {I.x + v1.x * d, I.y + v1.y * d};
                                tempT2 = {I.x + v2.x * d, I.y + v2.y * d};
                                Point2D bisector = {v1.x + v2.x, v1.y + v2.y};
                                double bisLen = std::hypot(bisector.x, bisector.y);
                                if (bisLen > 0) {
                                    bisector.x /= bisLen; bisector.y /= bisLen;
                                    double h = radius_ / std::sin(angle / 2.0);
                                    tempCenter = {I.x + bisector.x * h, I.y + bisector.y * h};
                                    tempSuccess = true;
                                }
                            }
                        }
                    }
                }

                if (tempSuccess) {
                    const int numPoints = 32;
                    sf::VertexArray arc(sf::LineStrip, numPoints);
                    double a1 = std::atan2(tempT1.y - tempCenter.y, tempT1.x - tempCenter.x);
                    double a2 = std::atan2(tempT2.y - tempCenter.y, tempT2.x - tempCenter.x);
                    double diff = a2 - a1;
                    while (diff < 0) diff += 2 * PI;
                    while (diff >= 2 * PI) diff -= 2 * PI;
                    if (diff > PI) std::swap(a1, a2);
                    diff = a2 - a1; while (diff < 0) diff += 2 * PI;
                    double step = diff / (numPoints - 1);

                    for (int i = 0; i < numPoints; ++i) {
                        double a = a1 + i * step;
                        arc[i].position = w2s(tempCenter.x + radius_ * std::cos(a), tempCenter.y + radius_ * std::sin(a));
                        arc[i].color = previewColor;
                    }
                    window.draw(arc);
                }
            }
        }
    }

} // namespace cad