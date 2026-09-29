#include "cad/commands/modify/fillet_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include <sstream>
#include <cmath>
#include <numbers>

namespace cad {

    FilletCommand::FilletCommand() {
        statusMessage_ = "EMPALME | Especificar radio (0 para esquina viva):";
    }

    void FilletCommand::execute(const std::string& input, Engine& engine) {
        // 1. Primero intentar parsear como coordenada "x,y" (clic del ratón)
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Si no es coordenada, intentar como número (radio)
        try {
            double value = std::stod(input);
            if (value >= 0) {
                radius_ = value;
                hasRadius_ = true;
                step_ = Step::WaitingLine1;
                statusMessage_ = "EMPALME | Seleccionar primera línea:";
            } else {
                statusMessage_ = "EMPALME | Radio no válido. Debe ser >= 0.";
            }
        } catch (...) {
            statusMessage_ = "EMPALME | Formato no válido. Usa un número.";
        }
    }

    void FilletCommand::onPoint(const Point2D& point, Engine& engine) {
        double tolerance = 10.0 / engine.viewScale;
        
        if (step_ == Step::WaitingLine1) {
            // Buscar línea cercana
            Line* found = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (auto* line = dynamic_cast<Line*>(entity.get())) {
                    if (line->isNear(point, tolerance)) {
                        found = line;
                        break;
                    }
                }
            }
            
            if (found) {
                line1_ = found;
                hasLine1_ = true;
                step_ = Step::WaitingLine2;
                statusMessage_ = "EMPALME | Seleccionar segunda línea:";
            } else {
                statusMessage_ = "EMPALME | No es una línea. Intenta de nuevo:";
            }
        }
        else if (step_ == Step::WaitingLine2) {
            // Buscar segunda línea
            Line* found = nullptr;
            for (auto& entity : engine.doc.entities) {
                if (auto* line = dynamic_cast<Line*>(entity.get())) {
                    if (line->isNear(point, tolerance) && line != line1_) {
                        found = line;
                        break;
                    }
                }
            }
            
            if (found) {
                line2_ = found;
                createFillet(engine);
                
                // Resetear para permitir múltiples empalmes
                line1_ = nullptr;
                line2_ = nullptr;
                hasLine1_ = false;
                step_ = Step::WaitingLine1;
                statusMessage_ = "EMPALME | Seleccionar primera línea (o ESC para terminar):";
            } else {
                statusMessage_ = "EMPALME | No es una línea. Intenta de nuevo:";
            }
        }
    }

    void FilletCommand::createFillet(Engine& engine) {
        if (!line1_ || !line2_) return;
        
        // Calcular intersección
        auto inter = lineLineIntersection(line1_->p1, line1_->p2,
                                        line2_->p1, line2_->p2);
        
        if (!inter.intersects) {
            statusMessage_ = "EMPALME | Líneas paralelas. No se puede crear empalme.";
            return;
        }
        
        engine.saveState();
        
        Point2D I = inter.point;
        
        // Función auxiliar para normalizar
        auto normalize = [](Point2D a, Point2D b) {
            double dx = b.x - a.x, dy = b.y - a.y;
            double len = std::sqrt(dx * dx + dy * dy);
            return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0};
        };
        
        // Determinar qué extremos están más cerca de la intersección
        double d1a = std::hypot(line1_->p1.x - I.x, line1_->p1.y - I.y);
        double d1b = std::hypot(line1_->p2.x - I.x, line1_->p2.y - I.y);
        Point2D& end1 = (d1a < d1b) ? line1_->p1 : line1_->p2;
        
        double d2a = std::hypot(line2_->p1.x - I.x, line2_->p1.y - I.y);
        double d2b = std::hypot(line2_->p2.x - I.x, line2_->p2.y - I.y);
        Point2D& end2 = (d2a < d2b) ? line2_->p1 : line2_->p2;
        
        // Vectores desde I hacia los extremos a recortar
        Point2D v1 = normalize(I, end1);
        Point2D v2 = normalize(I, end2);
        
        // Ángulo entre los vectores
        double cosAngle = v1.x * v2.x + v1.y * v2.y;
        if (cosAngle > 1.0) cosAngle = 1.0;
        if (cosAngle < -1.0) cosAngle = -1.0;
        double angle = std::acos(cosAngle);
        
        if (angle < 0.001) {
            statusMessage_ = "EMPALME | Líneas prácticamente paralelas.";
            return;
        }
        
        // Distancia desde I a los puntos tangentes
        double d = radius_ / std::tan(angle / 2.0);
        
        // Nuevos extremos de las líneas (puntos tangentes)
        Point2D T1 = {I.x + v1.x * d, I.y + v1.y * d};
        Point2D T2 = {I.x + v2.x * d, I.y + v2.y * d};
        
        // Actualizar líneas originales (recortar)
        end1 = T1;
        end2 = T2;
        
        // Dibujar arco si radio > 0
        if (radius_ > 0.001) {
            // Centro del arco: en la bisectriz
            Point2D bisector = {v1.x + v2.x, v1.y + v2.y};
            double bisLen = std::sqrt(bisector.x * bisector.x + bisector.y * bisector.y);
            if (bisLen > 0) {
                bisector.x /= bisLen;
                bisector.y /= bisLen;
                double h = radius_ / std::sin(angle / 2.0);
                Point2D center = {I.x + bisector.x * h, I.y + bisector.y * h};
                
                // Calcular ángulos inicio/fin para el arco
                double a1 = std::atan2(T1.y - center.y, T1.x - center.x);
                double a2 = std::atan2(T2.y - center.y, T2.x - center.x);
                
                // Asegurar que el arco vaya en la dirección correcta (el más corto)
                double diff = a2 - a1;
                while (diff < 0) diff += 2 * std::numbers::pi;
                while (diff >= 2 * std::numbers::pi) diff -= 2 * std::numbers::pi;
                if (diff > std::numbers::pi) std::swap(a1, a2);
                
                auto newArc = std::make_unique<Arc>();
                newArc->center = center;
                newArc->radius = radius_;
                newArc->startAngle = a1 * 180.0 / std::numbers::pi;
                newArc->endAngle = a2 * 180.0 / std::numbers::pi;
                newArc->layerName = engine.doc.currentLayerName;
                engine.doc.addEntity(std::move(newArc));
            }
        }
        
        statusMessage_ = "EMPALME | Empalme creado (radio: " + std::to_string(radius_) + ").";
    }

    void FilletCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string FilletCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool FilletCommand::isComplete() const {
        return finished_;
    }

} // namespace cad