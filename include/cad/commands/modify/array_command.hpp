#pragma once
#include "../command.hpp"
#include "../../core/geometry/entity.hpp"
#include <vector>

namespace cad {

class ArrayCommand : public ICommand {
public:
    ArrayCommand();
    
    void execute(const std::string& input, Engine& engine) override;
    void onPoint(const Point2D& point, Engine& engine) override;
    void onCancel() override;
    std::string getStatusMessage() const override;
    bool isComplete() const override;
    std::string getName() const override { return "MATRIZ"; }

    // Getters para feedback visual
    enum class Type { Rectangular, Polar };
    Type getType() const { return arrayType_; }
    const std::vector<Entity*>& getSelectedEntities() const { return selectedEntities_; }
    bool isSelectingEntities() const { return step_ == Step::SelectingEntities; }
    
    // Parámetros Rectangular
    int getRows() const { return rows_; }
    int getCols() const { return cols_; }
    double getRowSpacing() const { return rowSpacing_; }
    double getColSpacing() const { return colSpacing_; }
    
    // Parámetros Polar
    Point2D getPolarCenter() const { return polarCenter_; }
    bool hasPolarCenter() const { return hasPolarCenter_; }
    int getPolarCount() const { return polarCount_; }
    double getPolarAngle() const { return polarAngle_; }

private:
    enum class Step { 
        SelectingEntities, 
        ChoosingType,
        RectRows,
        RectCols,
        RectRowSpacing,
        RectColSpacing,
        PolarCenter,
        PolarCount,
        PolarAngle
    };
    Step step_ = Step::SelectingEntities;
    Type arrayType_ = Type::Rectangular;
    std::vector<Entity*> selectedEntities_;
    
    // Rectangular params
    int rows_ = 2;
    int cols_ = 2;
    double rowSpacing_ = 10.0;
    double colSpacing_ = 10.0;
    
    // Polar params
    Point2D polarCenter_;
    bool hasPolarCenter_ = false;
    int polarCount_ = 4;
    double polarAngle_ = 360.0;
    
    bool finished_ = false;
    std::string statusMessage_;

    void createArray(Engine& engine);
};

} // namespace cad