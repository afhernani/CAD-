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
#include <iostream>

namespace cad {
    
    double FilletCommand::savedRadius_ = 0.0;
    
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
        // Helper para formatear números bonito (ej: "10.00")
        std::string formatDouble(double val) {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(2) << val;
            return oss.str();
        }
        // Calcula la intersección de dos rectas infinitas
        // Devuelve true si se cruzan, false si son paralelas.
        bool getInfiniteLineIntersection(const Point2D& p1, const Point2D& p2, 
                                         const Point2D& p3, const Point2D& p4, 
                                         Point2D& outIntersection) {
            // Vectores directores de las rectas
            double dx1 = p2.x - p1.x;
            double dy1 = p2.y - p1.y;
            double dx2 = p4.x - p3.x;
            double dy2 = p4.y - p3.y;

            // Producto cruzado para detectar paralelismo
            double cross = dx1 * dy2 - dy1 * dx2;

            if (std::abs(cross) < 1e-10) {
                return false; // Las rectas son paralelas
            }

            // Calcular el parámetro 't' para encontrar el punto exacto
            double dx3 = p3.x - p1.x;
            double dy3 = p3.y - p1.y;
            double t = (dx3 * dy2 - dy3 * dx2) / cross;

            // Calcular las coordenadas del punto de intersección
            outIntersection = { p1.x + t * dx1, p1.y + t * dy1 };
            return true;
        }
    }

    FilletCommand::FilletCommand() {
        step_ = Step::WaitingRadius;
        statusMessage_ = "EMPALME | Especificar radio <" + formatDouble(savedRadius_) +"> (0 para esquina viva):";
    }

        void FilletCommand::execute(const std::string& input, Engine& engine) {
        // Procesar coordenadas si el usuario las escribe (ej: "10,20")
        std::istringstream iss(input);
        double x, y; char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }
        
        // Si el usuario pulsa ENTER sin escribir nada
        if (input.empty()) {
            if (step_ == Step::WaitingRadius) {
                radius_ = savedRadius_;
                hasRadius_ = true;
                step_ = Step::WaitingEntity1;
                // ✅ LOG PARA VER QUÉ VALOR SE ESTÁ USANDO
                std::cout << "[DEBUG] Enter pulsado. Usando savedRadius_ = " << savedRadius_ << std::endl;
                statusMessage_ = "EMPALME | Seleccione primera entidad (La que sirve de límite):";
            } else {
                finished_ = true;
                statusMessage_ = "EMPALME | Finalizado.";
            }
            return;
        }
        
        // PASO 1: Interpretar el valor del radio
        if (step_ == Step::WaitingRadius) {
            try {
                double value = std::stod(input);
                if (value >= 0) {
                    radius_ = value;
                    savedRadius_ = value; // Guardar para la próxima vez
                    hasRadius_ = true;
                    step_ = Step::WaitingEntity1;
                    // ✅ LOG PARA CONFIRMAR QUE SE GUARDÓ
                    std::cout << "[DEBUG] Número introducido. savedRadius_ actualizado a: " << savedRadius_ << std::endl;
                    statusMessage_ = "EMPALME | Seleccione primera entidad (La que sirve de límite):";
                } else {
                    statusMessage_ = "EMPALME | Radio no válido. Debe ser >= 0.";
                }
            } catch (...) {
                statusMessage_ = "EMPALME | Formato no válido. Introduzca un número o pulse Enter.";
            }
            return;
        }
    }

    void FilletCommand::onPoint(const Point2D& point, Engine& engine) {
        std::cout << "[DEBUG FILLET] onPoint llamado. Step actual: " << static_cast<int>(step_) << std::endl;
        
        double tolerance = 10.0 / engine.viewScale;
        
        if (step_ == Step::WaitingEntity1) {
            std::cout << "[DEBUG FILLET] Buscando Entity1..." << std::endl;
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance)) {
                    if (dynamic_cast<Line*>(entity.get()) || dynamic_cast<Arc*>(entity.get()) || dynamic_cast<Circle*>(entity.get())) {
                        entity1_ = entity.get();
                        lastClickPoint_ = point;
                        step_ = Step::WaitingEntity2;
                        statusMessage_ = "EMPALME | Seleccione entidad(es) a empalmar (Enter para terminar):";
                        std::cout << "[DEBUG FILLET] Entity1 seleccionada con éxito." << std::endl;
                        return;
                    }
                }
            }
            statusMessage_ = "EMPALME | Entidad no válida. Use Línea o Arco.";
            std::cout << "[DEBUG FILLET] Entity1 NO encontrada." << std::endl;
        }
        else if (step_ == Step::WaitingEntity2) {
            std::cout << "[DEBUG FILLET] Buscando Entity2..." << std::endl;
            std::cout << "[DEBUG FILLET] Entity1 es valida? " << (entity1_ != nullptr ? "SI" : "NO") << std::endl;
            
            for (auto& entity : engine.doc.entities) {
                if (entity->isNear(point, tolerance) && entity.get() != entity1_) {
                    if (dynamic_cast<Line*>(entity.get()) || dynamic_cast<Arc*>(entity.get()) || dynamic_cast<Circle*>(entity.get())) {
                        entity2_ = entity.get();
                        lastClickPoint_ = point;
                        std::cout << "[DEBUG FILLET] Entity2 seleccionada. Llamando a createFillet..." << std::endl;
                        
                        createFillet(engine, point);
                        
                        entity2_ = nullptr; 
                        statusMessage_ = "EMPALME | Seleccione siguiente entidad (o Enter para terminar):";
                        std::cout << "[DEBUG FILLET] createFillet terminado. Listo para siguiente." << std::endl;
                        return;
                    }
                }
            }
            statusMessage_ = "EMPALME | Entidad no válida o es la misma que la límite.";
            std::cout << "[DEBUG FILLET] Entity2 NO encontrada." << std::endl;
        }
    }

    void FilletCommand::createFillet(Engine& engine, const Point2D& clickPoint) {
        std::cout << "\n[DEBUG] >>> INICIO DE CREATE_FILLET <<<" << std::endl;
        std::cout << "[DEBUG] Radius actual: " << radius_ << std::endl;

        if (!entity1_ || !entity2_) {
            std::cout << "[DEBUG] SALIDA PREMATURA: entity1 o entity2 son nulas." << std::endl;
            return;
        }

        engine.saveState();
        bool success = false;

        auto* l1 = dynamic_cast<Line*>(entity1_);
        auto* l2 = dynamic_cast<Line*>(entity2_);

        // --- CASO: LÍNEA - LÍNEA ---
        if (l1 && l2) {
            Point2D I;
            if (getInfiniteLineIntersection(l1->p1, l1->p2, l2->p1, l2->p2, I)) {
                
                // ✅ 1. SI EL RADIO ES 0 (ESQUINA VIVA)
                if (radius_ < 1e-5) {
                    // Para cada línea, modificar el extremo MÁS CERCANO a la intersección I
                    double dist1_p1 = std::hypot(l1->p1.x - I.x, l1->p1.y - I.y);
                    double dist1_p2 = std::hypot(l1->p2.x - I.x, l1->p2.y - I.y);
                    if (dist1_p1 < dist1_p2) l1->p1 = I; else l1->p2 = I;

                    double dist2_p1 = std::hypot(l2->p1.x - I.x, l2->p1.y - I.y);
                    double dist2_p2 = std::hypot(l2->p2.x - I.x, l2->p2.y - I.y);
                    if (dist2_p1 < dist2_p2) l2->p1 = I; else l2->p2 = I;
                    
                    success = true;
                    std::cout << "[DEBUG] >>> EMPALME EN ESQUINA VIVA CREADO CON ÉXITO <<<" << std::endl;
                } 
                // ✅ 2. SI EL RADIO ES MAYOR QUE 0 (EMPALME CON ARCO)
                else {
                    auto normalize = [](Point2D a, Point2D b) {
                        double dx = b.x - a.x, dy = b.y - a.y;
                        double len = std::hypot(dx, dy);
                        return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0};
                    };

                    // ✅ LÓGICA CORRECTA: El vector apunta desde I hacia el extremo MÁS CERCANO a I
                    double dist1_p1 = std::hypot(l1->p1.x - I.x, l1->p1.y - I.y);
                    double dist1_p2 = std::hypot(l1->p2.x - I.x, l1->p2.y - I.y);
                    Point2D targetEnd1 = (dist1_p1 < dist1_p2) ? l1->p1 : l1->p2;
                    Point2D v1 = normalize(I, targetEnd1);

                    double dist2_p1 = std::hypot(l2->p1.x - I.x, l2->p1.y - I.y);
                    double dist2_p2 = std::hypot(l2->p2.x - I.x, l2->p2.y - I.y);
                    Point2D targetEnd2 = (dist2_p1 < dist2_p2) ? l2->p1 : l2->p2;
                    Point2D v2 = normalize(I, targetEnd2);

                    double cosAngle = std::clamp(v1.x * v2.x + v1.y * v2.y, -1.0, 1.0);
                    double angle = std::acos(cosAngle);
                    
                    if (angle > 0.01) {
                        double d = radius_ / std::tan(angle / 2.0);
                        Point2D t1 = {I.x + v1.x * d, I.y + v1.y * d};
                        Point2D t2 = {I.x + v2.x * d, I.y + v2.y * d};
                        
                        Point2D bisector = {v1.x + v2.x, v1.y + v2.y};
                        double bisLen = std::hypot(bisector.x, bisector.y);
                        if (bisLen > 0) {
                            bisector.x /= bisLen; bisector.y /= bisLen;
                            double h = radius_ / std::sin(angle / 2.0);
                            Point2D filletCenter = {I.x + bisector.x * h, I.y + bisector.y * h};
                            
                            // ✅ Modificar el extremo más cercano a I
                            if (dist1_p1 < dist1_p2) l1->p1 = t1; else l1->p2 = t1;
                            if (dist2_p1 < dist2_p2) l2->p1 = t2; else l2->p2 = t2;

                            double a1Rad = std::atan2(t1.y - filletCenter.y, t1.x - filletCenter.x);
                            double a2Rad = std::atan2(t2.y - filletCenter.y, t2.x - filletCenter.x);
                            double diff = a2Rad - a1Rad;
                            while (diff < 0) diff += 2.0 * PI;
                            while (diff >= 2.0 * PI) diff -= 2.0 * PI;
                            if (diff > PI) std::swap(a1Rad, a2Rad);

                            auto newArc = std::make_unique<Arc>();
                            newArc->id = Entity::generateId();
                            newArc->center = filletCenter;
                            newArc->radius = radius_;
                            newArc->startAngle = a1Rad * 180.0 / PI;
                            newArc->endAngle = a2Rad * 180.0 / PI;
                            newArc->layerName = entity1_->layerName;
                            
                            engine.doc.addEntity(std::move(newArc));
                            success = true;
                            std::cout << "[DEBUG] >>> EMPALME CON ARCO CREADO CON ÉXITO <<<" << std::endl;
                        }
                    }
                }
            } else {
                std::cout << "[DEBUG] SALIDA: Las rectas son paralelas." << std::endl;
            }
        }

        if (!success) {
            std::cout << "[DEBUG] SALIDA FINAL: success es false." << std::endl;
            statusMessage_ = "EMPALME | No se pudo calcular.";
        } else {
            engine.doc.isModified = true; 
            statusMessage_ = "EMPALME | Empalme creado.";
        }
        
        std::cout << "[DEBUG] >>> FIN DE CREATE_FILLET <<<\n" << std::endl;
    }

    void FilletCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "EMPALME | Comando cancelado.";
    }

    std::string FilletCommand::getStatusMessage() const { return statusMessage_; }

    bool FilletCommand::isComplete() const { return finished_; }

    void FilletCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                     const Point2D& mouseWorldPos, sf::Font& font) const {
        // Si no hay primera entidad seleccionada, no dibujamos nada
        if (!entity1_) return;

        sf::Color highlightColor(0, 255, 0, 150); // Verde semitransparente para resaltar
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };

        // =========================================================================
        // 1. DIBUJAR ENTIDAD 1 RESALTADA
        // =========================================================================
        if (auto* l = dynamic_cast<Line*>(entity1_)) {
            sf::Vertex verts[] = { 
                sf::Vertex(w2s(l->p1.x, l->p1.y), highlightColor), 
                sf::Vertex(w2s(l->p2.x, l->p2.y), highlightColor) 
            };
            window.draw(verts, 2, sf::Lines);
        } 
        else if (auto* a = dynamic_cast<Arc*>(entity1_)) {
            const int numPoints = 32;
            sf::VertexArray arcVerts(sf::LineStrip, numPoints);
            double startRad = a->startAngle * PI / 180.0;
            double endRad = a->endAngle * PI / 180.0;
            double step = (endRad - startRad) / (numPoints - 1);
            for (int i = 0; i < numPoints; ++i) {
                double angle = startRad + i * step;
                arcVerts[i].position = w2s(a->center.x + a->radius * std::cos(angle), a->center.y + a->radius * std::sin(angle));
                arcVerts[i].color = highlightColor;
            }
            window.draw(arcVerts);
        }

        // =========================================================================
        // 2. PREVISUALIZACIÓN DEL EMPALME (ARCO AMARILLO)
        // =========================================================================
        if (hasRadius_ && radius_ > 0) {
            // Buscar entidad bajo el ratón
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
                Point2D tempCenter{0,0}, tempT1{0,0}, tempT2{0,0};
                bool tempSuccess = false;
                
                // --- PREVIEW LÍNEA - LÍNEA ---
                if (auto* l1 = dynamic_cast<Line*>(entity1_)) {
                    if (auto* l2 = dynamic_cast<Line*>(hoverEntity)) {
                        
                        Point2D I;
                        // Usamos rectas infinitas para que la previsualización funcione aunque no se toquen
                        if (getInfiniteLineIntersection(l1->p1, l1->p2, l2->p1, l2->p2, I)) {
                            
                            auto normalize = [](Point2D a, Point2D b) {
                                double dx = b.x - a.x, dy = b.y - a.y;
                                double len = std::hypot(dx, dy);
                                return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0};
                            };

                            // Línea 1: Vector desde I hacia el extremo MÁS CERCANO a I
                            double dist1_p1 = std::hypot(l1->p1.x - I.x, l1->p1.y - I.y);
                            double dist1_p2 = std::hypot(l1->p2.x - I.x, l1->p2.y - I.y);
                            Point2D targetEnd1 = (dist1_p1 < dist1_p2) ? l1->p1 : l1->p2;
                            Point2D v1 = normalize(I, targetEnd1);

                            // Línea 2: Vector desde I hacia el extremo MÁS CERCANO a I
                            double dist2_p1 = std::hypot(l2->p1.x - I.x, l2->p1.y - I.y);
                            double dist2_p2 = std::hypot(l2->p2.x - I.x, l2->p2.y - I.y);
                            Point2D targetEnd2 = (dist2_p1 < dist2_p2) ? l2->p1 : l2->p2;
                            Point2D v2 = normalize(I, targetEnd2);

                            double cosAngle = std::clamp(v1.x * v2.x + v1.y * v2.y, -1.0, 1.0);
                            double angle = std::acos(cosAngle);

                            if (angle > 0.01) { // Evitar líneas casi paralelas
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

                // =========================================================================
                // 3. DIBUJAR EL ARCO DE PREVISUALIZACIÓN
                // =========================================================================
                if (tempSuccess) {
                    const int numPoints = 32;
                    sf::VertexArray arc(sf::LineStrip, numPoints);
                    double a1 = std::atan2(tempT1.y - tempCenter.y, tempT1.x - tempCenter.x);
                    double a2 = std::atan2(tempT2.y - tempCenter.y, tempT2.x - tempCenter.x);
                    
                    double diff = a2 - a1;
                    while (diff < 0) diff += 2 * PI;
                    while (diff >= 2 * PI) diff -= 2 * PI;
                    if (diff > PI) std::swap(a1, a2); // Asegurar el arco más corto
                    
                    diff = a2 - a1; while (diff < 0) diff += 2 * PI;
                    double step = diff / (numPoints - 1);

                    sf::Color previewColor(255, 255, 0, 200); // Amarillo semitransparente
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