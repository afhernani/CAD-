#include "engine.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace cad {

    void Engine::processInput(const std::string& input) {
        std::string cleanInput = input;
        // Eliminar espacios en blanco al inicio/final (básico)
        cleanInput.erase(0, cleanInput.find_first_not_of(' '));
        cleanInput.erase(cleanInput.find_last_not_of(' ') + 1);

        if (currentMode == Mode::IDLE) {
            executeCommand(cleanInput);
        } else {
            processCoordinate(cleanInput);
        }
    }

    void Engine::executeCommand(const std::string& cmd) {
        std::string upperCmd = cmd;
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
            lastPoint = {0,0};
            statusMessage = "Dibujo borrado.";
        }
        else if (upperCmd == "AXIS" || upperCmd == "EJES") {
            // Este comando será manejado por App, no por Engine
            // Lo dejamos como placeholder por ahora
            statusMessage = "Usa el boton en la barra de herramientas para activar/desactivar ejes";
        }
        else {
            statusMessage = "Comando desconocido: " + cmd;
        }
    }

    void Engine::processCoordinate(const std::string& coordStr) {
        Point2D p = parseCoordinate(coordStr);
        lastPoint = p; // Actualizamos el último punto

        if (currentMode == Mode::DRAW_LINE) {
            if (statusMessage.find("primer punto") != std::string::npos) {
                tempPoint1 = p;
                statusMessage = "LINEA | Especificar siguiente punto:";
            } else {
                tempPoint2 = p;
                doc.lines.push_back({tempPoint1, tempPoint2});
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        } 
        else if (currentMode == Mode::DRAW_CIRCLE) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = p;
                statusMessage = "CIRCULO | Especificar radio (o punto en el borde):";
            } else {
                double dx = p.x - tempPoint1.x;
                double dy = p.y - tempPoint1.y;
                double radius = std::sqrt(dx*dx + dy*dy);
                
                doc.circles.push_back({tempPoint1, radius});
                currentMode = Mode::IDLE;
                statusMessage = "Listo";
            }
        }
    }

    Point2D Engine::parseCoordinate(const std::string& str) {
        Point2D p;
        std::string s = str;
        bool isRelative = false;

        // 1. Detectar si es relativa (@)
        if (!s.empty() && s[0] == '@') {
            isRelative = true;
            s = s.substr(1);
        }

        // 2. Detectar si es polar (<)
        size_t anglePos = s.find('<');
        if (anglePos != std::string::npos) {
            double dist = std::stod(s.substr(0, anglePos));
            double angleDeg = std::stod(s.substr(anglePos + 1));
            double angleRad = angleDeg * M_PI / 180.0;
            
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
            if (commaPos != std::string::npos) {
                double x = std::stod(s.substr(0, commaPos));
                double y = std::stod(s.substr(commaPos + 1));
                
                if (isRelative) {
                    p.x = lastPoint.x + x;
                    p.y = lastPoint.y + y;
                } else {
                    p.x = x;
                    p.y = y;
                }
            } else {
                // Si solo escribe un numero, asumimos X, Y=0
                p.x = std::stod(s);
                p.y = 0.0;
            }
        }
        return p;
    }

} // namespace cad