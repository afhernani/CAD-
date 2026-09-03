#include "engine.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
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
                getHelpText(topic);
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
        else if (upperCmd == "EL" || upperCmd == "ELLIPSE" || upperCmd == "ELIPSE") {
            currentMode = Mode::DRAW_ELLIPSE;
            statusMessage = "ELIPSE | Especificar centro:";
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
        else if (upperCmd == "M" || upperCmd == "MOVE" || upperCmd == "MOVER") {
            if (selectedEntities.empty()) {
                statusMessage = "MOVER | Primero selecciona entidades (clic izquierdo).";
            } else {
                currentMode = Mode::MOVE;
                statusMessage = "MOVER | Especificar punto base (" + 
                                std::to_string(selectedEntities.size()) + " entidades seleccionadas):";
            }
        }
        else if (upperCmd == "CO" || upperCmd == "COPY" || upperCmd == "COPIAR") {
            if (selectedEntities.empty()) {
                statusMessage = "COPIAR | Primero selecciona entidades.";
            } else {
                currentMode = Mode::COPY;
                statusMessage = "COPIAR | Punto base (" +
                                std::to_string(selectedEntities.size()) + " entidades):";
            }
        }
        else if (upperCmd == "RO" || upperCmd == "ROTATE" || upperCmd == "ROTAR") {
            if (selectedEntities.empty()) {
                statusMessage = "ROTAR | Primero selecciona entidades.";
            } else {
                currentMode = Mode::ROTATE;
                statusMessage = "ROTAR | Centro de rotación:";
            }
        }

        else if (upperCmd == "SC" || upperCmd == "SCALE" || upperCmd == "ESCALAR") {
            if (selectedEntities.empty()) {
                statusMessage = "ESCALAR | Primero selecciona entidades (clic izquierdo).";
            } else {
                currentMode = Mode::SCALE;
                statusMessage = "ESCALAR | Punto base (" + 
                                std::to_string(selectedEntities.size()) + " entidades):";
            }
        }
        else if (upperCmd == "SI" || upperCmd == "SYM" || upperCmd == "MIRROR" || upperCmd == "SIMETRIA") {
            if (selectedEntities.empty()) {
                statusMessage = "SIMETRIA | Primero selecciona entidades (clic izquierdo).";
            } else {
                currentMode = Mode::MIRROR;
                statusMessage = "SIMETRIA | Primer punto del eje de simetría:";
            }
        }
        else if (upperCmd == "DIST" || upperCmd == "MEDIR") {
            currentMode = Mode::MEASURE_DIST;
            statusMessage = "DIST | Especificar primer punto:";
        }

        else if (upperCmd == "TR" || upperCmd == "TRIM" || upperCmd == "RECORTAR") {
            currentMode = Mode::TRIM;
            trimSelectingBoundaries = true;
            trimBoundaries.clear();
            statusMessage = "TRIM | Seleccionar cortes (Enter para terminar):";
        }
        else if (upperCmd == "EX" || upperCmd == "EXTEND" || upperCmd == "ALARGAR") {
            currentMode = Mode::EXTEND;
            extendSelectingBoundaries = true;
            extendBoundaries.clear();
            statusMessage = "EXTEND | Seleccionar bordes (Enter para terminar):";
        }
        else if (upperCmd == "HELP" || upperCmd == "AYUDA" || upperCmd == "?") {
            //std::string helpText = getHelpText("");
            // Aquí necesitamos pasar el texto a App para que lo muestre
            // Opción simple: usar statusMessage para confirmación
            statusMessage = "Ayuda: escribe HELP <comando> para más detalles";
            // Nota: Necesitamos una forma de comunicar esto a App
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
            }
            else if (currentMode == Mode::TRIM) {
                if (trimSelectingBoundaries) {
                    // Terminar selección de cortes, pasar a recortar
                    trimSelectingBoundaries = false;
                    statusMessage = "TRIM | Seleccionar entidades a recortar (clic sobre ellas):";
                } else {
                    // Ya estábamos recortando, salir del comando
                    currentMode = Mode::IDLE;
                    statusMessage = "TRIM cancelado.";
                }
            }
            else if (currentMode == Mode::EXTEND) {
                if (extendSelectingBoundaries) {
                    extendSelectingBoundaries = false;
                    statusMessage = "EXTEND | Seleccionar entidades a alargar (clic sobre ellas):";
                } else {
                    currentMode = Mode::IDLE;
                    statusMessage = "EXTEND cancelado.";
                }
            }
            // Para otros modos: no hacer nada, mantener estado
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
            // Condición directa, sin variables intermedias
            if (statusMessage.find("centro") != std::string::npos || statusMessage.find("Centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "CIRCULO | Radio (numero) o punto en el borde:";
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
                statusMessage = "Circulo creado (radio: " + std::to_string(radius) + ")";
            }
        }
        // --- ARCO ---
        else if (currentMode == Mode::DRAW_ARC) {
            const double PI = 3.14159265358979323846; // Constante local a prueba de fallos en VS2019
            
            if (statusMessage.find("centro") != std::string::npos || statusMessage.find("Centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "ARCO | Radio (numero) o punto para definir radio:";
            }
            else if (statusMessage.find("radio") != std::string::npos || statusMessage.find("Radio") != std::string::npos) {
                if (isScalar) {
                    tempArcRadius = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    tempArcRadius = std::sqrt(dx * dx + dy * dy);
                }
                statusMessage = "ARCO | Angulo inicio (grados, 0=Este) o punto:";
            }
            else if (statusMessage.find("inicio") != std::string::npos || statusMessage.find("Inicio") != std::string::npos) {
                if (isScalar) {
                    tempArcStartAngle = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    tempArcStartAngle = std::atan2(dy, dx) * 180.0 / PI;
                    if (tempArcStartAngle < 0) tempArcStartAngle += 360.0;
                }
                statusMessage = "ARCO | Angulo final (grados) o punto:";
            }
            else {
                double endAngle;
                if (isScalar) {
                    endAngle = scalarValue;
                } else {
                    double dx = lastPoint.x - tempPoint1.x;
                    double dy = lastPoint.y - tempPoint1.y;
                    endAngle = std::atan2(dy, dx) * 180.0 / PI;
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

        // --- ELIPSE ---
        else if (currentMode == Mode::DRAW_ELLIPSE) {
            if (statusMessage.find("centro") != std::string::npos || 
                statusMessage.find("Centro") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "ELIPSE | Punto final del eje mayor (o valor):";
            }
            else if (statusMessage.find("eje mayor") != std::string::npos ||
                    statusMessage.find("Eje mayor") != std::string::npos) {
                tempPoint2 = lastPoint;  // Guardar el punto del eje mayor
                statusMessage = "ELIPSE | Radio del otro eje (o valor):";
            }
            else {
                // Calcular eje mayor
                double dx = tempPoint2.x - tempPoint1.x;
                double dy = tempPoint2.y - tempPoint1.y;
                double majorRadius = std::sqrt(dx * dx + dy * dy);
                double rotationAngle = std::atan2(dy, dx);
                
                // Calcular eje menor
                double minorRadius;
                if (isScalar) {
                    minorRadius = scalarValue;
                } else {
                    double dx2 = lastPoint.x - tempPoint1.x;
                    double dy2 = lastPoint.y - tempPoint1.y;
                    minorRadius = std::sqrt(dx2 * dx2 + dy2 * dy2);
                }
                
                auto newEllipse = std::make_unique<Ellipse>();
                newEllipse->center = tempPoint1;
                newEllipse->majorRadius = majorRadius;
                newEllipse->minorRadius = minorRadius;
                newEllipse->rotationAngle = rotationAngle;
                newEllipse->layerName = doc.currentLayerName;
                doc.addEntity(std::move(newEllipse));
                
                currentMode = Mode::IDLE;
                statusMessage = "Elipse creada.";
            }
        }

        // --- MOVER ---
        else if (currentMode == Mode::MOVE) {
            if (statusMessage.find("base") != std::string::npos) {
                moveBasePoint = lastPoint;
                statusMessage = "MOVER | Especificar punto destino:";
            } else {
                // Calcular delta y aplicar a todas las entidades seleccionadas
                double dx = lastPoint.x - moveBasePoint.x;
                double dy = lastPoint.y - moveBasePoint.y;
                
                for (Entity* entity : selectedEntities) {
                    entity->move(dx, dy);
                }
                
                currentMode = Mode::IDLE;
                statusMessage = "Entidades movidas.";
            }
        }
        // --- COPIAR ---
        else if (currentMode == Mode::COPY) {
            if (statusMessage.find("base") != std::string::npos) {
                copyBasePoint = lastPoint;
                statusMessage = "COPIAR | Punto destino:";
            } else {
                double dx = lastPoint.x - copyBasePoint.x;
                double dy = lastPoint.y - copyBasePoint.y;
                // Clonar y mover cada entidad seleccionada
                for (Entity* e : selectedEntities) {
                    auto copy = e->clone();
                    copy->move(dx, dy);
                    doc.addEntity(std::move(copy));
                }
                currentMode = Mode::IDLE;
                statusMessage = "Entidades copiadas.";
            }
        }
        // --- ROTAR ---
        else if (currentMode == Mode::ROTATE) {
            // Búsqueda robusta (insensible a mayúsculas/minúsculas)
            std::string msgLower = statusMessage;
            std::transform(msgLower.begin(), msgLower.end(), msgLower.begin(), ::tolower);
            
            if (statusMessage.find("Centro") != std::string::npos) {
                rotateCenter = lastPoint;
                statusMessage = "ROTAR | Ángulo (grados) o punto:";
            } else {
                double angle;
                if (isScalar) {
                    angle = scalarValue;
                } else {
                    double dx = lastPoint.x - rotateCenter.x;
                    double dy = lastPoint.y - rotateCenter.y;
                    angle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                }
                // Aplicar rotacion a todas las entidades seleccionadas.
                for (Entity* e : selectedEntities) {
                    e->rotate(rotateCenter, angle);
                }
                currentMode = Mode::IDLE;
                // Usar ostringstream para evitar std::format
                std::ostringstream oss;
                oss << "Entidades rotadas " << angle << "°";
                statusMessage = oss.str();
            }
        }
        // --- ESCALAR ---
        else if (currentMode == Mode::SCALE) {
            if (statusMessage.find("base") != std::string::npos) {
                scaleBasePoint = lastPoint;
                statusMessage = "ESCALAR | Factor de escala (número) o dos puntos:";
            } else {
                double factor;
                if (isScalar) {
                    factor = scalarValue;
                } else {
                    // Si es un punto, calcular factor como distancia al base
                    // (simplificación: factor = distancia del punto al base)
                    double dx = lastPoint.x - scaleBasePoint.x;
                    double dy = lastPoint.y - scaleBasePoint.y;
                    factor = std::sqrt(dx * dx + dy * dy);
                    // Normalizar si es necesario (ej: factor = 1.0 si distancia = 100)
                    // Por ahora, usamos la distancia directa como factor
                }
                
                for (Entity* e : selectedEntities) {
                    e->scale(scaleBasePoint, factor);
                }
                
                currentMode = Mode::IDLE;
                statusMessage = "Entidades escaladas (factor: " + std::to_string(factor) + ")";
            }
        }
        // --- SIMETRIA (MIRROR) ---
        else if (currentMode == Mode::MIRROR) {
            if (statusMessage.find("Primer") != std::string::npos) {
                mirrorAxisP1 = lastPoint;
                statusMessage = "SIMETRIA | Segundo punto del eje:";
            } else {
                Point2D axisP2 = lastPoint;
                
                // Para cada entidad seleccionada, creamos una copia reflejada
                // (Si no tienes el método clone(), puedes modificar las originales directamente)
                for (Entity* e : selectedEntities) {
                    // Opción A: Modificar las originales (las mueve)
                    // e->mirror(mirrorAxisP1, axisP2);
                    
                    // Opción B: Crear copias reflejadas (recomendado)
                    // Necesitas el método clone() que vimos antes. Si no lo tienes, usa la Opción A.
                    auto copy = e->clone(); 
                    copy->mirror(mirrorAxisP1, axisP2);
                    doc.addEntity(std::move(copy));
                }
                
                currentMode = Mode::IDLE;
                statusMessage = "Simetría aplicada.";
            }
        }
        // --- MEDIR DISTANCIA ---
        else if (currentMode == Mode::MEASURE_DIST) {
            if (statusMessage.find("primer") != std::string::npos) {
                tempPoint1 = lastPoint;
                statusMessage = "DIST | Especificar segundo punto:";
            } else {
                double dx = lastPoint.x - tempPoint1.x;
                double dy = lastPoint.y - tempPoint1.y;
                double dist = std::sqrt(dx * dx + dy * dy);
                double angle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                
                // Mostrar en barra de estado
                //statusMessage = std::format("Distancia: {:.4f}, Ángulo XY: {:.2f}°, DX: {:.4f}, DY: {:.4f}", 
                //                            dist, angle, dx, dy);
                
                std::ostringstream ossDist;
                ossDist << std::fixed << std::setprecision(2);
                ossDist << "Distancia: " << dist 
                        << ", Angulo XY: " << angle << " deg"
                        << ", DX: " << dx 
                        << ", DY: " << dy;
                statusMessage = ossDist.str();

                // Opcional: añadir al historial de comandos para dejar constancia
                // (Esto requeriría que App tenga acceso, pero con statusMessage es suficiente por ahora)
                
                currentMode = Mode::IDLE;
            }
        }
        // --- TRIM ---
        else if (currentMode == Mode::TRIM) {
            if (trimSelectingBoundaries) {
                if (coordStr.empty()) {
                    // Terminar selección de cortes, pasar a seleccionar entidades a recortar
                    trimSelectingBoundaries = false;
                    statusMessage = "TRIM | Seleccionar entidades a recortar:";
                } else {
                    // Añadir entidad a cortes (usar último punto para buscar entidad cercana)
                    Entity* found = nullptr;
                    for (auto& entity : doc.entities) {
                        if (entity->isNear(lastPoint, 5.0 / viewScale)) { // tolerance hardcoded por ahora
                            found = entity.get();
                            break;
                        }
                    }
                    if (found) {
                        trimBoundaries.push_back(found);
                        statusMessage = "TRIM | Corte añadido (" + 
                                    std::to_string(trimBoundaries.size()) + " cortes). Enter para terminar:";
                    }
                }
            } else {
                // Seleccionar entidad a recortar y recortar al corte más cercano
                Entity* toTrim = nullptr;
                for (auto& entity : doc.entities) {
                    if (entity->isNear(lastPoint, 5.0 / viewScale)) {
                        toTrim = entity.get();
                        break;
                    }
                }
                
                if (toTrim && dynamic_cast<Line*>(toTrim)) {
                    Line* line = dynamic_cast<Line*>(toTrim);
                    Point2D closestCut;
                    double minDist = std::numeric_limits<double>::max();
                    
                    // Encontrar el corte más cercano a la línea
                    for (Entity* boundary : trimBoundaries) {
                        if (auto* boundaryLine = dynamic_cast<Line*>(boundary)) {
                            auto inter = lineLineIntersection(line->p1, line->p2, 
                                                            boundaryLine->p1, boundaryLine->p2);
                            if (inter.intersects && inter.param >= 0 && inter.param <= 1) {
                                double dist = std::hypot(inter.point.x - lastPoint.x, 
                                                        inter.point.y - lastPoint.y);
                                if (dist < minDist) {
                                    minDist = dist;
                                    closestCut = inter.point;
                                }
                            }
                        }
                    }
                    
                    if (minDist < std::numeric_limits<double>::max()) {
                        // Determinar qué extremo mantener
                        double d1 = std::hypot(closestCut.x - line->p1.x, closestCut.y - line->p1.y);
                        double d2 = std::hypot(closestCut.x - line->p2.x, closestCut.y - line->p2.y);
                        line->trim(closestCut, d1 < d2);
                        statusMessage = "TRIM | Entidad recortada.";
                    }
                }
            }
        }
        // --- EXTEND ---
        else if (currentMode == Mode::EXTEND) {
            if (extendSelectingBoundaries) {
                if (coordStr.empty()) {
                    extendSelectingBoundaries = false;
                    statusMessage = "EXTEND | Seleccionar entidades a alargar:";
                } else {
                    Entity* found = nullptr;
                    for (auto& entity : doc.entities) {
                        if (entity->isNear(lastPoint, 5.0 / viewScale)) {
                            found = entity.get();
                            break;
                        }
                    }
                    if (found) {
                        extendBoundaries.push_back(found);
                        statusMessage = "EXTEND | Borde añadido (" + 
                                    std::to_string(extendBoundaries.size()) + " bordes). Enter para terminar:";
                    }
                }
            } else {
                Entity* toExtend = nullptr;
                for (auto& entity : doc.entities) {
                    if (entity->isNear(lastPoint, 5.0 / viewScale)) {
                        toExtend = entity.get();
                        break;
                    }
                }
                
                if (toExtend && dynamic_cast<Line*>(toExtend)) {
                    Line* line = dynamic_cast<Line*>(toExtend);
                    Point2D closestBorder;
                    double minDist = std::numeric_limits<double>::max();
                    
                    for (Entity* boundary : extendBoundaries) {
                        if (auto* boundaryLine = dynamic_cast<Line*>(boundary)) {
                            auto inter = lineLineIntersection(line->p1, line->p2,
                                                            boundaryLine->p1, boundaryLine->p2);
                            if (inter.intersects) {
                                double dist = std::hypot(inter.point.x - lastPoint.x,
                                                        inter.point.y - lastPoint.y);
                                if (dist < minDist) {
                                    minDist = dist;
                                    closestBorder = inter.point;
                                }
                            }
                        }
                    }
                    
                    if (minDist < std::numeric_limits<double>::max()) {
                        line->extend(closestBorder);
                        statusMessage = "EXTEND | Entidad alargada.";
                    }
                }
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

    void Engine::clearSelection() {
        selectedEntities.clear();
    }

    void Engine::selectEntity(const Point2D& clickPoint, double tolerance) {
        // Buscar la entidad más cercana al click
        Entity* closest = nullptr;
        double minDist = tolerance + 1.0; // Inicialmente fuera de rango

        for (auto& entity : doc.entities) {
            if (entity->isNear(clickPoint, tolerance)) {
                // Calculamos distancia real para elegir la más cercana si hay solapamiento
                // (Simplificación: tomamos la primera que cumpla isNear)
                closest = entity.get();
                break; 
            }
        }

        if (closest) {
            // Si ya estaba seleccionada, la deseleccionamos (toggle)
            auto it = std::find(selectedEntities.begin(), selectedEntities.end(), closest);
            if (it != selectedEntities.end()) {
                selectedEntities.erase(it);
            } else {
                selectedEntities.push_back(closest);
            }
        } else {
            // Si no clicamos nada, limpiamos selección
            selectedEntities.clear();
        }
    }

    void Engine::deleteSelected() {
        if (selectedEntities.empty()) {
            statusMessage = "Nada seleccionado.";
            return;
        }
        // Eliminamos del vector principal de entidades
        doc.entities.erase(
            std::remove_if(doc.entities.begin(), doc.entities.end(),
                [this](const std::unique_ptr<Entity>& e) {
                    return std::find(selectedEntities.begin(), selectedEntities.end(), e.get()) != selectedEntities.end();
                }),
            doc.entities.end()
        );
        
        selectedEntities.clear();
        statusMessage = "Entidades borradas.";
    }

    std::string Engine::getHelpText(std::string_view topic) {
        std::string upperTopic(topic);
        std::transform(upperTopic.begin(), upperTopic.end(), upperTopic.begin(), ::toupper);
        upperTopic.erase(0, upperTopic.find_first_not_of(' '));
        upperTopic.erase(upperTopic.find_last_not_of(' ') + 1);

        std::ostringstream oss;
        // Si no hay tema específico, mostrar la ayuda general completa
        if (upperTopic.empty()) {
            //std::ostringstream oss;
            oss << "========================================\n";
            oss << "  CAD+ v1.0 - LISTA DE COMANDOS\n";
            oss << "========================================\n\n";
            
            oss << "[ DIBUJO ]\n";
            oss << "  L, LINEA      - Dibujar línea recta\n";
            oss << "  C, CIRCULO    - Dibujar círculo\n";
            oss << "  A, ARCO       - Dibujar arco\n";
            oss << "  PL, POLILINEA - Dibujar polilínea\n";
            oss << "  POL, POLIGONO - Dibujar polígono regular\n";
            oss << "  EL, ELIPSE    - Dibujar elipse\n\n";
            
            oss << "[ MODIFICACION ]\n";
            oss << "  M, MOVER      - Mover entidades\n";
            oss << "  CO, COPIAR    - Copiar entidades\n";
            oss << "  RO, ROTAR     - Rotar entidades\n";
            oss << "  SC, ESCALAR   - Escalar entidades\n";
            oss << "  SI, SIMETRIA  - Crear simetría (reflejo)\n";
            oss << "  TR, RECORTAR  - Recortar entidades\n";
            oss << "  EX, ALARGAR   - Alargar entidades\n\n";
            
            oss << "[ EDICION Y SISTEMA ]\n";
            oss << "  Z, BORRAR     - Borrar todo el dibujo\n";
            oss << "  LA, CAPA      - Gestionar capas (NEW, SET, ON, OFF, LIST)\n";
            oss << "  DIST, MEDIR   - Medir distancia y ángulo\n";
            oss << "  AYUDA, ?      - Mostrar esta ayuda\n\n";
            oss << "  SAVE, GUARDAR - Guardar dibujo en archivo JSON\n";
            oss << "  LOAD, CARGAR  - Cargar dibujo desde archivo JSON\n";
            
            oss << "[ PROXIMAMENTE ]\n";
            oss << "  AREA          - Calcular área y perímetro\n";
            oss << "  LISTA         - Listar propiedades de entidades\n";
            oss << "  GUARDAR       - Guardar dibujo en archivo\n";
            oss << "  CARGAR        - Cargar dibujo desde archivo\n";
            oss << "  DESHACER      - Deshacer última acción (Ctrl+Z)\n";
            oss << "  REHACER       - Rehacer última acción (Ctrl+Y)\n";
            oss << "========================================\n";
            oss << "TIP: Usa TAB para autocompletar y flechas para el historial.\n";
            oss << "========================================\n";
            return oss.str();
        }

        // Ayuda específica (ejemplos abreviados)
        if (upperTopic == "L" || upperTopic == "LINE" || upperTopic == "LINEA") {
            oss << "--- COMANDO: LINEA (L) ---\n";
            oss << "Dibuja una línea recta entre dos puntos.\n\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: L\n";
            oss << "  2. Especifica primer punto\n";
            oss << "  3. Especificar segundo punto\n\n";
            oss << "Ejemplos:\n";
            oss << "  L -> 0,0 -> 100,0\n";
            oss << "  L -> 0,0 -> @50,30\n";
            return oss.str();
        }
        else if (upperTopic == "C" || upperTopic == "CIRCLE" || upperTopic == "CIRCULO") {
            oss << "--- COMANDO: CIRCULO (C) ---\n";
            oss << "Dibuja un círculo especificando centro y radio.\n\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: C\n";
            oss << "  2. Especifica centro\n";
            oss << "  3. Especifica radio (número o punto)\n\n";
            oss << "Ejemplos:\n";
            oss << "  C -> 0,0 -> 50\n";
            oss << "  C -> 0,0 -> 100,0\n";
            return oss.str();
        }
        else if (upperTopic == "A" || upperTopic == "ARC" || upperTopic == "ARCO") {
            oss << "--- COMANDO: ARCO (A) ---\n";
            oss << "Dibuja un arco especificando centro, radio y ángulos.\n\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: A\n";
            oss << "  2. Especifica centro\n";
            oss << "  3. Especifica radio\n";
            oss << "  4. Especifica ángulo inicio (grados)\n";
            oss << "  5. Especifica ángulo final (grados)\n\n";
            oss << "Ejemplo: A -> 0,0 -> 50 -> 0 -> 180\n";
            return oss.str();
        }
        else if (upperTopic == "PL" || upperTopic == "POLILINEA") {
            oss << "--- COMANDO: POLILINEA (PL) ---\n";
            oss << "Dibuja una secuencia de segmentos conectados.\n\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: PL\n";
            oss << "  2. Especifica puntos sucesivos\n";
            oss << "  3. Termina: Enter (abierta) o C (cerrar)\n\n";
            oss << "Ejemplo: PL -> 0,0 -> 100,0 -> 100,100 -> C\n";
            return oss.str();
        }
        else if (upperTopic == "POL" || upperTopic == "POLIGONO") {
            oss << "--- COMANDO: POLIGONO (POL) ---\n";
            oss << "Dibuja un polígono regular.\n\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: POL\n";
            oss << "  2. Especifica centro\n";
            oss << "  3. Escribe número de lados\n";
            oss << "  4. Especifica radio\n\n";
            oss << "Ejemplo: POL -> 0,0 -> 6 -> 50\n";
            return oss.str();
        }
        else if (upperTopic == "EL" || upperTopic == "ELLIPSE" || upperTopic == "ELIPSE") {
            oss << "--- COMANDO: ELIPSE (EL) ---\n";
            oss << "Dibuja una elipse especificando centro y ejes.\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: EL\n";
            oss << "  2. Especifica centro\n";
            oss << "  3. Especifica punto final del eje mayor\n";
            oss << "  4. Especifica radio del otro eje\n";
            oss << "Ejemplo: EL -> 0,0 -> 100,0 -> 50\n";
            return oss.str();
        }
        else if (upperTopic == "LA" || upperTopic == "LAYER" || upperTopic == "CAPA") {
            oss << "--- COMANDO: CAPAS (LA) ---\n";
            oss << "Gestiona las capas del dibujo.\n\n";
            oss << "Subcomandos:\n";
            oss << "  LA NEW <nombre>    - Crear nueva capa\n";
            oss << "  LA SET <nombre>    - Establecer capa actual\n";
            oss << "  LA ON <nombre>     - Activar visibilidad\n";
            oss << "  LA OFF <nombre>    - Desactivar visibilidad\n";
            oss << "  LA LIST            - Listar capas\n\n";
            oss << "Ejemplo: LA NEW Muros\n";
            return oss.str();
        }
        else if (upperTopic == "DIST" || upperTopic == "MEDIR") {
            oss << "--- COMANDO: MEDIR (DIST) ---\n";
            oss << "Mide la distancia y ángulo entre dos puntos.\n\n";
            oss << "Uso:\n";
            oss << "  1. Escribe: DIST\n";
            oss << "  2. Especifica primer punto\n";
            oss << "  3. Especifica segundo punto\n\n";
            oss << "Muestra: Distancia total, ángulo en grados, delta X y delta Y.\n";
            return oss.str();
        }
        else {
            oss << "Comando no reconocido: " << topic << "\n";
            oss << "Usa HELP para ver comandos disponibles.\n";
            return oss.str();
        }
    }

    std::vector<std::string> Engine::getAllCommands() const {
        return {
            // Dibujo
            "LINEA", "CIRCULO", "ARCO", "POLILINEA", "POLIGONO", "ELIPSE",
            // Modificación
            "MOVER", "COPIAR", "ROTAR", "ESCALAR", "SIMETRIA", "RECORTAR", "ALARGAR",
            // Edición y Sistema
            "BORRAR", "CAPA", "MEDIR", "AYUDA", "GUARDAR", "CARGAR",
            // Futuras implementaciones (para la ayuda y autocompletado)
            "AREA", "LISTA", "GUARDAR", "CARGAR", "DESHACER", "REHACER", "EXPORTAR"
        };
    }

} // namespace cad