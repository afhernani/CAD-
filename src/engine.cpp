#include "engine.hpp"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cad {

    void Engine::cancelCommand() {
        currentMode = Mode::IDLE;
        tempPolylinePoints.clear();
        tempPolygonSides = 0;
        tempArcRadius = 0.0;
        tempArcStartAngle = 0.0;
        statusMessage = "Comando cancelado.";
    }

    // NUEVO: Detecta si el string es solo un número (posiblemente con signo y decimales)
    bool Engine::isNumericValue(std::string_view str) const {
        std::string s(str);
        // Eliminar espacios
        s.erase(0, s.find_first_not_of(' '));
        s.erase(s.find_last_not_of(' ') + 1);
        
        if (s.empty()) return false;
        
        // Si tiene coma, @, o <, NO es un valor escalar
        if (s.find(',') != std::string::npos) return false;
        if (s.find('@') != std::string::npos) return false;
        if (s.find('<') != std::string::npos) return false;
        
        // Intentar convertir a número
        try {
            std::stod(s);
            return true;
        } catch (...) {
            return false;
        }
    }

    void Engine::processInput(std::string_view input) {
        std::string cleanInput(input);
        cleanInput.erase(0, cleanInput.find_first_not_of(' '));
        cleanInput.erase(cleanInput.find_last_not_of(' ') + 1);

        // Detectar comando HELP con argumento
        if (cleanInput.size() >= 4) {
            std::string upperClean(cleanInput);
            std::transform(upperClean.begin(), upperClean.end(), upperClean.begin(), ::toupper);
            
            if (upperClean.substr(0, 4) == "HELP" || upperClean.substr(0, 5) == "AYUDA") {
                std::string topic = "";
                if (upperClean.size() > 5) {
                    topic = cleanInput.substr(5); // Mantener case original para el tema
                }
                showHelp(topic);
                return;
            }
        }

        // ENTER vacío en polilínea -> terminar sin cerrar
        if (cleanInput.empty() && currentMode == Mode::DRAW_POLYLINE) {
            if (tempPolylinePoints.size() >= 2) {
                auto newPoly = std::make_unique<Polyline>();
                newPoly->points = tempPolylinePoints;
                newPoly->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newPoly));
                statusMessage = "Polilínea terminada (" + 
                                std::to_string(tempPolylinePoints.size()) + " puntos).";
            } else {
                statusMessage = "Polilínea cancelada (puntos insuficientes).";
            }
            tempPolylinePoints.clear();
            currentMode = Mode::IDLE;
            return;
        }

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
        else if (upperCmd == "A" || upperCmd == "ARC" || upperCmd == "ARCO") {
            currentMode = Mode::DRAW_ARC;
            statusMessage = "ARCO | Especificar centro:";
        }
        else if (upperCmd == "PL" || upperCmd == "POLILINEA") {
            currentMode = Mode::DRAW_POLYLINE;
            tempPolylinePoints.clear();
            statusMessage = "POLILINEA | Primer punto (Enter=terminar, C=cerrar, U=deshacer):";
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
        else if (upperCmd == "HELP" || upperCmd == "AYUDA" || upperCmd == "?") {
            // Si hay un argumento después de HELP, mostrar ayuda específica
            // Por simplicidad, si el comando es solo "HELP", mostrar general
            // Si es "HELP L", el usuario debe escribirlo en dos pasos o usamos processInput
            showHelp("");
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

        // Input vacío = cancelar comando (excepto en polilínea, donde termina)
        if (coordStr.empty()) {
            if (currentMode == Mode::DRAW_POLYLINE) {
                // Terminar polilínea abierta
                if (tempPolylinePoints.size() >= 2) {
                    auto newPoly = std::make_unique<Polyline>();
                    newPoly->points = tempPolylinePoints;
                    newPoly->layerName = doc.currentLayerName;
                    doc.addEntity(std::move(newPoly));
                    statusMessage = "Polilinea terminada (" + 
                                    std::to_string(tempPolylinePoints.size()) + " puntos).";
                } else {
                    statusMessage = "Polilinea cancelada (puntos insuficientes).";
                }
                tempPolylinePoints.clear();
                currentMode = Mode::IDLE;
            } //else {
                // Cualquier otro modo: cancelar
                //cancelCommand();
            //}
            // para otros comandos: no hacer nada, mantener el estado actual
            return;
        }
        // Comandos especiales para Polilínea
        if (currentMode == Mode::DRAW_POLYLINE) {
            std::string upperStr(coordStr);
            std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
            
            if (upperStr == "C" || upperStr == "CLOSE" || upperStr == "CERRAR") {
                if (tempPolylinePoints.size() >= 2) {
                    tempPolylinePoints.push_back(tempPolylinePoints.front());
                    auto newPoly = std::make_unique<Polyline>();
                    newPoly->points = tempPolylinePoints;
                    newPoly->layerName = doc.currentLayerName;
                    doc.addEntity(std::move(newPoly));
                    statusMessage = "Polilínea cerrada.";
                }
                tempPolylinePoints.clear();
                currentMode = Mode::IDLE;
                return;
            }
            if (upperStr == "U" || upperStr == "UNDO") {
                if (!tempPolylinePoints.empty()) {
                    tempPolylinePoints.pop_back();
                    statusMessage = "Último punto eliminado.";
                }
                return;
            }
        }

        // Detectar si es valor escalar (número puro) o coordenada
        bool isScalar = isNumericValue(coordStr);
        double scalarValue = 0.0;
        if (isScalar) {
            try {
                scalarValue = std::stod(std::string(coordStr));
            } catch (...) {
                isScalar = false;
            }
        }

        // Parsear como coordenada (si no es escalar, o si necesitamos el punto)
        std::optional<Point2D> p;
        if (!isScalar) {
            p = parseCoordinate(coordStr);
            if (!p.has_value()) {
                if (currentMode != Mode::DRAW_POLYLINE) {
                    currentMode = Mode::IDLE;
                }
                return;
            }
            lastPoint = p.value();
        }

        // --- LÍNEA ---
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
        // --- CÍRCULO ---
        else if (currentMode == Mode::DRAW_CIRCLE) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "CIRCULO | Radio (número) o punto en el borde:";
            } else {
                double radius;
                if (isScalar) {
                    radius = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    radius = std::sqrt(dx * dx + dy * dy);
                }
                
                auto newCircle = std::make_unique<Circle>();
                newCircle->center = tempPoint1;
                newCircle->radius = radius;
                newCircle->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newCircle));
                
                currentMode = Mode::IDLE;
                statusMessage = "Círculo creado (radio: " + std::to_string(radius) + ")";
            }
        }
        // --- ARCO ---
        else if (currentMode == Mode::DRAW_ARC) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "ARCO | Radio (número) o punto para definir radio:";
            } 
            else if (statusMessage.find("radio") != std::string::npos) {
                if (isScalar) {
                    tempArcRadius = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    tempArcRadius = std::sqrt(dx * dx + dy * dy);
                }
                statusMessage = "ARCO | Ángulo inicio (grados, 0=Este) o punto:";
            }
            else if (statusMessage.find("inicio") != std::string::npos) {
                if (isScalar) {
                    tempArcStartAngle = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    tempArcStartAngle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                    if (tempArcStartAngle < 0) tempArcStartAngle += 360.0;
                }
                statusMessage = "ARCO | Ángulo final (grados) o punto:";
            }
            else {
                double endAngle;
                if (isScalar) {
                    endAngle = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    endAngle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                    if (endAngle < 0) endAngle += 360.0;
                }

                auto newArc = std::make_unique<Arc>();
                newArc->center = tempPoint1;
                newArc->radius = tempArcRadius;
                newArc->startAngle = tempArcStartAngle;
                newArc->endAngle = endAngle;
                newArc->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newArc));
                
                currentMode = Mode::IDLE;
                statusMessage = "Arco creado.";
            }
        }
        // --- POLÍGONO ---
        else if (currentMode == Mode::DRAW_POLYGON) {
            if (statusMessage.find("centro") != std::string::npos) {
                tempPolygonCenter = lastPoint;
                statusMessage = "POLIGONO | Número de lados (ej: 6):";
            }
            else if (statusMessage.find("lados") != std::string::npos) {
                // Si coordStr está vacío, usar valor por defecto
                if (coordStr.empty()) {
                    tempPolygonSides = 6; // Valor por defecto
                } else {
                    tempPolygonSides = static_cast<int>(scalarValue);
                    if (tempPolygonSides < 3) tempPolygonSides = 3;
                }
                statusMessage = "POLIGONO | Radio (número) o punto para definir radio:";
            }
            else {
                double radius;
                if (isScalar) {
                    radius = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPolygonCenter.x;
                    double dy = lastPoint.y - tempPolygonCenter.y;
                    radius = std::sqrt(dx * dx + dy * dy);
                }

                auto newPoly = std::make_unique<Polygon>();
                newPoly->center = tempPolygonCenter;
                newPoly->sides = tempPolygonSides;
                newPoly->radius = radius;
                newPoly->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newPoly));
                
                currentMode = Mode::IDLE;
                statusMessage = "Polígono creado (" + std::to_string(tempPolygonSides) + " lados).";
            }
        }
        // --- POLILÍNEA ---
        else if (currentMode == Mode::DRAW_POLYLINE) {
            tempPolylinePoints.push_back(lastPoint);
            statusMessage = "POLILINEA | Siguiente punto (Enter=terminar, C=cerrar, U=deshacer):";
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
                    // Número solo sin contexto de coordenada -> no debería llegar aquí
                    // porque isNumericValue lo captura antes
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

    void Engine::showHelp(std::string_view topic) {
        std::string upperTopic(topic);
        std::transform(upperTopic.begin(), upperTopic.end(), upperTopic.begin(), ::toupper);
        
        // Limpiar espacios
        upperTopic.erase(0, upperTopic.find_first_not_of(' '));
        upperTopic.erase(upperTopic.find_last_not_of(' ') + 1);

        // Si no hay tema, mostrar ayuda general
        if (upperTopic.empty()) {
            statusMessage = "AYUDA: Comandos disponibles: L, C, A, PL, POL, LA, Z, AXIS, HELP. Usa HELP <comando> para detalles.";
            std::cout << "\n========================================" << std::endl;
            std::cout << "  CAD+ v0.4 - SISTEMA DE AYUDA" << std::endl;
            std::cout << "========================================\n" << std::endl;
            
            std::cout << "COMANDOS DE DIBUJO:" << std::endl;
            std::cout << "  L, LINEA      - Dibujar línea" << std::endl;
            std::cout << "  C, CIRCULO    - Dibujar círculo" << std::endl;
            std::cout << "  A, ARCO       - Dibujar arco" << std::endl;
            std::cout << "  PL, POLILINEA - Dibujar polilínea" << std::endl;
            std::cout << "  POL, POLIGONO - Dibujar polígono regular\n" << std::endl;
            
            std::cout << "COMANDOS DE EDICIÓN:" << std::endl;
            std::cout << "  Z, BORRAR     - Borrar todo el dibujo" << std::endl;
            std::cout << "  ESC           - Cancelar comando actual\n" << std::endl;
            
            std::cout << "COMANDOS DE CAPAS:" << std::endl;
            std::cout << "  LA, LAYER, CAPA - Gestionar capas" << std::endl;
            std::cout << "    Subcomandos: NEW, SET, ON, OFF, LIST\n" << std::endl;
            
            std::cout << "COMANDOS DE VISTA:" << std::endl;
            std::cout << "  AXIS, EJES    - Activar/desactivar ejes" << std::endl;
            std::cout << "  Rueda ratón   - Zoom" << std::endl;
            std::cout << "  Clic derecho  - Desplazar vista (Pan)\n" << std::endl;
            
            std::cout << "SISTEMA DE COORDENADAS:" << std::endl;
            std::cout << "  x,y           - Coordenadas absolutas" << std::endl;
            std::cout << "  @dx,dy        - Coordenadas relativas" << std::endl;
            std::cout << "  @dist<ang     - Coordenadas polares" << std::endl;
            std::cout << "  numero        - Valor escalar (radio, ángulo, lados)\n" << std::endl;
            
            std::cout << "AYUDA ESPECÍFICA:" << std::endl;
            std::cout << "  HELP <comando> - Mostrar detalles de un comando" << std::endl;
            std::cout << "  Ejemplo: HELP L, HELP LA, HELP POL\n" << std::endl;
            
            std::cout << "========================================\n" << std::endl;
            return;
        }

        // Ayuda específica por comando
        if (upperTopic == "L" || upperTopic == "LINE" || upperTopic == "LINEA") {
            std::cout << "\n--- COMANDO: LINEA (L) ---" << std::endl;
            std::cout << "Dibuja una línea recta entre dos puntos.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  1. Escribe: L" << std::endl;
            std::cout << "  2. Especifica primer punto (coordenada o clic)" << std::endl;
            std::cout << "  3. Especifica segundo punto" << std::endl;
            std::cout << "  4. La línea se crea automáticamente\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  L -> 0,0 -> 100,0     (línea de 100 unidades horizontal)" << std::endl;
            std::cout << "  L -> 0,0 -> @50,30    (línea relativa)" << std::endl;
            std::cout << "  L -> 0,0 -> @100<45   (línea polar: 100 unidades a 45°)\n" << std::endl;
            statusMessage = "Ayuda: LINEA - Dibuja línea entre dos puntos";
        }
        else if (upperTopic == "C" || upperTopic == "CIRCLE" || upperTopic == "CIRCULO") {
            std::cout << "\n--- COMANDO: CIRCULO (C) ---" << std::endl;
            std::cout << "Dibuja un círculo especificando centro y radio.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  1. Escribe: C" << std::endl;
            std::cout << "  2. Especifica centro (coordenada o clic)" << std::endl;
            std::cout << "  3. Especifica radio de dos formas:" << std::endl;
            std::cout << "     - Número directo: 50 (radio = 50 unidades)" << std::endl;
            std::cout << "     - Punto en el borde: clic o coordenada\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  C -> 0,0 -> 50        (círculo centro (0,0), radio 50)" << std::endl;
            std::cout << "  C -> 0,0 -> 100,0     (círculo centro (0,0), radio 100)\n" << std::endl;
            statusMessage = "Ayuda: CIRCULO - Dibuja círculo con centro y radio";
        }
        else if (upperTopic == "A" || upperTopic == "ARC" || upperTopic == "ARCO") {
            std::cout << "\n--- COMANDO: ARCO (A) ---" << std::endl;
            std::cout << "Dibuja un arco especificando centro, radio y ángulos.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  1. Escribe: A" << std::endl;
            std::cout << "  2. Especifica centro" << std::endl;
            std::cout << "  3. Especifica radio (número o punto)" << std::endl;
            std::cout << "  4. Especifica ángulo de inicio (grados, 0°=Este)" << std::endl;
            std::cout << "  5. Especifica ángulo final (grados)\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  A -> 0,0 -> 50 -> 0 -> 180    (semicírculo superior)" << std::endl;
            std::cout << "  A -> 0,0 -> 100 -> 90 -> 270  (semicírculo izquierdo)\n" << std::endl;
            std::cout << "Nota: Los ángulos van en sentido antihorario desde el eje X positivo." << std::endl;
            statusMessage = "Ayuda: ARCO - Dibuja arco con centro, radio y ángulos";
        }
        else if (upperTopic == "PL" || upperTopic == "POLILINEA") {
            std::cout << "\n--- COMANDO: POLILINEA (PL) ---" << std::endl;
            std::cout << "Dibuja una secuencia de segmentos conectados.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  1. Escribe: PL" << std::endl;
            std::cout << "  2. Especifica puntos sucesivos" << std::endl;
            std::cout << "  3. Termina de una de estas formas:" << std::endl;
            std::cout << "     - Enter vacío: termina polilínea abierta" << std::endl;
            std::cout << "     - C (Cerrar): cierra la polilínea" << std::endl;
            std::cout << "     - U (Deshacer): elimina último punto\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  PL -> 0,0 -> 100,0 -> 100,100 -> C    (triángulo cerrado)" << std::endl;
            std::cout << "  PL -> 0,0 -> 50,0 -> 50,50 -> Enter   (polilínea abierta)\n" << std::endl;
            statusMessage = "Ayuda: POLILINEA - Dibuja secuencia de segmentos";
        }
        else if (upperTopic == "POL" || upperTopic == "POLIGONO") {
            std::cout << "\n--- COMANDO: POLIGONO (POL) ---" << std::endl;
            std::cout << "Dibuja un polígono regular especificando centro, lados y radio.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  1. Escribe: POL" << std::endl;
            std::cout << "  2. Especifica centro" << std::endl;
            std::cout << "  3. Escribe número de lados (mínimo 3)" << std::endl;
            std::cout << "  4. Especifica radio (número o punto)\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  POL -> 0,0 -> 6 -> 50     (hexágono, radio 50)" << std::endl;
            std::cout << "  POL -> 0,0 -> 4 -> 100    (cuadrado, radio 100)\n" << std::endl;
            statusMessage = "Ayuda: POLIGONO - Dibuja polígono regular";
        }
        else if (upperTopic == "LA" || upperTopic == "LAYER" || upperTopic == "CAPA") {
            std::cout << "\n--- COMANDO: CAPAS (LA) ---" << std::endl;
            std::cout << "Gestiona las capas del dibujo.\n" << std::endl;
            std::cout << "Subcomandos:" << std::endl;
            std::cout << "  LA NEW <nombre>    - Crear nueva capa" << std::endl;
            std::cout << "  LA SET <nombre>    - Establecer capa actual" << std::endl;
            std::cout << "  LA ON <nombre>     - Activar visibilidad de capa" << std::endl;
            std::cout << "  LA OFF <nombre>    - Desactivar visibilidad de capa" << std::endl;
            std::cout << "  LA LIST            - Listar todas las capas\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  LA NEW Muros       - Crea capa 'Muros'" << std::endl;
            std::cout << "  LA SET Muros       - Activa 'Muros' como capa actual" << std::endl;
            std::cout << "  LA OFF Muros       - Oculta la capa 'Muros'" << std::endl;
            std::cout << "  LA LIST            - Muestra: 0 (Actual) Muros\n" << std::endl;
            std::cout << "Nota: Las entidades nuevas se crean en la capa actual." << std::endl;
            statusMessage = "Ayuda: CAPAS - Gestión de capas del dibujo";
        }
        else if (upperTopic == "Z" || upperTopic == "BORRAR") {
            std::cout << "\n--- COMANDO: BORRAR (Z) ---" << std::endl;
            std::cout << "Elimina TODAS las entidades del dibujo.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  Escribe: Z" << std::endl;
            std::cout << "  El dibujo se limpia completamente.\n" << std::endl;
            std::cout << "⚠️  ADVERTENCIA: Esta acción no se puede deshacer." << std::endl;
            std::cout << "  Usa con precaución.\n" << std::endl;
            statusMessage = "Ayuda: BORRAR - Elimina todo el dibujo";
        }
        else if (upperTopic == "AXIS" || upperTopic == "EJES") {
            std::cout << "\n--- COMANDO: EJES (AXIS) ---" << std::endl;
            std::cout << "Activa o desactiva la visualización de los ejes cartesianos.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  Escribe: AXIS" << std::endl;
            std::cout << "  O usa el botón en la barra de herramientas.\n" << std::endl;
            std::cout << "Los ejes muestran:" << std::endl;
            std::cout << "  - Eje X en rojo (50 unidades)" << std::endl;
            std::cout << "  - Eje Y en verde (50 unidades)" << std::endl;
            std::cout << "  - Origen en blanco\n" << std::endl;
            statusMessage = "Ayuda: EJES - Activa/desactiva ejes cartesianos";
        }
        else if (upperTopic == "HELP" || upperTopic == "AYUDA") {
            std::cout << "\n--- COMANDO: AYUDA (HELP) ---" << std::endl;
            std::cout << "Muestra información sobre los comandos disponibles.\n" << std::endl;
            std::cout << "Uso:" << std::endl;
            std::cout << "  HELP              - Lista general de comandos" << std::endl;
            std::cout << "  HELP <comando>    - Detalles de un comando específico\n" << std::endl;
            std::cout << "Ejemplos:" << std::endl;
            std::cout << "  HELP              - Muestra todos los comandos" << std::endl;
            std::cout << "  HELP L            - Ayuda sobre LINEA" << std::endl;
            std::cout << "  HELP LA           - Ayuda sobre CAPAS" << std::endl;
            std::cout << "  HELP POL          - Ayuda sobre POLIGONO\n" << std::endl;
            statusMessage = "Ayuda: HELP - Sistema de ayuda";
        }
        else {
            std::cout << "\n⚠️  Comando no reconocido: " << upperTopic << std::endl;
            std::cout << "Usa HELP para ver la lista de comandos disponibles.\n" << std::endl;
            statusMessage = "Comando no encontrado. Usa HELP para ver disponibles.";
        }
    }


} // namespace cad