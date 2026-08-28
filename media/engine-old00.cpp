#include "engine.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cad {

    void Engine::processInput(std::string_view input) {
        std::string cleanInput(input);
        cleanInput.erase(0, cleanInput.find_first_not_of(' '));
        cleanInput.erase(cleanInput.find_last_not_of(' ') + 1);

        if (currentMode == Mode::IDLE) {
            executeCommand(cleanInput);
        } else if (currentMode == Mode::LAYER_COMMAND) {
            processLayerCommand(cleanInput);
        } else {
            processCoordinate(cleanInput);
        }
    }

    void Engine::executeCommand(std::string_view cmd) {
        std::string upperCmd(cmd);
        std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

        if (upperCmd == "L" || upperCmd == "LINE" || upperCmd == "LINEA") {
            currentMode = Mode::DRAW_LINE;
            statusMessage = "LINEA | Especificar primer punto:";
        }
        else if (upperCmd == "C" || upperCmd == "CIRCLE" || upperCmd == "CIRCULO") {
            currentMode = Mode::DRAW_CIRCLE;
            statusMessage = "CIRCULO | Especificar centro:";
        }
        else if (upperCmd == "A" || upperCmd == "ARCO" || upperCmd == "ARC") {
            currentMode = Mode::DRAW_ARC;
            statusMessage = "ARCO | Especificar centro:";
        }
        else if (upperCmd == "PL" || upperCmd == "POLILINEA") {
            currentMode = Mode::DRAW_POLYLINE;
            statusMessage = "POLILINEA | Especificar primer punto (Enter para terminar):";
            // Limpiamos puntos anteriores si los hubiera
            // (Idealmente tendrías un vector temporal en Engine, pero usaremos doc.addEntity dinámicamente)
        }
        else if (upperCmd == "POL" || upperCmd == "POLIGONO") {
            currentMode = Mode::DRAW_POLYGON;
            statusMessage = "POLIGONO | Especificar centro:";
        }
        else if (upperCmd == "Z" || upperCmd == "BORRAR") {
            doc.clear();
            lastPoint = {0.0, 0.0};
            statusMessage = "Dibujo borrado.";
        }
        else if (upperCmd == "AXIS" || upperCmd == "EJES") {
            statusMessage = "Usa el boton en la barra de herramientas para activar/desactivar ejes";
        }
        else if (upperCmd == "LA" || upperCmd == "LAYER" || upperCmd == "CAPA") {
            currentMode = Mode::LAYER_COMMAND;
            statusMessage = "CAPA | ON <nombre> | OFF <nombre> | NEW <nombre> | SET <nombre> | LIST";
        }
        else {
            statusMessage = "Comando desconocido: " + std::string(cmd);
        }
    }

    void Engine::processLayerCommand(std::string_view input) {
        std::string upperInput(input);
        std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
        
        std::istringstream iss(upperInput);
        std::string subCmd;
        std::string layerName;
        
        iss >> subCmd;
        iss >> layerName;

        if (subCmd == "NEW" || subCmd == "NUEVA") {
            if (!layerName.empty()) {
                doc.addLayer(layerName);
                statusMessage = "Capa '" + layerName + "' creada.";
            } else {
                statusMessage = "Error: Especifica un nombre para la nueva capa.";
            }
        }
        else if (subCmd == "SET" || subCmd == "ACTUAL") {
            if (!layerName.empty() && doc.layers.find(layerName) != doc.layers.end()) {
                doc.setCurrentLayer(layerName);
                statusMessage = "Capa actual: '" + layerName + "'.";
            } else {
                statusMessage = "Error: Capa no encontrada o nombre vacío.";
            }
        }
        else if (subCmd == "ON" || subCmd == "ENCENDER") {
            if (!layerName.empty()) {
                doc.setLayerVisibility(layerName, true);
                statusMessage = "Capa '" + layerName + "' activada.";
            } else {
                statusMessage = "Error: Especifica el nombre de la capa.";
            }
        }
        else if (subCmd == "OFF" || subCmd == "APAGAR") {
            if (!layerName.empty()) {
                doc.setLayerVisibility(layerName, false);
                statusMessage = "Capa '" + layerName + "' desactivada.";
            } else {
                statusMessage = "Error: Especifica el nombre de la capa.";
            }
        }
        else if (subCmd == "LIST" || subCmd == "LISTA") {
            std::string list = "Capas: ";
            for (const auto& pair : doc.layers) {
                list += pair.first + (pair.second.isCurrent ? " (Actual) " : " ");
            }
            statusMessage = list;
        }
        else {
            statusMessage = "Subcomando no reconocido. Usa: NEW, SET, ON, OFF, LIST";
        }
        
        currentMode = Mode::IDLE;
    }

    void Engine::processCoordinate(std::string_view coordStr) {
        auto p = parseCoordinate(coordStr);

        if (!p.has_value()) {
            currentMode = Mode::IDLE;
            return;
        }

        lastPoint = p.value();

        if (currentMode == Mode::DRAW_LINE) {
            if (statusMessage.find("primer punto") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "LINEA | Especificar siguiente punto:";
            } else {
                tempPoint2 = lastPoint;
                auto newLine = std::make_unique<Line>();
                newLine->p1 = tempPoint1;
                newLine->p2 = tempPoint2;
                newLine->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newLine));
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        }
        else if (currentMode == Mode::DRAW_CIRCLE) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "CIRCULO | Especificar radio (o punto en el borde):";
            } else {
                // NUEVO: Detectar si es un radio directo o una coordenada
                std::string input(coordStr);
                try {
                    double radius = std::stod(input);
                    // Si no hay comas ni símbolos de coordenada, es un radio directo
                    if (input.find(',') == std::string::npos && 
                        input.find('@') == std::string::npos && 
                        input.find('<') == std::string::npos) {
                        // Radio directo
                        auto newCircle = std::make_unique<Circle>();
                        newCircle->center = tempPoint1;
                        newCircle->radius = radius;
                        newCircle->layerName = doc.currentLayerName;
                        doc.addEntity(std::move(newCircle));
                        currentMode = Mode::IDLE;
                        statusMessage = "Círculo creado con radio: " + std::to_string(radius);
                        return;
                    }
                } catch (...) {
                    // No es un número, continuar con cálculo de distancia
                }


                double dx = lastPoint.x - tempPoint1.x;
                double dy = lastPoint.y - tempPoint1.y;
                double radius = std::sqrt(dx * dx + dy * dy);
                
                auto newCircle = std::make_unique<Circle>();
                newCircle->center = tempPoint1;
                newCircle->radius = radius;
                newCircle->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newCircle));
                
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        }
        else if (currentMode == Mode::DRAW_ARC) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "ARCO | Especificar punto de inicio (radio):";
            } else if (statusMessage.find("inicio") != std::string::npos) {
                double dx = lastPoint.x - tempPoint1.x;
                double dy = lastPoint.y - tempPoint1.y;
                double radius = std::sqrt(dx * dx + dy * dy);
                tempPoint2 = lastPoint; // Guardamos el punto de inicio
                
                // Calculamos el ángulo inicial
                double startAngle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                if (startAngle < 0) startAngle += 360.0;
                
                statusMessage = "ARCO | Especificar punto final (o escribe el ángulo de barrido):";
            } else {
                // Aquí podrías detectar si es un número (ángulo de barrido) o un punto
                // Por simplicidad, asumimos punto final:
                double dx = lastPoint.x - tempPoint1.x;
                double dy = lastPoint.y - tempPoint1.y;
                double radius = std::sqrt(dx * dx + dy * dy); // Debería ser igual al anterior
                
                double endAngle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                if (endAngle < 0) endAngle += 360.0;

                auto newArc = std::make_unique<Arc>();
                newArc->center = tempPoint1;
                newArc->radius = radius;
                newArc->startAngle = std::atan2(tempPoint2.y - tempPoint1.y, tempPoint2.x - tempPoint1.x) * 180.0 / std::numbers::pi;
                newArc->endAngle = endAngle;
                newArc->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newArc));
                
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        }
        else if (currentMode == Mode::DRAW_POLYLINE) {
            // Para simplificar, creamos una nueva polilínea por cada segmento o gestionamos un vector temporal.
            // Opción simple: cada clic añade un segmento a una polilínea existente o crea una nueva.
            // Opción robusta: necesitas un std::vector<Point2D> tempPolylinePoints en Engine.
            statusMessage = "POLILINEA | Especificar siguiente punto (Escribe 'FIN' o 'C' para cerrar):";
            // (Te recomiendo implementar un vector temporal en Engine para esto, dime si quieres el código exacto)
        }
        else if (currentMode == Mode::DRAW_POLYGON) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "POLIGONO | Escribe el número de lados (ej: 6):";
            } else {
                // Aquí deberíamos haber capturado el número de lados. 
                // Si el input es un número, lo guardamos y pedimos el radio.
                // (Requiere un pequeño ajuste en processInput para detectar texto vs coordenada).
            }
        }
    }

    std::optional<Point2D> Engine::parseCoordinate(std::string_view str) {
        Point2D p{0.0, 0.0};
        std::string_view s = str;
        bool isRelative = false;

        if (s.empty()) {
            return std::nullopt;
        }

        if (s[0] == '@') {
            isRelative = true;
            s = s.substr(1);
        }

        try {
            size_t anglePos = s.find('<');
            if (anglePos != std::string_view::npos) {
                double dist = std::stod(std::string(s.substr(0, anglePos)));
                double angleDeg = std::stod(std::string(s.substr(anglePos + 1)));
                
                double angleRad = angleDeg * std::numbers::pi / 180.0;
                double dx = dist * std::cos(angleRad);
                double dy = dist * std::sin(angleRad);

                if (isRelative) {
                    p.x = lastPoint.x + dx;
                    p.y = lastPoint.y + dy;
                } else {
                    p.x = dx;
                    p.y = dy;
                }
            }
            else {
                size_t commaPos = s.find(',');
                if (commaPos != std::string_view::npos) {
                    double x = std::stod(std::string(s.substr(0, commaPos)));
                    double y = std::stod(std::string(s.substr(commaPos + 1)));

                    if (isRelative) {
                        p.x = lastPoint.x + x;
                        p.y = lastPoint.y + y;
                    } else {
                        p.x = x;
                        p.y = y;
                    }
                } else {
                    p.x = std::stod(std::string(s));
                    p.y = 0.0;
                }
            }
        } 
        catch (const std::invalid_argument&) {
            statusMessage = "Error: Formato de coordenada inválido.";
            return std::nullopt;
        } 
        catch (const std::out_of_range&) {
            statusMessage = "Error: Número fuera de rango.";
            return std::nullopt;
        }

        return p;
    }

} // namespace cad