#pragma once
#include "document.hpp"
#include <string>

namespace cad {

    enum class Mode {
        IDLE,           // Esperando un comando
        DRAW_LINE,      // Esperando puntos para una línea
        DRAW_CIRCLE     // Esperando centro y radio
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

        // Procesa el texto que el usuario escribe y pulsa Enter
        void processInput(const std::string& input);

    private:
        void executeCommand(const std::string& cmd);
        void processCoordinate(const std::string& coordStr);
        Point2D parseCoordinate(const std::string& str);
    };

} // namespace cad