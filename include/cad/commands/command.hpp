// include/cad/commands/command.hpp
#pragma once
#include "../core/geometry/point.hpp"
#include <string>
#include <memory>

namespace cad {

    //class Document;
    class Engine;

    class ICommand {
    public:
        virtual ~ICommand() = default;
        
        // Procesar entrada de texto (coordenadas, valores, etc.)
        virtual void execute(const std::string& input, Engine& engine) = 0;
        
        // Procesar clic del ratón
        virtual void onPoint(const Point2D& point, Engine& engine) = 0;
        
        // Cancelar el comando
        virtual void onCancel() = 0;
        
        // Mensaje de estado actual
        virtual std::string getStatusMessage() const = 0;
        
        // ¿El comando está completo?
        virtual bool isComplete() const = 0;
        
        // Nombre del comando (para logs/ayuda)
        virtual std::string getName() const = 0;
    };

} // namespace cad