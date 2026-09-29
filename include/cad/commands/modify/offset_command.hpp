#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp" 

namespace cad {

class OffsetCommand : public ICommand {
public:
    OffsetCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "DESPLAZAR"; }

    // Para feedback visual
    double getDistance() const { return distance_; }
    bool hasDistance() const { return hasDistance_; }
    Entity* getSelectedEntity() const { return selectedEntity_; }

private:
    enum class Step { 
        WaitingDistance, 
        WaitingEntity, 
        WaitingSide 
    };
    Step step_ = Step::WaitingDistance;
    double distance_ = 0.0;
    Entity* selectedEntity_ = nullptr;
    Point2D firstPoint_;  // Para calcular distancia con dos puntos
    
    bool hasDistance_ = false;
    bool finished_ = false;
    std::string statusMessage_;

    // Método auxiliar para crear entidad desplazada
    void createOffsetEntity(Entity* entity, const Point2D& sidePoint, Engine& engine);
};

} // namespace cad