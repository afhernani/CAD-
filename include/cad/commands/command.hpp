// include/cad/commands/command.hpp
#pragma once
#include "../core/geometry/point.hpp"
#include <string>
#include <memory>

// Forward declarations
namespace sf { class RenderWindow; class Font; }
namespace cad { class Engine; class View; }

namespace cad {

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
        // >>> ACTUALIZADO: Ahora recibe Engine& para que los comandos de modificación 
        // puedan acceder a engine.selectedEntities
        virtual void drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                  const Point2D& mouseWorldPos, sf::Font& font) const {
            // Implementación vacía por defecto
        }

    };

} // namespace cad