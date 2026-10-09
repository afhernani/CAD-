#include "cad/commands/modify/chamfer_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/intersections.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT
#include <sstream>
#include <cmath>

namespace cad {

    ChamferCommand::ChamferCommand() {
        statusMessage_ = "CHAFLAN | Primera distancia (0 para esquina viva):";
    }

    void ChamferCommand::execute(const std::string& input, Engine& engine) {
        // 1. Primero intentar parsear como coordenada "x,y" (clic del ratón)
        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            Point2D p{x, y};
            onPoint(p, engine);
            return;
        }

        // 2. Si no es coordenada, intentar como número (distancia)
        try {
            double value = std::stod(input);
            if (value >= 0) {
                if (step_ == Step::WaitingDist1) {
                    dist1_ = value;
                    step_ = Step::WaitingDist2;
                    statusMessage_ = "CHAFLAN | Segunda distancia:";
                }
                else if (step_ == Step::WaitingDist2) {
                    dist2_ = value;
                    step_ = Step::WaitingLine1;
                    statusMessage_ = "CHAFLAN | Primera línea:";
                }
                else {
                    statusMessage_ = "CHAFLAN | Valor no válido en este paso.";
                }
            } else {
                statusMessage_ = "CHAFLAN | Distancia no válida. Debe ser >= 0.";
            }
        } catch (...) {
            statusMessage_ = "CHAFLAN | Formato no válido. Usa un número.";
        }
    }

    void ChamferCommand::onPoint(const Point2D& point, Engine& engine) {
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
                statusMessage_ = "CHAFLAN | Segunda línea:";
            } else {
                statusMessage_ = "CHAFLAN | No es una línea. Intenta de nuevo:";
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
                createChamfer(engine);
                
                // Resetear para permitir múltiples chaflanes
                line1_ = nullptr;
                line2_ = nullptr;
                hasLine1_ = false;
                step_ = Step::WaitingLine1;
                statusMessage_ = "CHAFLAN | Primera línea (o ESC para terminar):";
            } else {
                statusMessage_ = "CHAFLAN | No es una línea. Intenta de nuevo:";
            }
        }
    }

    void ChamferCommand::createChamfer(Engine& engine) {
        if (!line1_ || !line2_) return;
        
        // Calcular intersección
        auto inter = lineLineIntersection(line1_->p1, line1_->p2,
                                        line2_->p1, line2_->p2);
        
        if (!inter.intersects) {
            statusMessage_ = "CHAFLAN | Líneas paralelas. No se puede crear chaflán.";
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
        
        // Puntos tangentes (a dist1 y dist2 desde I)
        Point2D T1 = {I.x + v1.x * dist1_, I.y + v1.y * dist1_};
        Point2D T2 = {I.x + v2.x * dist2_, I.y + v2.y * dist2_};
        
        // Actualizar líneas originales (recortar)
        end1 = T1;
        end2 = T2;
        
        // Crear línea de chaflán si las distancias son > 0
        if (dist1_ > 0.001 || dist2_ > 0.001) {
            auto newLine = std::make_unique<Line>();
            newLine->id = Entity::generateId(); // Generar un nuevo ID único
            newLine->p1 = T1;
            newLine->p2 = T2;
            newLine->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newLine));
        }
        
        statusMessage_ = "CHAFLAN | Chaflán creado.";
    }

    void ChamferCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "Comando cancelado.";
    }

    std::string ChamferCommand::getStatusMessage() const {
        return statusMessage_;
    }

    bool ChamferCommand::isComplete() const {
        return finished_;
    }

    void ChamferCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                      const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasLine1_ || !line1_) return;

        sf::Color highlightColor(0, 255, 0, 150);
        sf::Color previewColor(255, 255, 0, 200);

        auto w2s = [&](double x, double y) {
            return view.worldToScreen(x, y);
        };

        // 1. Dibujar la primera línea seleccionada resaltada
        Line* l1 = line1_;
        sf::Vertex line1Verts[] = {
            sf::Vertex(w2s(l1->p1.x, l1->p1.y), highlightColor),
            sf::Vertex(w2s(l1->p2.x, l1->p2.y), highlightColor)
        };
        window.draw(line1Verts, 2, sf::Lines);

        // 2. Buscar la segunda línea bajo el cursor del ratón
        Line* hoverLine = nullptr;
        double tolerance = 10.0 / view.getScale();
        for (auto& entity : engine.doc.entities) {
            if (auto* line = dynamic_cast<Line*>(entity.get())) {
                if (line->isNear(mouseWorldPos, tolerance) && line != l1) {
                    hoverLine = line;
                    break;
                }
            }
        }

        // 3. Si hay segunda línea y tenemos distancias válidas, calculamos y dibujamos el chaflán
        if (hoverLine && dist1_ > 0.0 && dist2_ > 0.0) {
            auto inter = lineLineIntersection(l1->p1, l1->p2, hoverLine->p1, hoverLine->p2);
            if (inter.intersects) {
                Point2D I = inter.point;
                
                // Lambda para normalizar vectores
                auto normalize = [](Point2D a, Point2D b) {
                    double dx = b.x - a.x, dy = b.y - a.y;
                    double len = std::sqrt(dx * dx + dy * dy);
                    return len > 0 ? Point2D{dx / len, dy / len} : Point2D{0, 0};
                };

                // Encontrar el extremo de la línea 1 más cercano a la intersección
                double d1a = std::hypot(l1->p1.x - I.x, l1->p1.y - I.y);
                double d1b = std::hypot(l1->p2.x - I.x, l1->p2.y - I.y);
                Point2D end1 = (d1a < d1b) ? l1->p1 : l1->p2;

                // Encontrar el extremo de la línea 2 (hover) más cercano a la intersección
                double d2a = std::hypot(hoverLine->p1.x - I.x, hoverLine->p1.y - I.y);
                double d2b = std::hypot(hoverLine->p2.x - I.x, hoverLine->p2.y - I.y);
                Point2D end2 = (d2a < d2b) ? hoverLine->p1 : hoverLine->p2;

                // Vectores unitarios desde la intersección hacia los extremos
                Point2D v1 = normalize(I, end1);
                Point2D v2 = normalize(I, end2);

                // Calcular puntos de corte del chaflán
                Point2D T1 = {I.x + v1.x * dist1_, I.y + v1.y * dist1_};
                Point2D T2 = {I.x + v2.x * dist2_, I.y + v2.y * dist2_};

                // Dibujar la línea de chaflán preview
                sf::Vertex chamferLine[] = {
                    sf::Vertex(w2s(T1.x, T1.y), previewColor),
                    sf::Vertex(w2s(T2.x, T2.y), previewColor)
                };
                window.draw(chamferLine, 2, sf::Lines);
            }
        }
    }

} // namespace cad