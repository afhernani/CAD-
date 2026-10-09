#include "cad/commands/draw/dimension_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include "cad/core/constants.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace cad {

    // >>> CONSTRUCTOR PREDETERMINADO (Cota Alineada por defecto)
    DimensionCommand::DimensionCommand() : type_(DimType::ALIGNED) {
        statusMessage_ = "ACOTAR | Especificar primer punto de extensión:";
    }

    DimensionCommand::DimensionCommand(DimType type) : type_(type) {
        if (type_ == DimType::RADIUS || type_ == DimType::DIAMETER) {
            step_ = Step::WaitingEntity;
            statusMessage_ = (type_ == DimType::RADIUS) ? 
                "COTA RADIO | Seleccionar arco o círculo:" : 
                "COTA DIÁMETRO | Seleccionar arco o círculo:";
        } else {
            step_ = Step::WaitingP1;
            std::string typeName = (type_ == DimType::HORIZONTAL) ? "HORIZONTAL" : 
                                   (type_ == DimType::VERTICAL) ? "VERTICAL" : "ALINEADA";
            statusMessage_ = "COTA " + typeName + " | Especificar primer punto de extensión:";
        }
    }

    std::string DimensionCommand::getName() const {
        if (type_ == DimType::HORIZONTAL) return "COTA HORIZONTAL";
        if (type_ == DimType::VERTICAL) return "COTA VERTICAL";
        if (type_ == DimType::ALIGNED) return "COTA ALINEADA";
        if (type_ == DimType::RADIUS) return "COTA RADIO";
        if (type_ == DimType::DIAMETER) return "COTA DIÁMETRO";
        return "COTA";
    }

    void DimensionCommand::execute(const std::string& input, Engine& engine) {
        std::istringstream iss(input);
        double x, y; char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            onPoint({x, y}, engine);
        } else {
            statusMessage_ = "Formato inválido. Usa: x,y o haz clic.";
        }
    }

    void DimensionCommand::onPoint(const Point2D& point, Engine& engine) {
        // --- LÓGICA PARA HORIZONTAL, VERTICAL, ALIGNED ---
        if (type_ == DimType::HORIZONTAL || type_ == DimType::VERTICAL || type_ == DimType::ALIGNED) {
            if (step_ == Step::WaitingP1) {
                p1_ = point; hasP1_ = true; step_ = Step::WaitingP2;
                statusMessage_ = "Especificar segundo punto de extensión:";
            }
            else if (step_ == Step::WaitingP2) {
                p2_ = point; hasP2_ = true; step_ = Step::WaitingLocation;
                statusMessage_ = "Ubicar línea de cota:";
            }
            else if (step_ == Step::WaitingLocation) {
                double val = std::hypot(p2_.x - p1_.x, p2_.y - p1_.y);
                auto dim = std::make_unique<Dimension>();
                dim->id = Entity::generateId(); // Generar un ID único
                dim->type = type_;
                dim->p1 = p1_; dim->p2 = p2_; dim->location = point;
                dim->value = val;
                dim->isAligned = (type_ == DimType::ALIGNED);
                dim->isHorizontal = (type_ == DimType::HORIZONTAL);
                dim->layerName = engine.doc.currentLayerName;
                
                engine.saveState();
                engine.doc.addEntity(std::move(dim));
                finished_ = true;
                statusMessage_ = "Cota creada. (Enter para terminar)";
            }
        }
        // --- LÓGICA PARA RADIO Y DIÁMETRO ---
        else if (type_ == DimType::RADIUS || type_ == DimType::DIAMETER) {
            if (step_ == Step::WaitingEntity) {
                double tolerance = 10.0 / engine.viewScale;
                for (auto& entity : engine.doc.entities) {
                    if ((dynamic_cast<Circle*>(entity.get()) || dynamic_cast<Arc*>(entity.get())) && 
                        entity->isNear(point, tolerance)) {
                        selectedCircleOrArc_ = entity.get();
                        step_ = Step::WaitingEntityLocation;
                        statusMessage_ = "Ubicar línea de cota:";
                        return;
                    }
                }
                statusMessage_ = "No se encontró arco o círculo. Intenta de nuevo:";
            }
            else if (step_ == Step::WaitingEntityLocation) {
                auto dim = std::make_unique<Dimension>();
                dim->type = type_;
                // Asumimos que la entidad tiene un método getCenter() o lo calculamos
                Point2D center = {0,0}; double radius = 0;
                if (auto* c = dynamic_cast<Circle*>(selectedCircleOrArc_)) { center = c->center; radius = c->radius; }
                else if (auto* a = dynamic_cast<Arc*>(selectedCircleOrArc_)) { center = a->center; radius = a->radius; }
                dim->id = Entity::generateId(); // Generar un ID único
                dim->p1 = center; 
                dim->p2 = point; // El punto donde se hace clic define la dirección de la línea
                dim->location = point;
                dim->value = (type_ == DimType::RADIUS) ? radius : radius * 2.0;
                dim->layerName = engine.doc.currentLayerName;
                
                engine.saveState();
                engine.doc.addEntity(std::move(dim));
                finished_ = true;
                statusMessage_ = "Cota creada. (Enter para terminar)";
            }
        }
    }

    void DimensionCommand::onCancel() { finished_ = true; statusMessage_ = "Comando cancelado."; }
    std::string DimensionCommand::getStatusMessage() const { return statusMessage_; }
    bool DimensionCommand::isComplete() const { return finished_; }

    void DimensionCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                        const Point2D& mouseWorldPos, sf::Font& font) const {
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };
        sf::Color previewColor(255, 255, 0, 180);

        if (type_ == DimType::HORIZONTAL || type_ == DimType::VERTICAL || type_ == DimType::ALIGNED) {
            if (hasP1_ && !hasP2_) {
                sf::Vertex line[] = { sf::Vertex(w2s(p1_.x, p1_.y), previewColor), sf::Vertex(w2s(mouseWorldPos.x, mouseWorldPos.y), previewColor) };
                window.draw(line, 2, sf::Lines);
            }
            else if (hasP1_ && hasP2_ && !finished_) {
                Dimension tempDim;
                tempDim.type = type_; tempDim.p1 = p1_; tempDim.p2 = p2_; tempDim.location = mouseWorldPos;
                tempDim.value = std::hypot(p2_.x - p1_.x, p2_.y - p1_.y);
                tempDim.isAligned = (type_ == DimType::ALIGNED);
                tempDim.isHorizontal = (type_ == DimType::HORIZONTAL);
                tempDim.draw(window, w2s, previewColor, view.getScale()); // ¡Reutiliza el dibujo de la entidad!
            }
        }
        else if ((type_ == DimType::RADIUS || type_ == DimType::DIAMETER) && selectedCircleOrArc_ != nullptr && !finished_) {
            Point2D center = {0,0}; double radius = 0;
            if (auto* c = dynamic_cast<Circle*>(selectedCircleOrArc_)) { center = c->center; radius = c->radius; }
            else if (auto* a = dynamic_cast<Arc*>(selectedCircleOrArc_)) { center = a->center; radius = a->radius; }

            Dimension tempDim;
            tempDim.type = type_;
            tempDim.p1 = center;
            tempDim.p2 = mouseWorldPos;
            tempDim.location = mouseWorldPos;
            tempDim.value = (type_ == DimType::RADIUS) ? radius : radius * 2.0;
            tempDim.draw(window, w2s, previewColor, view.getScale());
        }
    }

} // namespace cad