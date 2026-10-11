#include "cad/commands/block/block_insert_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/block_insert.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include "cad/core/constants.hpp"
#include <sstream>

namespace cad {

    BlockInsertCommand::BlockInsertCommand() {
        statusMessage_ = "INSERTAR | Nombre del bloque a insertar:";
    }

    void BlockInsertCommand::execute(const std::string& input, Engine& engine) {
        if (step_ == Step::WaitingName) {
            if (input.empty()) {
                statusMessage_ = "INSERTAR | Comando terminado.";
                finished_ = true;
                return;
            }
            definition_ = engine.doc.findBlockDefinition(input);
            if (!definition_) {
                statusMessage_ = "INSERTAR | Bloque no encontrado. Introduce otro nombre:";
                return; // Se queda en WaitingName
            }
            step_ = Step::WaitingInsertPoint;
            statusMessage_ = "INSERTAR | Punto de inserción (clic en canvas o coordenada x,y):";
        }
        // ✅ CORRECCIÓN 1: Manejar la entrada de coordenadas para el punto de inserción
        else if (step_ == Step::WaitingInsertPoint) {
            std::istringstream iss(input);
            double x, y;
            char comma;
            if (iss >> x >> comma >> y && comma == ',') {
                onPoint({x, y}, engine);
                return;
            }
            statusMessage_ = "INSERTAR | Formato no válido. Usa clic en canvas o coordenada x,y:";
        }
    }

    void BlockInsertCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingInsertPoint && definition_) {
            engine.saveState();
            
            auto newInsert = std::make_unique<BlockInsert>();
            newInsert->id = Entity::generateId();
            newInsert->definition = definition_;
            newInsert->insertPoint = point;
            newInsert->layerName = engine.doc.currentLayerName;
            engine.doc.addEntity(std::move(newInsert));
            
            statusMessage_ = "INSERTAR | Bloque insertado. Escribe otro nombre o Enter para terminar:";
            
            // ✅ CORRECCIÓN 3: Permitir múltiples inserciones (como AutoCAD)
            step_ = Step::WaitingName; 
            definition_ = nullptr; 
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
            sf::Color ghostColor(0, 200, 255, 150); 
            
            auto w2s = [&](double x, double y) {
                return view.worldToScreen(x, y);
            };

            // ✅ CORRECCIÓN 2: Las entidades ya están normalizadas al (0,0) local 
            // porque en BlockCreateCommand se les restó el basePoint.
            // Por tanto, solo necesitamos moverlas a la posición del ratón.
            for (const auto& entityPtr : definition_->entities) {
                auto ghost = entityPtr->clone();
                ghost->move(mouseWorldPos.x, mouseWorldPos.y); // ¡Solo esto!
                ghost->draw(window, w2s, ghostColor, view.getScale());
            }

            // Marcador de punto de inserción (coincidirá perfectamente con el bloque fantasma)
            sf::CircleShape insertMark(6.0f);
            insertMark.setFillColor(ghostColor);
            insertMark.setOrigin(6.0f, 6.0f);
            insertMark.setPosition(w2s(mouseWorldPos.x, mouseWorldPos.y));
            window.draw(insertMark);

            // Texto con el nombre del bloque
            std::string blockName = definition_->name;
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