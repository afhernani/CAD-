#include "cad/commands/modify/array_command.hpp"
#include "cad/commands/engine.hpp"
#include <sstream>
#include <cmath>
#include <numbers>

namespace cad {

ArrayCommand::ArrayCommand() {
    statusMessage_ = "ARRAY | Seleccionar entidades (Enter para terminar):";
}

void ArrayCommand::execute(const std::string& input, Engine& engine) {
    // 1. Intentar parsear como coordenada "x,y" (para clics o centro polar)
    std::istringstream iss(input);
    double x, y;
    char comma;
    if (iss >> x >> comma >> y && comma == ',') {
        Point2D p{x, y};
        onPoint(p, engine);
        return;
    }

    // 2. Input vacío (Enter)
    if (input.empty()) {
        if (step_ == Step::SelectingEntities) {
            if (selectedEntities_.empty()) {
                statusMessage_ = "ARRAY | No hay entidades seleccionadas. Comando cancelado.";
                finished_ = true;
            } else {
                step_ = Step::ChoosingType;
                statusMessage_ = "ARRAY | Tipo [Rectangular(R)/Polar(P)] <R>:";
            }
        }
        return;
    }

    // 3. Parsear tipo de array (R o P)
    std::string upperInput = input;
    std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
    if (step_ == Step::ChoosingType) {
        if (upperInput == "R" || upperInput == "RECTANGULAR") {
            arrayType_ = Type::Rectangular;
            step_ = Step::RectRows;
            statusMessage_ = "ARRAY RECTANGULAR | Número de filas <" + std::to_string(rows_) + ">:";
        } else if (upperInput == "P" || upperInput == "POLAR") {
            arrayType_ = Type::Polar;
            step_ = Step::PolarCenter;
            statusMessage_ = "ARRAY POLAR | Punto base (centro):";
        } else {
            statusMessage_ = "ARRAY | Tipo no válido. Usa R o P:";
        }
        return;
    }

    // 4. Parsear números para los parámetros
    try {
        double value = std::stod(input);
        
        if (step_ == Step::RectRows) {
            rows_ = std::max(1, (int)value);
            step_ = Step::RectCols;
            statusMessage_ = "ARRAY RECTANGULAR | Número de columnas <" + std::to_string(cols_) + ">:";
        }
        else if (step_ == Step::RectCols) {
            cols_ = std::max(1, (int)value);
            step_ = Step::RectRowSpacing;
            statusMessage_ = "ARRAY RECTANGULAR | Espaciado entre filas:";
        }
        else if (step_ == Step::RectRowSpacing) {
            rowSpacing_ = value;
            step_ = Step::RectColSpacing;
            statusMessage_ = "ARRAY RECTANGULAR | Espaciado entre columnas:";
        }
        else if (step_ == Step::RectColSpacing) {
            colSpacing_ = value;
            createArray(engine);
            finished_ = true;
        }
        else if (step_ == Step::PolarCount) {
            polarCount_ = std::max(1, (int)value);
            step_ = Step::PolarAngle;
            statusMessage_ = "ARRAY POLAR | Ángulo total a rellenar (360=círculo completo):";
        }
        else if (step_ == Step::PolarAngle) {
            polarAngle_ = value;
            createArray(engine);
            finished_ = true;
        }
        else {
            statusMessage_ = "ARRAY | Valor no válido en este paso.";
        }
    } catch (...) {
        statusMessage_ = "ARRAY | Formato no válido. Usa un número.";
    }
}

void ArrayCommand::onPoint(const Point2D& point, Engine& engine) {
    double tolerance = 10.0 / engine.viewScale;

    if (step_ == Step::SelectingEntities) {
        // Buscar entidad cercana
        Entity* found = nullptr;
        for (auto& entity : engine.doc.entities) {
            if (entity->isNear(point, tolerance)) {
                found = entity.get();
                break;
            }
        }
        
        if (found) {
            // Evitar duplicados
            if (std::find(selectedEntities_.begin(), selectedEntities_.end(), found) == selectedEntities_.end()) {
                selectedEntities_.push_back(found);
            }
            statusMessage_ = "ARRAY | Entidad añadida (" + 
                           std::to_string(selectedEntities_.size()) + 
                           "). Enter para terminar:";
        } else {
            statusMessage_ = "ARRAY | No se encontró entidad. Intenta de nuevo:";
        }
    }
    else if (step_ == Step::PolarCenter) {
        polarCenter_ = point;
        hasPolarCenter_ = true;
        step_ = Step::PolarCount;
        statusMessage_ = "ARRAY POLAR | Número de elementos <" + std::to_string(polarCount_) + ">:";
    }
}

void ArrayCommand::createArray(Engine& engine) {
    if (selectedEntities_.empty()) return;
    
    engine.saveState();
    
    if (arrayType_ == Type::Rectangular) {
        for (int r = 0; r < rows_; ++r) {
            for (int c = 0; c < cols_; ++c) {
                if (r == 0 && c == 0) continue; // No duplicar la original
                
                double dx = c * colSpacing_;
                double dy = r * rowSpacing_;
                
                for (Entity* original : selectedEntities_) {
                    auto clone = original->clone();
                    clone->move(dx, dy);
                    clone->layerName = engine.doc.currentLayerName;
                    engine.doc.addEntity(std::move(clone));
                }
            }
        }
        statusMessage_ = "ARRAY | Matriz rectangular creada (" + 
                        std::to_string(rows_) + "x" + std::to_string(cols_) + ").";
    }
    else if (arrayType_ == Type::Polar && hasPolarCenter_) {
        double angleStep = polarAngle_ / polarCount_;
        if (polarAngle_ == 360.0) angleStep = 360.0 / polarCount_;
        
        for (int i = 1; i < polarCount_; ++i) {
            double angle = i * angleStep;
            
            for (Entity* original : selectedEntities_) {
                auto clone = original->clone();
                clone->rotate(polarCenter_, angle);
                clone->layerName = engine.doc.currentLayerName;
                engine.doc.addEntity(std::move(clone));
            }
        }
        statusMessage_ = "ARRAY | Matriz polar creada (" + 
                        std::to_string(polarCount_) + " elementos, " + 
                        std::to_string(polarAngle_) + "°).";
    }
}

void ArrayCommand::onCancel() {
    finished_ = true;
    statusMessage_ = "ARRAY | Comando cancelado.";
}

std::string ArrayCommand::getStatusMessage() const {
    return statusMessage_;
}

bool ArrayCommand::isComplete() const {
    return finished_;
}

} // namespace cad