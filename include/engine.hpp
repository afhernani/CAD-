#pragma once
#include "document.hpp"
#include <string>
#include <optional>    // para manejar valores que pueden no existir
#include <string_view> // c++17/20 para vistars de cadenas sin copia
#include <vector>

namespace cad {

    enum class Mode {
        IDLE,           // Esperando un comando
        DRAW_LINE,      // Esperando puntos para una línea
        DRAW_CIRCLE,     // Esperando centro y radio
        DRAW_ARC,
        DRAW_POLYLINE,
        DRAW_POLYGON,
        DRAW_ELLIPSE,
        LAYER_COMMAND, // Nuevo modo para gestionar subcomandos de capa
        COPY,
        ROTATE,
        SCALE,
        MOVE,
        MIRROR,
        MEASURE_DIST,
        TRIM,
        EXTEND,
        UNDO,
        REDO,
        GRIP_EDIT // << grips editables
    };

    class Engine {
    public:
        Document doc;
        Mode currentMode = Mode::IDLE;
        std::string statusMessage = "Listo";
        
        // Puntos temporales mientras se dibuja
        Point2D tempPoint1;
        Point2D tempPoint2;
        Point2D lastPoint; // Guarda el último punto para cálculos relativos
        // Copia sincronizada desde App para cálculos de tolerancia
        double viewScale = 1.0;  

        // Estado específico para Polilínea
        std::vector<Point2D> tempPolylinePoints;

        // Estado específico para Polígono
        Point2D tempPolygonCenter;
        int tempPolygonSides = 0;

        // Estado específico para Arco
        double tempArcRadius = 0.0;
        double tempArcStartAngle = 0.0;

        // Procesa el texto que el usuario escribe y pulsa Enter
        void processInput(std::string_view input);
        // Cancela cualquier comando en curso
        void cancelCommand();
        // ... métodos existentes ...
        std::string getHelpForTopic(std::string_view topic) {
            return getHelpText(topic);
        }
        
        std::vector<Entity*> selectedEntities; // Punteros a entidades seleccionadas

        void clearSelection();
        void selectEntity(const Point2D& clickPoint, double tolerance);
        void deleteSelected();

        Point2D moveBasePoint; // punto para mover
        // COPIAR Y ROTAR
        Point2D copyBasePoint;
        Point2D rotateCenter;
        double rotateAngle = 0.0;
        // ESCALAR
        Point2D scaleBasePoint;
        double scaleFactor = 1.0;
        // MIRROR
        Point2D mirrorAxisP1;
        // EXTEND / TRIM
        std::vector<Entity*> trimBoundaries;
        std::vector<Entity*> extendBoundaries;
        bool trimSelectingBoundaries = true;
        bool extendSelectingBoundaries = true;
        // Variables para Grip Edit
        Entity* activeGripEntity = nullptr;
        int activeGripIndex = -1;
        std::unique_ptr<Entity> gripBackup; // Para restaurar si se pulsa ESC

        // Variables temporales para elipse
        double tempEllipseMajorRadius = 0.0;
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