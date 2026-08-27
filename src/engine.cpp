#include "engine.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <numbers>     // <-- NUEVO: C++20 para constantes matemáticas
#include <stdexcept>   // <-- NUEVO: Para capturar excepciones de std::stod


namespace cad {

    void Engine::processInput(std::string_view input) { // <-- CAMBIO: string_view
        // string_view no tiene .erase(), así que hacemos una copia local solo para limpiar
        std::string cleanInput(input);
        cleanInput.erase(0, cleanInput.find_first_not_of(' '));
        cleanInput.erase(cleanInput.find_last_not_of(' ') + 1);

        if (currentMode == Mode::IDLE) {
            executeCommand(cleanInput);
        } else {
            processCoordinate(cleanInput);
        }
    }

    void Engine::executeCommand(std::string_view cmd) { // <-- CAMBIO: string_view
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
        else if (upperCmd == "Z" || upperCmd == "BORRAR") {
            doc.clear();
            lastPoint = {0.0, 0.0}; // <-- MEJORA: Inicialización explícita
            statusMessage = "Dibujo borrado.";
        }
        else if (upperCmd == "AXIS" || upperCmd == "EJES") {
            statusMessage = "Usa el boton en la barra de herramientas para activar/desactivar ejes";
        }
        else {
            statusMessage = "Comando desconocido: " + std::string(cmd);
        }
    }

    void Engine::processCoordinate(std::string_view coordStr) { // <-- CAMBIO: string_view
        // <-- CAMBIO CRÍTICO: Ahora recibimos un optional
        auto p = parseCoordinate(coordStr);

        // BLINDAJE: Si el parsing falló (devolvió nullopt), cancelamos la operación
        if (!p.has_value()) {
            currentMode = Mode::IDLE; // Reseteamos el modo para que el usuario pueda intentar de nuevo
            return;
        }

        lastPoint = p.value(); // Desempaquetamos el valor con seguridad

        if (currentMode == Mode::DRAW_LINE) {
            if (statusMessage.find("primer punto") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "LINEA | Especificar siguiente punto:";
            } else {
                tempPoint2 = lastPoint;
                doc.lines.push_back({tempPoint1, tempPoint2});
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        }
        else if (currentMode == Mode::DRAW_CIRCLE) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "CIRCULO | Especificar radio (o punto en el borde):";
            } else {
                double dx = lastPoint.x - tempPoint1.x;
                double dy = lastPoint.y - tempPoint1.y;
                double radius = std::sqrt(dx * dx + dy * dy);
                doc.circles.push_back({tempPoint1, radius});
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        }
    }

    // <-- CAMBIO: Devuelve optional y es const
    std::optional<Point2D> Engine::parseCoordinate(std::string_view str) {
        Point2D p{0.0, 0.0}; // <-- MEJORA: Inicialización designada/clara
        std::string_view s = str;
        bool isRelative = false;

        if (s.empty()) {
            return std::nullopt;
        }

        // 1. Detectar si es relativa (@)
        if (s[0] == '@') {
            isRelative = true;
            s = s.substr(1);
        }

        try {
            // 2. Detectar si es polar (<)
            size_t anglePos = s.find('<');
            if (anglePos != std::string_view::npos) {
                // std::stod requiere std::string, así que convertimos el string_view temporalmente
                double dist = std::stod(std::string(s.substr(0, anglePos)));
                double angleDeg = std::stod(std::string(s.substr(anglePos + 1)));
                
                // <-- CAMBIO C++20: Usamos std::numbers::pi en lugar de M_PI
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
            // 3. Si es cartesiana (x,y)
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
        // <-- CAMBIO CRÍTICO: Capturamos las excepciones para evitar crashes
        catch (const std::invalid_argument&) {
            statusMessage = "Error: Formato de coordenada inválido.";
            return std::nullopt;
        } 
        catch (const std::out_of_range&) {
            statusMessage = "Error: Número fuera de rango.";
            return std::nullopt;
        }

        return p; // Si todo salió bien, devolvemos el punto válido
    }

} // namespace cad