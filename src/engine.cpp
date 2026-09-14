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
        tempOffsetDistance = 0.0;
        tempOffsetEntity = nullptr;
        tempFilletRadius = 0.0;
        tempFilletLine1 = nullptr;
        tempFilletLine2 = nullptr;
        // Reset variables for chamfer
        tempChamferDist1 = 0.0;
        tempChamferDist2 = 0.0;
        tempChamferLine1 = nullptr;
        tempChamferLine2 = nullptr;
        // Reset variables for Array
        tempArrayEntities.clear();
        tempArrayRows = 1; tempArrayCols = 1;
        tempArrayRowSpacing = 0.0; tempArrayColSpacing = 0.0;
        tempArrayCount = 1; tempArrayAngle = 360.0;
        tempArrayCenter = {0.0, 0.0};
        
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
        // >>> MANEJO CENTRALIZADO DEL ENTER VACÍO <<<
        if (cleanInput.empty()) {
            if (currentMode == Mode::DRAW_POLYLINE) {
                if (tempPolylinePoints.size() >= 2) {
                    auto newPoly = std::make_unique<Polyline>();
                    newPoly->points = tempPolylinePoints;
                    newPoly->layerName = doc.currentLayerName;
                    saveState();
                    doc.addEntity(std::move(newPoly));
                    statusMessage = "Polilínea terminada (" + std::to_string(tempPolylinePoints.size()) + " puntos).";
                } else {
                    statusMessage = "Polilínea cancelada (puntos insuficientes).";
                }
                tempPolylinePoints.clear();
                currentMode = Mode::IDLE;
            }
            else if (currentMode == Mode::TRIM || currentMode == Mode::EXTEND || 
                    currentMode == Mode::OFFSET || currentMode == Mode::FILLET ||
                    currentMode == Mode::CHAMFER || currentMode == Mode::ARRAY) {
                // Delegamos la lógica de terminación/cancelación a processCoordinate
                processCoordinate("");
            }
            // Para modo IDLE u otros, simplemente ignoramos el Enter vacío y no hacemos nada
            return;
        }
        if (currentMode == Mode::IDLE) {
            executeCommand(cleanInput);
        } else if (currentMode == Mode::LAYER_COMMAND) {
            processLayerCommand(cleanInput);
        }
        // >>> Gestionar el menú de opciones de COTA <<<
        else if (currentMode == Mode::DIM_OPTIONS) {
            std::string upperInput = cleanInput;
            std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
            
            if (upperInput == "A" || upperInput == "ALINEADA") {
                currentDimType = DimType::ALIGNED;
                currentMode = Mode::DRAW_DIM_ALIGNED;
                statusMessage = "COTA ALINEADA | Selecciona línea o primer punto:";
            } else if (upperInput == "R" || upperInput == "RADIO") {
                currentDimType = DimType::RADIUS;
                currentMode = Mode::DRAW_DIM_RADIUS;
                statusMessage = "COTA RADIO | Selecciona círculo o arco:";
            } else if (upperInput == "D" || upperInput == "DIAMETRO") {
                currentDimType = DimType::DIAMETER;
                currentMode = Mode::DRAW_DIM_DIAMETER;
                statusMessage = "COTA DIÁMETRO | Selecciona círculo o arco:";
            } else if (upperInput == "AN" || upperInput == "ANGULO") {
                currentDimType = DimType::ANGULAR;
                currentMode = Mode::DRAW_DIM_ANGULAR;
                statusMessage = "COTA ANGULAR | Selecciona la primera línea:";
            } else {
                // Por defecto (Enter o cualquier otra cosa): Horizontal/Vertical
                currentDimType = DimType::HORIZONTAL;
                currentMode = Mode::DRAW_DIMENSION;
                statusMessage = "COTA | Primer punto:";
            }
        } 
        else {
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
        else if (upperCmd == "DIM" || upperCmd == "COTA" || upperCmd == "ACOTAR") {
            currentMode = Mode::DIM_OPTIONS;
            currentDimType = DimType::HORIZONTAL; // Reseteamos al valor por defecto
            statusMessage = "COTA | [Alineada/Radio/Diámetro/Ángulo] <Horizontal>:";
        }
        else if (upperCmd == "Q" || upperCmd == "QUIT" || upperCmd == "SALIR") {
            statusMessage = "Usa el botón de cerrar ventana para salir.";
        }
        // else if (upperCmd == "ESC" || upperCmd == "CANCEL" || upperCmd == "CANCELAR") {
        //     cancelCommand();
        // }
        else if (upperCmd == "Z" || upperCmd == "BORRAR") {
            saveState();
            doc.clear();
            lastPoint = {0.0, 0.0};
            statusMessage = "Dibujo borrado.";
        }
        else if (upperCmd == "UNDO" || upperCmd == "DESHACER") {
            undo();
        }
        else if (upperCmd == "REDO" || upperCmd == "REHACER") {
            redo();
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
        else if (upperCmd == "OF" || upperCmd == "OFFSET" || upperCmd == "DESPLAZAR") {
            currentMode = Mode::OFFSET;
            statusMessage = "OFFSET | Especificar distancia de desplazamiento:";
        }
        else if (upperCmd == "F" || upperCmd == "FILLET" || upperCmd == "EMPALME") {
            currentMode = Mode::FILLET;
            tempFilletLine1 = nullptr;
            tempFilletLine2 = nullptr;
            statusMessage = "FILLET | Especificar radio (0 para esquina viva):";
        }
        else if (upperCmd == "CHA" || upperCmd == "CHAMFER" || upperCmd == "CHAFLAN") {
            currentMode = Mode::CHAMFER;
            tempChamferLine1 = nullptr;
            tempChamferLine2 = nullptr;
            statusMessage = "CHAFLAN | Especificar primera distancia (0 para esquina viva):";
        }
        else if (upperCmd == "ARR" || upperCmd == "ARRAY") {
            currentMode = Mode::ARRAY;
            tempArrayEntities.clear();
            tempArrayRows = 1; tempArrayCols = 1;
            tempArrayRowSpacing = 0.0; tempArrayColSpacing = 0.0;
            tempArrayCount = 1; tempArrayAngle = 360.0;
            tempArrayCenter = {0.0, 0.0};
            statusMessage = "ARRAY | Selecciona entidades (clic) y pulsa Enter para continuar:";
        }
        else if (upperCmd == "GRID" || upperCmd == "REJILLA") {
            toggleGrid();
            statusMessage = gridEnabled ? "Rejilla activada." : "Rejilla desactivada.";
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
                    saveState();
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
            else if (currentMode == Mode::OFFSET) {
                currentMode = Mode::IDLE;
                tempOffsetDistance = 0.0;
                tempOffsetEntity = nullptr;
                statusMessage = "OFFSET terminado.";
            }
            else if (currentMode == Mode::FILLET) {
                currentMode = Mode::IDLE;
                tempFilletRadius = 0.0;
                tempFilletLine1 = nullptr;
                tempFilletLine2 = nullptr;
                statusMessage = "FILLET cancelado.";
            }
            else if (currentMode == Mode::CHAMFER) {
                currentMode = Mode::IDLE;
                tempChamferDist1 = 0.0;
                tempChamferDist2 = 0.0;
                tempChamferLine1 = nullptr;
                tempChamferLine2 = nullptr;
                statusMessage = "CHAFLAN cancelado.";
            }
            else if (currentMode == Mode::ARRAY) {
                if (tempArrayEntities.empty()) {
                    statusMessage = "ARRAY | No hay entidades seleccionadas. Comando cancelado.";
                    currentMode = Mode::IDLE;
                } else {
                    statusMessage = "ARRAY | Elige tipo [Rectangular/Polar] (Escribe R o P):";
                }
            }
            // else {
            //     cancelCommand();
            // }
            // Para otros modos: no hacer nada, mantener estado
            return;
        }
        // --- ARRAY: Manejar R/P antes de parsear coordenadas ---
        if (currentMode == Mode::ARRAY && !tempArrayEntities.empty()) {
            std::string upperStr(coordStr);
            std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
            
            if (upperStr == "R" || upperStr == "RECTANGULAR") {
                tempArrayType = ArrayType::RECTANGULAR;
                statusMessage = "ARRAY RECTANGULAR | Punto base:";
                return; // Salir aquí para no continuar con el parseo
            }
            else if (upperStr == "P" || upperStr == "POLAR") {
                tempArrayType = ArrayType::POLAR;
                statusMessage = "ARRAY POLAR | Punto base:";
                return; // Salir aquí
            }
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
                    saveState();
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
                
                // Ejemplo para Línea:
                saveState(); // << AÑADIR ESTO
                
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
                saveState();
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
                saveState();
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
                saveState();
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
                saveState();
                doc.addEntity(std::move(newEllipse));
                
                currentMode = Mode::IDLE;
                statusMessage = "Elipse creada.";
            }
        }
        // --- COTA (DIMENSION) ---
        else if (currentMode == Mode::DRAW_DIMENSION) {
            if (statusMessage.find("Primer") != std::string::npos) {
                tempDimP1 = lastPoint;
                statusMessage = "COTA | Segundo punto:";
            }
            else if (statusMessage.find("Segundo") != std::string::npos) {
                tempDimP2 = lastPoint;
                statusMessage = "COTA | Ubicación de la línea de cota:";
            }
            else {
                Point2D loc = lastPoint;
                // Determinar si es horizontal o vertical
                double cx = (tempDimP1.x + tempDimP2.x) / 2.0;
                double cy = (tempDimP1.y + tempDimP2.y) / 2.0;
                bool isHoriz = std::abs(loc.y - cy) > std::abs(loc.x - cx);
                
                double val = isHoriz ? std::abs(tempDimP2.x - tempDimP1.x) : std::abs(tempDimP2.y - tempDimP1.y);
                
                auto newDim = std::make_unique<Dimension>();
                newDim->p1 = tempDimP1; newDim->p2 = tempDimP2;
                newDim->location = loc; newDim->isHorizontal = isHoriz;
                newDim->value = val; newDim->layerName = doc.currentLayerName;
                
                saveState(); // Para deshacer
                doc.addEntity(std::move(newDim));
                currentMode = Mode::IDLE;
                statusMessage = "Cota creada.";
            }
        }
        // --- COTA ALINEADA (permite seleccionar línea o picar puntos) ---
        else if (currentMode == Mode::DRAW_DIM_ALIGNED) {
            if (statusMessage.find("primer") != std::string::npos) {
                // Intentar detectar una línea cercana al clic
                Entity* foundLine = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            foundLine = entity.get();
                            tempDimP1 = line->p1;
                            tempDimP2 = line->p2;
                            break;
                        }
                    }
                }
                if (foundLine) {
                    // Línea detectada: saltar directamente a pedir ubicación
                    statusMessage = "COTA ALINEADA | Ubicación:";
                } else {
                    // No hay línea: modo manual, pedir primer punto
                    tempDimP1 = lastPoint;
                    statusMessage = "COTA ALINEADA | Segundo punto:";
                }
            }
            else if (statusMessage.find("Segundo") != std::string::npos) {
                tempDimP2 = lastPoint;
                statusMessage = "COTA ALINEADA | Ubicación:";
            }
            else {
                // Crear la cota
                Point2D loc = lastPoint;
                double val = std::hypot(tempDimP2.x - tempDimP1.x, tempDimP2.y - tempDimP1.y);
                auto newDim = std::make_unique<Dimension>();
                newDim->p1 = tempDimP1; newDim->p2 = tempDimP2;
                newDim->location = loc;
                newDim->isAligned = true;
                newDim->type = DimType::ALIGNED;
                newDim->value = val;
                newDim->layerName = doc.currentLayerName;
                saveState();
                doc.addEntity(std::move(newDim));
                currentMode = Mode::IDLE;
                statusMessage = "Cota alineada creada.";
            }
        }
        // --- COTA RADIO ---
        else if (currentMode == Mode::DRAW_DIM_RADIUS) {
            // PASO 1: Seleccionar círculo
            if (statusMessage.find("Selecciona") != std::string::npos || 
                statusMessage.find("Centro") != std::string::npos) {
                
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                        double dist = std::hypot(lastPoint.x - circle->center.x, lastPoint.y - circle->center.y);
                        if (dist < minDist || std::abs(dist - circle->radius) < minDist) {
                            found = entity.get();
                            tempDimP1 = circle->center; 
                            // Guardamos el radio en tempDimP2.x temporalmente
                            tempDimP2 = {circle->radius, 0.0}; 
                            break;
                        }
                    }
                    else if (auto* arc = dynamic_cast<Arc*>(entity.get())) {
                        double distCenter = std::hypot(lastPoint.x - arc->center.x,
                                                    lastPoint.y - arc->center.y);
                        // Detectar si clicamos cerca del centro o del borde del arco
                        if (distCenter < minDist || std::abs(distCenter - arc->radius) < minDist) {
                            found = entity.get();
                            tempDimP1 = arc->center;
                            // Usar el ángulo medio del arco para el punto de referencia
                            double midAngle = (arc->startAngle + arc->endAngle) / 2.0;
                            double rad = midAngle * std::numbers::pi / 180.0;
                            tempDimP2 = {arc->radius, 0.0}; // Guardamos el radio en tempDimP2.x
                            break;
                        }
                    }
                }
                
                if (found) statusMessage = "COTA RADIO | Ubicación de la cota:";
                else statusMessage = "COTA RADIO | No se encontró figura. Centro manual:";
            } 
            // PASO 2: Colocar cota (Calcular dirección hacia el clic)
            else {
                double radius = tempDimP2.x; // Recuperamos el radio guardado
                
                // Calcular vector dirección desde el centro hacia donde hizo clic el usuario
                double dx = lastPoint.x - tempDimP1.x;
                double dy = lastPoint.y - tempDimP1.y;
                double len = std::hypot(dx, dy);
                
                Point2D borderPoint;
                if (len > 0) {
                    // Normalizar y multiplicar por radio para obtener el punto exacto en el borde
                    borderPoint = { tempDimP1.x + (dx / len) * radius, tempDimP1.y + (dy / len) * radius };
                } else {
                    borderPoint = { tempDimP1.x + radius, tempDimP1.y }; // Fallback si clicó en el centro
                }

                auto newDim = std::make_unique<Dimension>();
                newDim->p1 = tempDimP1;      // Centro
                newDim->p2 = borderPoint;    // Punto en el borde (dirección del ratón)
                newDim->location = lastPoint; // Donde va el texto
                newDim->type = DimType::RADIUS;
                newDim->value = radius;
                newDim->layerName = doc.currentLayerName;
                
                saveState();
                doc.addEntity(std::move(newDim));
                currentMode = Mode::IDLE;
                statusMessage = "Cota de radio creada.";
            }
        }
        // --- COTA DIÁMETRO ---
        else if (currentMode == Mode::DRAW_DIM_DIAMETER) {
            // PASO 1: Seleccionar círculo o arco
            if (statusMessage.find("Selecciona") != std::string::npos || 
                statusMessage.find("Centro") != std::string::npos) {
                
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                
                for (auto& entity : doc.entities) {
                    if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                        double dist = std::hypot(lastPoint.x - circle->center.x, lastPoint.y - circle->center.y);
                        if (dist < minDist || std::abs(dist - circle->radius) < minDist) {
                            found = entity.get();
                            tempDimP1 = circle->center; 
                            // Guardamos el radio en tempDimP2.x temporalmente
                            tempDimP2 = {circle->radius, 0.0}; 
                            break;
                        }
                    }
                    else if (auto* arc = dynamic_cast<Arc*>(entity.get())) {
                        double distCenter = std::hypot(lastPoint.x - arc->center.x, lastPoint.y - arc->center.y);
                        if (distCenter < minDist || std::abs(distCenter - arc->radius) < minDist) {
                            found = entity.get();
                            tempDimP1 = arc->center;
                            // Guardamos el radio en tempDimP2.x
                            tempDimP2 = {arc->radius, 0.0}; 
                            break;
                        }
                    }
                }
                
                if (found) {
                    statusMessage = "COTA DIÁMETRO | Ubicación de la cota:";
                } else {
                    statusMessage = "COTA DIÁMETRO | No se encontró figura. Centro manual:";
                }
            } 
            // PASO 2: Colocar cota (La línea debe pasar por el centro)
            else {
                double radius = tempDimP2.x; // Recuperamos el radio guardado
                Point2D center = tempDimP1;  // Recuperamos el centro
                
                // Calcular vector dirección desde el centro hacia donde hizo clic el usuario
                double dx = lastPoint.x - center.x;
                double dy = lastPoint.y - center.y;
                double len = std::hypot(dx, dy);
                
                // Normalizar la dirección (evitar división por cero)
                double nx = (len > 0) ? (dx / len) : 1.0;
                double ny = (len > 0) ? (dy / len) : 0.0;
                
                // Calcular los dos puntos opuestos en el borde de la figura
                Point2D p1_border = { center.x + nx * radius, center.y + ny * radius };
                Point2D p2_border = { center.x - nx * radius, center.y - ny * radius };

                auto newDim = std::make_unique<Dimension>();
                newDim->p1 = p1_border;      // Un extremo del diámetro
                newDim->p2 = p2_border;      // El extremo opuesto (la línea pasará por el centro)
                newDim->location = lastPoint; // Donde va el texto
                newDim->type = DimType::DIAMETER;
                newDim->value = radius * 2.0;
                newDim->layerName = doc.currentLayerName;
                
                saveState();
                doc.addEntity(std::move(newDim));
                currentMode = Mode::IDLE;
                statusMessage = "Cota de diámetro creada.";
            }
        }
        // --- COTA ANGULAR ---
        else if (currentMode == Mode::DRAW_DIM_ANGULAR) {
            // PASO 1: Seleccionar primera línea
            if (statusMessage.find("primera") != std::string::npos ||
                statusMessage.find("Primera") != std::string::npos) {
                
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            found = entity.get();
                            tempDimP1 = line->p1; // Guardar puntos de línea 1
                            tempDimP2 = line->p2;
                            break;
                        }
                    }
                }
                if (found) {
                    statusMessage = "COTA ANGULAR | Selecciona la segunda línea:";
                } else {
                    statusMessage = "COTA ANGULAR | No es una línea. Selecciona la primera línea:";
                }
            }
            // PASO 2: Seleccionar segunda línea y calcular intersección
            else if (statusMessage.find("segunda") != std::string::npos ||
                    statusMessage.find("Segunda") != std::string::npos) {
                
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            found = entity.get();
                            auto inter = lineLineIntersection(tempDimP1, tempDimP2, line->p1, line->p2);
                            if (inter.intersects) {
                                // GUARDAR CORRECTAMENTE:
                                tempDimP1 = inter.point;           // Vértice (centro del arco)
                                tempDimP2 = tempDimP2;             // Punto dirección línea 1
                                tempDimP2_line2 = line->p2;        // Punto dirección línea 2
                                
                                // Calcular ángulo
                                double dx1 = tempDimP2.x - tempDimP1.x;
                                double dy1 = tempDimP2.y - tempDimP1.y;
                                double dx2 = line->p2.x - tempDimP1.x;
                                double dy2 = line->p2.y - tempDimP1.y;
                                double angle = std::atan2(dy2, dx2) - std::atan2(dy1, dx1);
                                if (angle < 0) angle += 2 * std::numbers::pi;
                                tempDimAngle = angle * 180.0 / std::numbers::pi;
                                
                                statusMessage = "COTA ANGULAR | Ubicación del arco de cota:";
                            } else {
                                statusMessage = "COTA ANGULAR | Líneas paralelas. Intenta de nuevo.";
                            }
                            break;
                        }
                    }
                }
                if (!found && statusMessage.find("paralelas") == std::string::npos &&
                    statusMessage.find("Ubicación") == std::string::npos) {
                    statusMessage = "COTA ANGULAR | No es una línea. Selecciona la segunda línea:";
                }
            }
            // PASO 3: Colocar el arco
            else {
                auto newDim = std::make_unique<Dimension>();
                newDim->location = tempDimP1;          // VÉRTICE (centro del arco)
                newDim->p1 = tempDimP2;                // Dirección línea 1
                newDim->p2 = tempDimP2_line2;          // Dirección línea 2
                newDim->p3 = lastPoint;                // Solo para radio
                newDim->type = DimType::ANGULAR;
                newDim->value = tempDimAngle;
                newDim->layerName = doc.currentLayerName;
                
                saveState();
                doc.addEntity(std::move(newDim));
                currentMode = Mode::IDLE;
                statusMessage = "Cota angular creada.";
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
                saveState();
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
                saveState();
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
                    saveState();
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
                
                saveState();
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
                    saveState();  // Guardar estado antes de recortar
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
                    saveState();  // Guardar estado antes de alargar
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
        // --- OFFSET ---
        else if (currentMode == Mode::OFFSET) {
            // PASO 1: Segundo punto para distancia
            if (statusMessage.find("Segundo") != std::string::npos ||
                    statusMessage.find("segundo") != std::string::npos) {
                double dx = lastPoint.x - tempOffsetP1.x;
                double dy = lastPoint.y - tempOffsetP1.y;
                tempOffsetDistance = std::sqrt(dx * dx + dy * dy);
                statusMessage = "OFFSET | Seleccionar entidad a desplazar:";
            }
            // PASO 1b: Esperando distancia
            else if (statusMessage.find("distancia") != std::string::npos ||
                statusMessage.find("Distancia") != std::string::npos) {
                if (isScalar) {
                    tempOffsetDistance = scalarValue;
                    statusMessage = "OFFSET | Seleccionar entidad a desplazar:";
                } else {
                    // Si no es escalar, usar como primer punto para medir distancia
                    tempOffsetP1 = lastPoint;
                    statusMessage = "OFFSET | Segundo punto para definir distancia:";
                }
            }
            // PASO 2: Seleccionar entidad
            else if (statusMessage.find("Seleccionar") != std::string::npos ||
                    statusMessage.find("seleccionar") != std::string::npos) {
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (entity->isNear(lastPoint, minDist)) {
                        found = entity.get();
                        tempOffsetEntity = entity.get();
                        break;
                    }
                }
                if (found) {
                    statusMessage = "OFFSET | Indicar lado del desplazamiento:";
                } else {
                    statusMessage = "OFFSET | No se encontró entidad. Intenta de nuevo:";
                }
            }
            // PASO 3: Indicar lado y crear entidad paralela
            else {
                if (tempOffsetEntity) {
                    saveState();
                    
                    // Crear entidad paralela según el tipo
                    if (auto* line = dynamic_cast<Line*>(tempOffsetEntity)) {
                        // Calcular línea paralela
                        double dx = line->p2.x - line->p1.x;
                        double dy = line->p2.y - line->p1.y;
                        double len = std::sqrt(dx * dx + dy * dy);
                        if (len > 0) {
                            // Normal perpendicular
                            double nx = -dy / len;
                            double ny = dx / len;
                            
                            // Determinar lado según clic
                            double vx = lastPoint.x - line->p1.x;
                            double vy = lastPoint.y - line->p1.y;
                            double side = vx * nx + vy * ny;
                            double sign = (side >= 0) ? 1.0 : -1.0;
                            
                            auto newLine = std::make_unique<Line>();
                            newLine->p1 = {line->p1.x + nx * tempOffsetDistance * sign,
                                        line->p1.y + ny * tempOffsetDistance * sign};
                            newLine->p2 = {line->p2.x + nx * tempOffsetDistance * sign,
                                        line->p2.y + ny * tempOffsetDistance * sign};
                            newLine->layerName = doc.currentLayerName;
                            doc.addEntity(std::move(newLine));
                        }
                    }
                    else if (auto* circle = dynamic_cast<Circle*>(tempOffsetEntity)) {
                        // Círculo concéntrico con radio modificado
                        auto newCircle = std::make_unique<Circle>();
                        newCircle->center = circle->center;
                        newCircle->radius = circle->radius + tempOffsetDistance;
                        if (newCircle->radius < 0) newCircle->radius = std::abs(newCircle->radius);
                        newCircle->layerName = doc.currentLayerName;
                        doc.addEntity(std::move(newCircle));
                    }
                    else if (auto* arc = dynamic_cast<Arc*>(tempOffsetEntity)) {
                        // Arco concéntrico con radio modificado
                        auto newArc = std::make_unique<Arc>();
                        newArc->center = arc->center;
                        newArc->radius = arc->radius + tempOffsetDistance;
                        newArc->startAngle = arc->startAngle;
                        newArc->endAngle = arc->endAngle;
                        if (newArc->radius < 0) newArc->radius = std::abs(newArc->radius);
                        newArc->layerName = doc.currentLayerName;
                        doc.addEntity(std::move(newArc));
                    }
                    
                    statusMessage = "OFFSET | Entidad desplazada. Seleccionar otra entidad o ESC para terminar:";
                    tempOffsetEntity = nullptr; // Resetear para permitir seleccionar otra
                }
            }
        }
        // --- FILLET (EMPALME) ---
        else if (currentMode == Mode::FILLET) {
            // PASO 1: Radio
            if (statusMessage.find("radio") != std::string::npos || statusMessage.find("Radio") != std::string::npos) {
                if (isScalar) {
                    tempFilletRadius = scalarValue;
                    statusMessage = "FILLET | Seleccionar primera línea:";
                } else {
                    statusMessage = "FILLET | Valor no válido. Especificar radio:";
                }
            }
            // PASO 2: Primera línea
            else if (statusMessage.find("primera") != std::string::npos || statusMessage.find("Primera") != std::string::npos) {
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            found = entity.get();
                            tempFilletLine1 = line;
                            break;
                        }
                    }
                }
                if (found) statusMessage = "FILLET | Seleccionar segunda línea:";
                else statusMessage = "FILLET | No es una línea. Seleccionar primera línea:";
            }
            // PASO 3: Segunda línea y calcular empalme
            else if (statusMessage.find("segunda") != std::string::npos || statusMessage.find("Segunda") != std::string::npos) {
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            found = entity.get();
                            tempFilletLine2 = line;
                            break;
                        }
                    }
                }
                
                if (found && tempFilletLine1 && tempFilletLine2) {
                    // Calcular intersección
                    auto inter = lineLineIntersection(tempFilletLine1->p1, tempFilletLine1->p2, 
                                                    tempFilletLine2->p1, tempFilletLine2->p2);
                    if (inter.intersects) {
                        saveState();
                        Point2D I = inter.point;
                        
                        // Vectores dirección de las líneas (normalizados)
                        auto normalize = [](Point2D a, Point2D b) {
                            double dx = b.x - a.x, dy = b.y - a.y;
                            double len = std::sqrt(dx*dx + dy*dy);
                            return len > 0 ? Point2D{dx/len, dy/len} : Point2D{0,0};
                        };
                        
                        // Determinar qué extremos de las líneas están más cerca de la intersección
                        // (Asumimos que el usuario quiere fillet en los extremos cercanos a I)
                        double d1a = std::hypot(tempFilletLine1->p1.x - I.x, tempFilletLine1->p1.y - I.y);
                        double d1b = std::hypot(tempFilletLine1->p2.x - I.x, tempFilletLine1->p2.y - I.y);
                        Point2D& end1 = (d1a < d1b) ? tempFilletLine1->p1 : tempFilletLine1->p2;
                        Point2D& far1 = (d1a < d1b) ? tempFilletLine1->p2 : tempFilletLine1->p1;
                        
                        double d2a = std::hypot(tempFilletLine2->p1.x - I.x, tempFilletLine2->p1.y - I.y);
                        double d2b = std::hypot(tempFilletLine2->p2.x - I.x, tempFilletLine2->p2.y - I.y);
                        Point2D& end2 = (d2a < d2b) ? tempFilletLine2->p1 : tempFilletLine2->p2;
                        Point2D& far2 = (d2a < d2b) ? tempFilletLine2->p2 : tempFilletLine2->p1;
                        
                        // Vectores desde I hacia los extremos a recortar
                        Point2D v1 = normalize(I, end1);
                        Point2D v2 = normalize(I, end2);
                        
                        // Ángulo entre los vectores
                        double cosAngle = v1.x * v2.x + v1.y * v2.y;
                        // Limitar para evitar errores de precisión
                        if (cosAngle > 1.0) cosAngle = 1.0;
                        if (cosAngle < -1.0) cosAngle = -1.0;
                        double angle = std::acos(cosAngle);
                        
                        if (angle > 0.001) { // Si no son paralelas
                            // Distancia desde I a los puntos tangentes
                            double d = tempFilletRadius / std::tan(angle / 2.0);
                            
                            // Nuevos extremos de las líneas (puntos tangentes)
                            Point2D T1 = {I.x + v1.x * d, I.y + v1.y * d};
                            Point2D T2 = {I.x + v2.x * d, I.y + v2.y * d};
                            
                            // Actualizar líneas originales (recortar)
                            end1 = T1;
                            end2 = T2;
                            
                            // Dibujar arco si radio > 0
                            if (tempFilletRadius > 0.001) {
                                // Centro del arco: en la bisectriz
                                Point2D bisector = {v1.x + v2.x, v1.y + v2.y};
                                double bisLen = std::sqrt(bisector.x*bisector.x + bisector.y*bisector.y);
                                if (bisLen > 0) {
                                    bisector.x /= bisLen; bisector.y /= bisLen;
                                    double h = tempFilletRadius / std::sin(angle / 2.0);
                                    Point2D center = {I.x + bisector.x * h, I.y + bisector.y * h};
                                    
                                    // Calcular ángulos inicio/fin para el arco
                                    double a1 = std::atan2(T1.y - center.y, T1.x - center.x);
                                    double a2 = std::atan2(T2.y - center.y, T2.x - center.x);
                                    
                                    // Asegurar que el arco vaya en la dirección correcta (el más corto)
                                    double diff = a2 - a1;
                                    while (diff < 0) diff += 2 * std::numbers::pi;
                                    while (diff >= 2 * std::numbers::pi) diff -= 2 * std::numbers::pi;
                                    if (diff > std::numbers::pi) std::swap(a1, a2);
                                    
                                    auto newArc = std::make_unique<Arc>();
                                    newArc->center = center;
                                    newArc->radius = tempFilletRadius;
                                    newArc->startAngle = a1 * 180.0 / std::numbers::pi;
                                    newArc->endAngle = a2 * 180.0 / std::numbers::pi;
                                    newArc->layerName = doc.currentLayerName;
                                    doc.addEntity(std::move(newArc));
                                }
                            }
                            statusMessage = "FILLET | Empalme creado. Seleccionar primera línea o ESC:";
                        } else {
                            statusMessage = "FILLET | Líneas paralelas. Seleccionar primera línea:";
                        }
                    } else {
                        statusMessage = "FILLET | Líneas paralelas. Seleccionar primera línea:";
                    }
                } else {
                    statusMessage = "FILLET | No es una línea. Seleccionar segunda línea:";
                }
                // Resetear para permitir múltiples empalmes
                tempFilletLine1 = nullptr;
                tempFilletLine2 = nullptr;
            }
        }
        // --- CHAMFER (CHAFLÁN) ---
        else if (currentMode == Mode::CHAMFER) {
            // PASO 4: Segunda línea (debe ir ANTES que PASO 2)
            if (statusMessage.find("Segunda línea") != std::string::npos ||
                statusMessage.find("segunda línea") != std::string::npos) {
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            found = entity.get();
                            tempChamferLine2 = line;
                            break;
                        }
                    }
                }
                
                if (found && tempChamferLine1) {
                    auto inter = lineLineIntersection(tempChamferLine1->p1, tempChamferLine1->p2,
                                                    tempChamferLine2->p1, tempChamferLine2->p2);
                    if (inter.intersects) {
                        saveState();
                        Point2D I = inter.point;
                        auto normalize = [](Point2D a, Point2D b) {
                            double dx = b.x - a.x, dy = b.y - a.y;
                            double len = std::sqrt(dx*dx + dy*dy);
                            return len > 0 ? Point2D{dx/len, dy/len} : Point2D{0,0};
                        };
                        double d1a = std::hypot(tempChamferLine1->p1.x - I.x, tempChamferLine1->p1.y - I.y);
                        double d1b = std::hypot(tempChamferLine1->p2.x - I.x, tempChamferLine1->p2.y - I.y);
                        Point2D& end1 = (d1a < d1b) ? tempChamferLine1->p1 : tempChamferLine1->p2;
                        
                        double d2a = std::hypot(tempChamferLine2->p1.x - I.x, tempChamferLine2->p1.y - I.y);
                        double d2b = std::hypot(tempChamferLine2->p2.x - I.x, tempChamferLine2->p2.y - I.y);
                        Point2D& end2 = (d2a < d2b) ? tempChamferLine2->p1 : tempChamferLine2->p2;
                        
                        Point2D v1 = normalize(I, end1);
                        Point2D v2 = normalize(I, end2);
                        
                        Point2D T1 = {I.x + v1.x * tempChamferDist1, I.y + v1.y * tempChamferDist1};
                        Point2D T2 = {I.x + v2.x * tempChamferDist2, I.y + v2.y * tempChamferDist2};
                        
                        end1 = T1;
                        end2 = T2;
                        
                        if (tempChamferDist1 > 0.001 || tempChamferDist2 > 0.001) {
                            auto newLine = std::make_unique<Line>();
                            newLine->p1 = T1;
                            newLine->p2 = T2;
                            newLine->layerName = doc.currentLayerName;
                            doc.addEntity(std::move(newLine));
                        }
                        statusMessage = "CHAFLAN | Selecciona la primera línea (o Enter para terminar):";
                    } else {
                        statusMessage = "CHAFLAN | Líneas paralelas. Selecciona la primera línea:";
                    }
                    // Resetear SOLO si tuvo éxito
                    tempChamferLine1 = nullptr;
                    tempChamferLine2 = nullptr;
                } else {
                    // CORRECCIÓN: Si falla, NO reseteamos tempChamferLine1 para que el usuario pueda intentarlo de nuevo
                    statusMessage = "CHAFLAN | No se detectó la línea. Intenta seleccionar la segunda línea de nuevo:";
                }
            }
            // PASO 3: Primera línea
            else if (statusMessage.find("primera línea") != std::string::npos ||
                    statusMessage.find("Primera línea") != std::string::npos) {
                Entity* found = nullptr;
                double minDist = 10.0 / viewScale;
                for (auto& entity : doc.entities) {
                    if (auto* line = dynamic_cast<Line*>(entity.get())) {
                        if (line->isNear(lastPoint, minDist)) {
                            found = entity.get();
                            tempChamferLine1 = line;
                            break;
                        }
                    }
                }
                if (found) statusMessage = "CHAFLAN | Segunda línea:";
                else statusMessage = "CHAFLAN | No es una línea. Selecciona la primera línea:";
            }
            // PASO 1: Primera distancia
            else if (statusMessage.find("Primera distancia") != std::string::npos ||
                    statusMessage.find("primera distancia") != std::string::npos) {
                if (isScalar) {
                    tempChamferDist1 = scalarValue;
                    statusMessage = "CHAFLAN | Segunda distancia:";
                } else {
                    statusMessage = "CHAFLAN | Valor no válido. Primera distancia:";
                }
            }
            // PASO 2: Segunda distancia
            else if (statusMessage.find("Segunda distancia") != std::string::npos ||
                    statusMessage.find("segunda distancia") != std::string::npos) {
                if (isScalar) {
                    tempChamferDist2 = scalarValue;
                    statusMessage = "CHAFLAN | Primera línea:";
                } else {
                    statusMessage = "CHAFLAN | Valor no válido. Segunda distancia:";
                }
            }
        }
        // --- ARRAY ---
        else if (currentMode == Mode::ARRAY) {
            std::string upperStr(coordStr);
            std::transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);
            
            // PASO 1: Elegir tipo (R o P)
            if (statusMessage.find("Tipo") != std::string::npos ||
                statusMessage.find("tipo") != std::string::npos ||
                statusMessage.find("Rectangular") != std::string::npos ||
                statusMessage.find("Polar") != std::string::npos) {
                if (upperStr == "R" || upperStr == "RECTANGULAR") {
                    tempArrayType = ArrayType::RECTANGULAR;
                    statusMessage = "ARRAY RECTANGULAR | Punto base:";  // ← NUEVO: Pedir punto base primero
                }
                else if (upperStr == "P" || upperStr == "POLAR") {
                    tempArrayType = ArrayType::POLAR;
                    statusMessage = "ARRAY POLAR | Punto base (centro de rotación):";  // ← NUEVO: Pedir punto base primero
                }
                else {
                    statusMessage = "ARRAY | Tipo no válido. Escribe R o P:";
                }
            }
            // PASO 2: Flujo Rectangular (con punto base)
            else if (tempArrayType == ArrayType::RECTANGULAR) {
                // Sub-paso 2a: Punto base
                if (statusMessage.find("Punto base") != std::string::npos ||
                    statusMessage.find("punto base") != std::string::npos) {
                    tempArrayBasePoint = lastPoint;  // ← Guardar punto base
                    statusMessage = "ARRAY RECTANGULAR | Número de filas:";
                }
                // Sub-paso 2b: Número de filas
                else if (statusMessage.find("Número de filas") != std::string::npos && isScalar) {
                    tempArrayRows = std::max(1, (int)scalarValue);
                    statusMessage = "ARRAY RECTANGULAR | Número de columnas:";
                }
                // Sub-paso 2c: Número de columnas
                else if (statusMessage.find("Número de columnas") != std::string::npos && isScalar) {
                    tempArrayCols = std::max(1, (int)scalarValue);
                    statusMessage = "ARRAY RECTANGULAR | Espaciado entre filas:";
                }
                // Sub-paso 2d: Espaciado entre filas
                else if (statusMessage.find("Espaciado entre filas") != std::string::npos && isScalar) {
                    tempArrayRowSpacing = scalarValue;
                    statusMessage = "ARRAY RECTANGULAR | Espaciado entre columnas:";
                }
                // Sub-paso 2e: Espaciado entre columnas y ejecutar
                else if (statusMessage.find("Espaciado entre columnas") != std::string::npos && isScalar) {
                    tempArrayColSpacing = scalarValue;
                    saveState();
                    int created = 0;
                    for (int r = 0; r < tempArrayRows; ++r) {
                        for (int c = 0; c < tempArrayCols; ++c) {
                            if (r == 0 && c == 0) continue;  // Saltar la posición original
                            double offX = c * tempArrayColSpacing;
                            double offY = r * tempArrayRowSpacing;
                            for (Entity* e : tempArrayEntities) {
                                auto copy = e->clone();
                                // Desplazamiento simple desde la posición original
                                copy->move(offX, offY);
                                copy->layerName = doc.currentLayerName;
                                doc.addEntity(std::move(copy));
                                created++;
                            }
                        }
                    }
                    statusMessage = "ARRAY | Creadas " + std::to_string(created) + " copias rectangulares.";
                    tempArrayEntities.clear();
                    currentMode = Mode::IDLE;
                }
                else {
                    statusMessage = "ARRAY | Introduce un valor numérico válido.";
                }
            }
            // PASO 3: Flujo Polar (con punto base = centro)
            else if (tempArrayType == ArrayType::POLAR) {
                // Sub-paso 3a: Punto base (centro de rotación)
                if (statusMessage.find("Punto base") != std::string::npos ||
                    statusMessage.find("punto base") != std::string::npos ) {
                    tempArrayCenter = lastPoint;  // ← Guardar centro
                    statusMessage = "ARRAY POLAR | Número de copias:";
                }
                // Sub-paso 3b: Número de copias
                else if (statusMessage.find("Número de copias") != std::string::npos && isScalar) {
                    tempArrayCount = std::max(2, (int)scalarValue);
                    statusMessage = "ARRAY POLAR | Ángulo total (grados, 360=completo):";
                }
                // Sub-paso 3c: Ángulo total y ejecutar
                else if (statusMessage.find("Ángulo total") != std::string::npos && isScalar) {
                    tempArrayAngle = scalarValue;
                    saveState();
                    int created = 0;
                    double angleStep = (tempArrayCount > 1) ? (tempArrayAngle / (tempArrayCount - 1)) : 0.0;
                    for (int i = 1; i < tempArrayCount; ++i) {
                        double ang = i * angleStep;
                        for (Entity* e : tempArrayEntities) {
                            auto copy = e->clone();
                            copy->rotate(tempArrayCenter, ang);
                            copy->layerName = doc.currentLayerName;
                            doc.addEntity(std::move(copy));
                            created++;
                        }
                    }
                    statusMessage = "ARRAY | Creadas " + std::to_string(created) + " copias polares.";
                    tempArrayEntities.clear();
                    currentMode = Mode::IDLE;
                }
                else {
                    statusMessage = "ARRAY | Introduce un valor numérico válido.";
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
        saveState();  // Guardamos estado antes de borrar
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
            
            oss << "[ COTA ]\n";
            oss << "  DIM, COTA     - Dibujar cota (dimensión)\n\n";
            
            oss << "[ MODIFICACION ]\n";
            oss << "  M, MOVER      - Mover entidades\n";
            oss << "  CO, COPIAR    - Copiar entidades\n";
            oss << "  RO, ROTAR     - Rotar entidades\n";
            oss << "  SC, ESCALAR   - Escalar entidades\n";
            oss << "  SI, SIMETRIA  - Crear simetría (reflejo)\n";
            oss << "  TR, RECORTAR  - Recortar entidades\n";
            oss << "  EX, ALARGAR   - Alargar entidades\n\n";
            oss << "  AR, ARRAY     - Crear matriz de copias (Rectangular/Polar)\n";
            
            oss << "[ EDICION Y SISTEMA ]\n";
            oss << "  Z, BORRAR     - Borrar todo el dibujo\n";
            oss << "  LA, CAPA      - Gestionar capas (NEW, SET, ON, OFF, LIST)\n";
            oss << "  DIST, MEDIR   - Medir distancia y ángulo\n";
            oss << "  GRID, REJILLA   - Activar/desactivar cuadrícula de fondo\n";
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
            "LINEA", "CIRCULO", "ARCO", "POLILINEA", "POLIGONO", "ELIPSE", "COTA", "ACOTAR", "DIM", "DIST", "MEDIR",
            // Modificación
            "MOVER", "COPIAR", "ROTAR", "ESCALAR", "SIMETRIA", "RECORTAR", "ALARGAR", "OFFSET", "FILLET", "EMPALME", 
            "DESPLAZAR", "CHAFLAN", "CHAMFER", "DESPLAZAR", "ARRAY", "MATRIZ",
            // Edición y Sistema
            "BORRAR", "CAPA", "MEDIR", "AYUDA", "GUARDAR", "CARGAR", "DESHACER", "REHACER", "EXPORTAR", "GRID", "REJILLA",
            // Futuras implementaciones (para la ayuda y autocompletado)
            "AREA", "LISTA", "GUARDAR", "CARGAR", "DESHACER", "REHACER", "EXPORTAR"
        };
    }

    void Engine::saveState() {
        // 1. Clonar todas las entidades actuales
        std::vector<std::unique_ptr<Entity>> state;
        for (const auto& e : doc.entities) {
            state.push_back(e->clone());
        }
        
        // 2. Guardar en la pila de deshacer
        undoStack.push_back(std::move(state));
        
        // 3. Si hacemos una acción nueva, el historial de "rehacer" se borra
        redoStack.clear();
        
        // 4. Limitar el historial a 50 estados para no consumir mucha memoria RAM
        if (undoStack.size() > 50) {
            undoStack.erase(undoStack.begin());
        }
    }

    void Engine::undo() {
        if (undoStack.empty()) {
            statusMessage = "No hay nada que deshacer.";
            return;
        }
        
        // Guardar el estado actual en la pila de rehacer
        std::vector<std::unique_ptr<Entity>> currentState;
        for (const auto& e : doc.entities) {
            currentState.push_back(e->clone());
        }
        redoStack.push_back(std::move(currentState));

        // Restaurar el estado anterior
        doc.entities.clear();
        auto& prevState = undoStack.back();
        for (auto& e : prevState) {
            doc.entities.push_back(e->clone());
        }
        undoStack.pop_back();
        
        selectedEntities.clear(); // Limpiar selección al cambiar el estado
        statusMessage = "Deshacer.";
    }

    void Engine::redo() {
        if (redoStack.empty()) {
            statusMessage = "No hay nada que rehacer.";
            return;
        }
        
        // Guardar el estado actual en la pila de deshacer
        std::vector<std::unique_ptr<Entity>> currentState;
        for (const auto& e : doc.entities) {
            currentState.push_back(e->clone());
        }
        undoStack.push_back(std::move(currentState));

        // Restaurar el estado siguiente
        doc.entities.clear();
        auto& nextState = redoStack.back();
        for (auto& e : nextState) {
            doc.entities.push_back(e->clone());
        }
        redoStack.pop_back();
        
        selectedEntities.clear();
        statusMessage = "Rehacer.";
    }

} // namespace cad