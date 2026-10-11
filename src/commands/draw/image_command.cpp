#include "cad/commands/draw/image_command.hpp"
#include "cad/commands/engine.hpp"
#include "cad/core/geometry/entities/image.hpp"
#include "cad/render/view.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/Image.hpp> // ✅ Necesario para leer dimensiones
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <iostream>

namespace cad {

    ImageCommand::ImageCommand(const std::string& preselectedPath) {
        imagePath_ = preselectedPath;
        if (!imagePath_.empty() && std::filesystem::exists(imagePath_)) {
            // Cargar imagen temporalmente solo para leer sus dimensiones
            sf::Image tempImg;
            if (tempImg.loadFromFile(imagePath_)) {
                unsigned int pxWidth = tempImg.getSize().x;
                unsigned int pxHeight = tempImg.getSize().y;
                aspectRatio_ = static_cast<double>(pxHeight) / static_cast<double>(pxWidth);
                hasPath_ = true;
                statusMessage_ = "IMAGEN | Especificar punto de insercion (clic en canvas o x,y):";
            } else {
                statusMessage_ = "IMAGEN | Error: No se pudo leer el archivo de imagen.";
                finished_ = true;
            }
        } else {
            statusMessage_ = "IMAGEN | Error: Archivo no encontrado.";
            finished_ = true;
        }
    }

    void ImageCommand::execute(const std::string& input, Engine& engine) {
        std::string upper = input;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        if (upper == "C" || upper == "S" || upper == "CANCELAR" || upper == "EXIT" || upper == "ESC") {
            onCancel();
            return;
        }

        std::istringstream iss(input);
        double x, y;
        char comma;
        if (iss >> x >> comma >> y && comma == ',') {
            onPoint({x, y}, engine);
            return;
        }

        if (hasPath_) {
            statusMessage_ = "IMAGEN | Especificar punto de insercion (clic en canvas o x,y):";
        }
    }

    void ImageCommand::onPoint(const Point2D& point, Engine& engine) {
        std::cout << "[DEBUG] ImageCommand::onPoint llamado con punto: " 
                  << point.x << "," << point.y << std::endl;

        if (!hasPath_) {
            finished_ = true;
            return;
        }

        auto img = std::make_unique<ImageEntity>();
        img->id = Entity::generateId();
        img->insertPoint = point;
        
        // TAMAÑO REALISTA: 100 unidades de ancho, alto proporcional
        img->width = 100.0; 
        img->height = 100.0 * aspectRatio_; 
        
        img->rotation = 0.0;
        img->imagePath = imagePath_;
        img->layerName = engine.doc.currentLayerName;

        engine.saveState();
        engine.doc.addEntity(std::move(img));

        statusMessage_ = "IMAGEN | Insertada correctamente.";
        finished_ = true; // Esto limpia el comando de la consola
    }

    void ImageCommand::onCancel() {
        finished_ = true;
        statusMessage_ = "IMAGEN | Cancelado.";
    }

    std::string ImageCommand::getStatusMessage() const { return statusMessage_; }
    bool ImageCommand::isComplete() const { return finished_; }

    void ImageCommand::drawFeedback(sf::RenderWindow& window, const View& view, Engine& engine,
                                    const Point2D& mouseWorldPos, sf::Font& font) const {
        if (!hasPath_) return;
        
        sf::Color feedbackColor(255, 255, 0, 150);
        auto w2s = [&](double x, double y) { return view.worldToScreen(x, y); };
        
        sf::Vector2f screenPos = w2s(mouseWorldPos.x, mouseWorldPos.y);
        // Dibujar un rectángulo de preview con la proporción real
        float previewWidth = 100.0f * view.getScale();
        float previewHeight = previewWidth * static_cast<float>(aspectRatio_);
        
        sf::RectangleShape rect(sf::Vector2f(previewWidth, previewHeight));
        rect.setOrigin(0, previewHeight); // Origen abajo-izquierda
        rect.setPosition(screenPos);
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(feedbackColor);
        rect.setOutlineThickness(2.0f);
        
        window.draw(rect);
    }

} // namespace cad