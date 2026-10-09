#include "cad/commands/block/block_insert_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/render/view.hpp"             // OBLIGATORIO
#include <SFML/Graphics.hpp>               // OBLIGATORIO
#include "cad/core/constants.hpp"          // Para CANVAS_HEIGHT

namespace cad {

    BlockInsertCommand::BlockInsertCommand() {
        statusMessage_ = "INSERTAR | Nombre del bloque a insertar:";
    }

    void BlockInsertCommand::execute(const std::string& input, Engine& engine) {
        if (step_ == Step::WaitingName) {
            if (input.empty()) {
                statusMessage_ = "INSERTAR | Comando cancelado.";
                finished_ = true;
                return;
            }
            definition_ = engine.doc.findBlockDefinition(input);
            if (!definition_) {
                statusMessage_ = "INSERTAR | Bloque no encontrado. Nombre:";
                return; // Se queda en WaitingName
            }
            step_ = Step::WaitingInsertPoint;
            statusMessage_ = "INSERTAR | Punto de inserción:";
        }
    }

    void BlockInsertCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingInsertPoint && definition_) {
            engine.saveState();
            
            auto newInsert = std::make_unique<BlockInsert>();
            newInsert->id = Entity::generateId(); // Generar un nuevo ID único
            newInsert->definition = definition_;
            newInsert->insertPoint = point;
            newInsert->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newInsert));
            
            statusMessage_ = "INSERTAR | Bloque insertado. Nombre o Enter para terminar:";
            finished_ = true; 
        }
    }

    void BlockInsertCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "INSERTAR | Comando cancelado.";
    }

    std::string BlockInsertCommand::getStatusMessage() const { return statusMessage_; }
    bool BlockInsertCommand::isComplete() const { return finished_; }

    void BlockInsertCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                          const Point2D& mouseWorldPos, sf::Font& font) const {
        if (step_ == Step::WaitingInsertPoint && definition_ != nullptr) {
            sf::Color ghostColor(0, 200, 255, 150); // Cian translúcido
            
            auto w2s = [&](double x, double y) {
                return view.worldToScreen(x, y);
            };

            // 1. Intentar dibujar una vista previa de las entidades del bloque
            // (Asumiendo que BlockDefinition tiene un vector 'entities' de unique_ptr<Entity>)
            for (const auto& entityPtr : definition_->entities) {
                auto ghost = entityPtr->clone();
                
                // NOTA: Si tu BlockDefinition tiene un 'basePoint' explícito, 
                // el desplazamiento debería ser: (mouseWorldPos - definition_->basePoint)
                // Por ahora, asumimos que las entidades del bloque están relativas a (0,0) 
                // y las movemos directamente a la posición del ratón.
                // Si tus entidades ya tienen coordenadas absolutas del dibujo original, 
                // necesitarás ajustar este offset restando el punto base original del bloque.
                double offsetX = mouseWorldPos.x - definition_->basePoint.x;
                double offsetY = mouseWorldPos.y - definition_->basePoint.y;
                
                ghost->move(offsetX, offsetY);
                ghost->draw(window, w2s, ghostColor, view.getScale());
            }

            // 2. Marcador de punto de inserción (siempre visible y útil)
            sf::CircleShape insertMark(6.0f);
            insertMark.setFillColor(ghostColor);
            insertMark.setOrigin(6.0f, 6.0f);
            insertMark.setPosition(w2s(mouseWorldPos.x, mouseWorldPos.y));
            window.draw(insertMark);

            // 3. Texto con el nombre del bloque que se está insertando
            std::string blockName = definition_->name; // Asumiendo que BlockDefinition tiene un campo 'name'
            sf::Text nameText;
            nameText.setFont(font);
            nameText.setString(sf::String::fromUtf8(blockName.begin(), blockName.end()));
            nameText.setCharacterSize(14);
            nameText.setFillColor(ghostColor);
            nameText.setStyle(sf::Text::Bold);
            
            sf::Vector2f mouseScreen = w2s(mouseWorldPos.x, mouseWorldPos.y);
            nameText.setPosition(mouseScreen.x + 15.f, mouseScreen.y - 35.f);
            window.draw(nameText);
        }
    }

} // namespace cad