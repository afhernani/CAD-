#include "cad/commands/draw/text_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/document/document.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include "cad/core/constants.hpp"
#include <sstream>
#include <cmath>

namespace cad {

    namespace {
        sf::String toSfString(const std::string& utf8Str) {
            return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
        }
    }

    TextCommand::TextCommand() {
        statusMessage_ = "TEXTO | Especificar punto de inserción:";
    }

    void TextCommand::execute(const std::string& input, Engine& engine) {
        // 1. SI ESTAMOS ESPERANDO EL PUNTO, INTENTAR PARSEAR COORDENADA "x,y"
        if (step_ == Step::WaitingPoint) {
            std::istringstream iss(input);
            double x, y;
            char comma;
            if (iss >> x >> comma >> y && comma == ',') {
                onPoint({x, y}, engine);
                return; // Salimos porque onPoint ya actualizó el estado
            }
        }

        // 2. SI ESTAMOS ESPERANDO EL TEXTO, LO GUARDAMOS Y CREAMOS LA ENTIDAD
        if (step_ == Step::WaitingText) {
            if (input.empty()) {
                if (!content_.empty()) {
                    createTextEntity(engine);
                }
                finished_ = true;
                statusMessage_ = "TEXTO | Comando finalizado.";
            } else {
                content_ = input;
                createTextEntity(engine);
                finished_ = true;
                statusMessage_ = "TEXTO | Texto creado.";
            }
            return;
        }

        // 3. PARA ALTURA Y ROTACIÓN, PARSEAMOS NÚMEROS SIMPLES
        try {
            double value = std::stod(input);
            if (step_ == Step::WaitingHeight) {
                height_ = value > 0 ? value : 1.0;
                step_ = Step::WaitingRotation;
                statusMessage_ = "TEXTO | Especificar ángulo de rotación <0>:";
            } else if (step_ == Step::WaitingRotation) {
                rotation_ = value * 3.14159265358979323846 / 180.0; // Convertir a radianes
                step_ = Step::WaitingText;
                statusMessage_ = "TEXTO | Introducir texto (Enter para terminar):";
            }
        } catch (...) {
            statusMessage_ = "TEXTO | Valor no válido. Se esperaba un número o coordenada x,y.";
        }
    }

    void TextCommand::onPoint(const Point2D& point, Engine& engine) {
        if (step_ == Step::WaitingPoint) {
            position_ = point;
            step_ = Step::WaitingHeight;
            statusMessage_ = "TEXTO | Especificar altura <1.0>:";
        }
    }

    void TextCommand::createTextEntity(Engine& engine) {
        auto text = std::make_unique<Text>();
        text->position = position_;
        text->content = content_;
        text->height = height_;
        text->rotation = rotation_;
        text->layerName = engine.doc.currentLayerName;
        
        engine.saveState();
        engine.doc.addEntity(std::move(text));
    }

    void TextCommand::onCancel() { 
        finished_ = true; 
        statusMessage_ = "TEXTO | Cancelado."; 
    }

    std::string TextCommand::getStatusMessage() const { 
        return statusMessage_; 
    }

    bool TextCommand::isComplete() const { 
        return finished_; 
    }

    void TextCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                   const Point2D& mouseWorldPos, sf::Font& font) const {
        sf::Color previewColor(255, 255, 0, 180);
        
        if (step_ == Step::WaitingPoint) {
            // Dibujar cruz en el ratón
            sf::Vector2f pos = view.worldToScreen(mouseWorldPos.x, mouseWorldPos.y);
            sf::Vertex v1, v2, v3, v4;
            v1.position = {pos.x - 5, pos.y}; v1.color = previewColor;
            v2.position = {pos.x + 5, pos.y}; v2.color = previewColor;
            v3.position = {pos.x, pos.y - 5}; v3.color = previewColor;
            v4.position = {pos.x, pos.y + 5}; v4.color = previewColor;
            sf::Vertex lines[] = {v1, v2, v3, v4};
            window.draw(lines, 4, sf::Lines);
        }
        else if (step_ >= Step::WaitingHeight && !finished_) {
            // Mostrar preview del texto en la posición elegida
            Point2D drawPos = (step_ == Step::WaitingPoint) ? mouseWorldPos : position_;
            sf::Vector2f screenPos = view.worldToScreen(drawPos.x, drawPos.y);
            
            sf::Text sfText;
            sfText.setFont(font);
            sfText.setString(toSfString(content_.empty() ? "Texto de prueba" : content_));
            sfText.setCharacterSize(static_cast<unsigned int>(height_ * view.getScale()));
            sfText.setFillColor(previewColor);
            sfText.setRotation(-rotation_ * 180.0f / 3.14159f); // SFML usa rotación horaria
            sfText.setOrigin(0, height_ * view.getScale()); // Alinear abajo-izquierda
            sfText.setPosition(screenPos);
            window.draw(sfText);
        }
    }

} // namespace cad