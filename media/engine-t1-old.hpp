#pragma once
#include "cad/core/document/document.hpp"
#include "command.hpp"
#include "draw/line_command.hpp"
#include "draw/circle_command.hpp"
#include "draw/arc_command.hpp"
#include "draw/polyline_command.hpp"
#include "draw/polygon_command.hpp"
#include "draw/ellipse_command.hpp"
#include "modify/move_command.hpp"
#include "modify/copy_command.hpp"
#include "modify/rotate_command.hpp"
#include "modify/scale_command.hpp"
#include "modify/mirror_command.hpp"
#include "modify/offset_command.hpp"
#include "modify/fillet_command.hpp"
#include "modify/chamfer_command.hpp"
#include "modify/trim_command.hpp"
#include "modify/extend_command.hpp"
#include "modify/measure_command.hpp"
#include "modify/array_command.hpp"
#include "modify/stretch_command.hpp"
#include "block/block_create_command.hpp"
#include "block/block_insert_command.hpp"
#include "../core/geometry/entities/line.hpp"
#include "../core/geometry/entities/circle.hpp"
#include "../core/geometry/entities/arc.hpp"
#include "../core/geometry/entities/polyline.hpp"
#include "../core/geometry/entities/polygon.hpp"
#include "../core/geometry/entities/ellipse.hpp"
#include "../core/geometry/entities/dimension.hpp"
#include "../core/geometry/entities/block_insert.hpp"
#include "../core/geometry/intersections.hpp" 
#include <string>
#include <optional>    // para manejar valores que pueden no existir
#include <string_view> // c++17/20 para vistars de cadenas sin copia
#include <vector>
#include <memory>

namespace cad {

    enum class Mode {
        IDLE,           // Esperando un comando
        DRAW_LINE,      // Esperando puntos para una línea
        DRAW_CIRCLE,     // Esperando centro y radio
        DRAW_ARC,
        DRAW_POLYLINE,
        DRAW_POLYGON,
        DRAW_ELLIPSE, DRAW_DIMENSION, DIM_OPTIONS, DRAW_DIM_ALIGNED,
        DRAW_DIM_RADIUS, DRAW_DIM_DIAMETER, DRAW_DIM_ANGULAR,
        LAYER_COMMAND, // Nuevo modo para gestionar subcomandos de capa
        COPY,
        ROTATE,
        SCALE,
        MOVE,
        MIRROR,
        MEASURE_DIST,
        TRIM,
        EXTEND,
        UNDO, ARRAY, STRETCH,
        REDO, OFFSET, FILLET, CHAMFER,
        BLOCK_CREATE, BLOCK_INSERT, 
        GRIP_EDIT // << grips editables
    };

    // Nuevo enum auxiliar para el tipo de array:
    enum class ArrayType { RECTANGULAR, POLAR };

    class Engine {
    public:
        Document doc;
        Mode currentMode = Mode::IDLE;
        std::string statusMessage = "Listo";
        
        // Comando activo
        std::unique_ptr<ICommand> activeCommand_;
        
        // Puntos temporales mientras se dibuja
        Point2D lastPoint; // Guarda el último punto para cálculos relativos
        // Copia sincronizada desde App para cálculos de tolerancia
        double viewScale = 1.0;  

        // Procesa el texto que el usuario escribe y pulsa Enter
        void processInput(std::string_view input);
        // Cancela cualquier comando en curso
        void cancelCommand();
        // ... métodos existentes ...
        std::string getHelpForTopic(std::string_view topic) {
            return getHelpText(topic);
        }
        std::string getEntityList() const;
    
        std::vector<Entity*> selectedEntities; // Punteros a entidades seleccionadas

        void clearSelection();
        void selectEntity(const Point2D& clickPoint, double tolerance);
        void deleteSelected();

        // Variables para Grip Edit
        Entity* activeGripEntity = nullptr;
        int activeGripIndex = -1;
        std::unique_ptr<Entity> gripBackup; // Para restaurar si se pulsa ESC

        // Devuelve la lista completa de comandos (actuales y futuros)
        std::vector<std::string> getAllCommands() const;

        // Historial de estados para Deshacer / Rehacer
        std::vector<std::vector<std::unique_ptr<Entity>>> undoStack;
        std::vector<std::vector<std::unique_ptr<Entity>>> redoStack;

        void saveState();   // Guarda el estado actual en la pila de deshacer
        void undo();        // Retrocede un estado
        void redo();        // Avanza un estado

        bool gridEnabled = false;
        void toggleGrid() { gridEnabled = !gridEnabled; }

        Point2D tempDimP1, tempDimP2, tempDimP3, tempDimP2_line2; // Puntos temporales para cota angular
        double tempDimAngle = 0.0; // Angulo calculado para cota angular.
        DimType currentDimType = DimType::HORIZONTAL;
        
    private:
        void executeCommand(std::string_view cmd);
        void processCoordinate(std::string_view coordStr);
        void processLayerCommand(std::string_view input); // Nuevo método
        // Detecta si el input es solo un número (valor escalar)
        bool isNumericValue(std::string_view str) const;
        // NUEVO: Devuelve el texto de ayuda en lugar de imprimirlo
        std::string getHelpText(std::string_view topic);
        [[nodiscard]] std::optional<Point2D> parseCoordinate(std::string_view str);
    };
    
} // namespace cad