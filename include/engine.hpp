#pragma once
#include "document.hpp"
#include <string>
#include <optional>    // para manejar valores que pueden no existir
#include <string_view> // c++17/20 para vistars de cadenas sin copia

namespace cad {

    enum class Mode {
        IDLE,           // Esperando un comando
        DRAW_LINE,      // Esperando puntos para una línea
        DRAW_CIRCLE,     // Esperando centro y radio
        LAYER_COMMAND // Nuevo modo para gestionar subcomandos de capa
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
        void processInput(std::string_view input);

    private:
        void executeCommand(std::string_view cmd);
        void processCoordinate(std::string_view coordStr);
        void processLayerCommand(std::string_view input); // Nuevo método
        // <-- CAMBIO: Devuelve std::optional y es const
        [[nodiscard]] std::optional<Point2D> parseCoordinate(std::string_view str);
    };

} // namespace cad