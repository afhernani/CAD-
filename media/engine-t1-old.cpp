#include "cad/commands/engine.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cad {

    void Engine::cancelCommand() {
        activeCommand_.reset();
        currentMode = Mode::IDLE;
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

        // 1. SI HAY UN COMANDO ACTIVO (Patrón Command), DELEGAR TODO (incluido Enter vacío)
        if (activeCommand_ && !activeCommand_->isComplete()) {
            activeCommand_->execute(cleanInput, *this);
            statusMessage = activeCommand_->getStatusMessage();
            
            // Si el comando ha terminado, resetear
            if (activeCommand_->isComplete()) {
                activeCommand_.reset();
                currentMode = Mode::IDLE;
            }
            return; // Salimos siempre si había un comando activo
        }

        // 2. DETECTAR HELP (solo si no hay comando activo)
        if (cleanInput.size() >= 4) {
            std::string upperClean(cleanInput);
            std::transform(upperClean.begin(), upperClean.end(), upperClean.begin(), ::toupper);
            if (upperClean.substr(0, 4) == "HELP" || upperClean.substr(0, 5) == "AYUDA") {
                std::string topic = "";
                if (upperClean.size() > 5) {
                    topic = cleanInput.substr(5);
                }
                getHelpText(topic);
                return;
            }
        }

        // 4. PROCESAMIENTO NORMAL (Nuevo comando o modos antiguos sin activeCommand_)
        if (currentMode == Mode::IDLE) {
            executeCommand(cleanInput);
        } else if (currentMode == Mode::LAYER_COMMAND) {
            processLayerCommand(cleanInput);
        } else if (currentMode == Mode::DIM_OPTIONS) {
            // Lógica de selección de tipo de cota (sin cambios)
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
                currentDimType = DimType::HORIZONTAL;
                currentMode = Mode::DRAW_DIMENSION;
                statusMessage = "COTA | Primer punto:";
            }
        } else {
            // Modos antiguos (como STRETCH o BLOCK si no se han migrado)
            processCoordinate(cleanInput);
        }
    }

    void Engine::executeCommand(std::string_view cmd) {
        std::string upperCmd(cmd);
        std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

        if (upperCmd == "L" || upperCmd == "LINE" || upperCmd == "LINEA") {
            activeCommand_ = std::make_unique<LineCommand>();
            currentMode = Mode::DRAW_LINE;  // ← MANTENER para feedback visual
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "C" || upperCmd == "CIRCLE" || upperCmd == "CIRCULO") {
            activeCommand_ = std::make_unique<CircleCommand>();
            currentMode = Mode::DRAW_CIRCLE;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "A" || upperCmd == "ARC" || upperCmd == "ARCO") {
            activeCommand_ = std::make_unique<ArcCommand>();
            currentMode = Mode::DRAW_ARC;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "PL" || upperCmd == "POLILINEA") {
            activeCommand_ = std::make_unique<PolylineCommand>();
            currentMode = Mode::DRAW_POLYLINE;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "POL" || upperCmd == "POLIGONO") {
            activeCommand_ = std::make_unique<PolygonCommand>();
            currentMode = Mode::DRAW_POLYGON;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "EL" || upperCmd == "ELLIPSE" || upperCmd == "ELIPSE") {
            activeCommand_ = std::make_unique<EllipseCommand>();
            currentMode = Mode::DRAW_ELLIPSE;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "DIM" || upperCmd == "COTA" || upperCmd == "ACOTAR") {
            currentMode = Mode::DIM_OPTIONS;
            currentDimType = DimType::HORIZONTAL; // Reseteamos al valor por defecto
            statusMessage = "COTA | [Alineada/Radio/Diámetro/Ángulo] <Horizontal>:";
        }
        else if (upperCmd == "Q" || upperCmd == "QUIT" || upperCmd == "SALIR") {
            statusMessage = "Usa el botón de cerrar ventana para salir.";
        }
        
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
                activeCommand_ = std::make_unique<MoveCommand>();
                currentMode = Mode::MOVE;
                statusMessage = activeCommand_->getStatusMessage();
            }
        }
        else if (upperCmd == "CO" || upperCmd == "COPY" || upperCmd == "COPIAR") {
            if (selectedEntities.empty()) {
                statusMessage = "COPIAR | Primero selecciona entidades (clic izquierdo).";
            } else {
                activeCommand_ = std::make_unique<CopyCommand>();
                currentMode = Mode::COPY;
                statusMessage = activeCommand_->getStatusMessage();
            }
        }
        else if (upperCmd == "RO" || upperCmd == "ROTATE" || upperCmd == "ROTAR") {
            if (selectedEntities.empty()) {
                statusMessage = "ROTAR | Primero selecciona entidades (clic izquierdo).";
            } else {
                activeCommand_ = std::make_unique<RotateCommand>();
                currentMode = Mode::ROTATE;
                statusMessage = activeCommand_->getStatusMessage();
            }
        }

        else if (upperCmd == "SC" || upperCmd == "SCALE" || upperCmd == "ESCALAR") {
            if (selectedEntities.empty()) {
                statusMessage = "ESCALAR | Primero selecciona entidades (clic izquierdo).";
            } else {
                activeCommand_ = std::make_unique<ScaleCommand>();
                currentMode = Mode::SCALE;
                statusMessage = activeCommand_->getStatusMessage();
            }
        }
        else if (upperCmd == "SI" || upperCmd == "SYM" || upperCmd == "MIRROR" || upperCmd == "SIMETRIA") {
            if (selectedEntities.empty()) {
                statusMessage = "SIMETRIA | Primero selecciona entidades (clic izquierdo).";
            } else {
                activeCommand_ = std::make_unique<MirrorCommand>();
                currentMode = Mode::MIRROR;
                statusMessage = activeCommand_->getStatusMessage();
            }
        }
        else if (upperCmd == "OF" || upperCmd == "OFFSET" || upperCmd == "DESPLAZAR") {
            activeCommand_ = std::make_unique<OffsetCommand>();
            currentMode = Mode::OFFSET;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "F" || upperCmd == "FILLET" || upperCmd == "EMPALME") {
            activeCommand_ = std::make_unique<FilletCommand>();
            currentMode = Mode::FILLET;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "CHA" || upperCmd == "CHAMFER" || upperCmd == "CHAFLAN") {
            activeCommand_ = std::make_unique<ChamferCommand>();
            currentMode = Mode::CHAMFER;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "TR" || upperCmd == "TRIM" || upperCmd == "RECORTAR") {
            activeCommand_ = std::make_unique<TrimCommand>();
            currentMode = Mode::TRIM;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "EX" || upperCmd == "EXTEND" || upperCmd == "ALARGAR") {
            activeCommand_ = std::make_unique<ExtendCommand>();
            currentMode = Mode::EXTEND;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "DIST" || upperCmd == "MEDIR") {
            activeCommand_ = std::make_unique<MeasureCommand>();
            currentMode = Mode::MEASURE_DIST;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "ARR" || upperCmd == "ARRAY") {
            activeCommand_ = std::make_unique<ArrayCommand>();
            currentMode = Mode::ARRAY;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "S" || upperCmd == "STRETCH" || upperCmd == "ESTIRAR") {
            activeCommand_ = std::make_unique<StretchCommand>();
            currentMode = Mode::STRETCH;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "GRID" || upperCmd == "REJILLA") {
            toggleGrid();
            statusMessage = gridEnabled ? "Rejilla activada." : "Rejilla desactivada.";
        }
        else if (upperCmd == "BLOCK" || upperCmd == "BLOQUE") {
            activeCommand_ = std::make_unique<BlockCreateCommand>();
            currentMode = Mode::BLOCK_CREATE;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "INSERT" || upperCmd == "INSERTAR") {
            activeCommand_ = std::make_unique<BlockInsertCommand>();
            currentMode = Mode::BLOCK_INSERT;
            statusMessage = activeCommand_->getStatusMessage();
        }
        else if (upperCmd == "GRIP" || upperCmd == "GRIPS" || upperCmd == "EDIT") {
            if (selectedEntities.empty()) {
                statusMessage = "GRIP EDIT | Primero selecciona entidades.";
            } else {
                currentMode = Mode::GRIP_EDIT;
                activeGripEntity = nullptr;
                activeGripIndex = -1;
                gripBackup.reset();
                statusMessage = "GRIP EDIT | Selecciona un grip para mover (clic izquierdo):";
            }
        }
        else if (upperCmd == "LIST" || upperCmd == "LISTA") {
            // El comando LISTA activa/desactiva el panel de propiedades
            // La lógica de toggle está en App
            statusMessage = "LISTA | Panel de propiedades activado/desactivado";
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
        // Si hay un comando activo, delegar a él
        if (activeCommand_ && !activeCommand_->isComplete()) {
            auto p = parseCoordinate(coordStr);
            if (p.has_value()) {
                activeCommand_->onPoint(p.value(), *this);
                statusMessage = activeCommand_->getStatusMessage();
                lastPoint = p.value();
                if (activeCommand_->isComplete()) {
                    activeCommand_.reset();
                    currentMode = Mode::IDLE;
                }
            }
            return;
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
            "DESPLAZAR", "CHAFLAN", "CHAMFER", "DESPLAZAR", "ARRAY", "MATRIZ", "STRETCH", "ESTIRAR",
            "BLOQUE", "BLOCK", "INSERTAR", "INSERT",
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

    std::string Engine::getEntityList() const {
        std::ostringstream oss;
        oss << "========================================\n";
        oss << "  LISTA DE ENTIDADES (" << doc.entities.size() << ")\n";
        oss << "========================================\n\n";
        
        if (doc.entities.empty()) {
            oss << "No hay entidades en el dibujo.\n";
            return oss.str();
        }
        
        int count = 0;
        for (const auto& entity : doc.entities) {
            count++;
            oss << "--- Entidad #" << count << " ---\n";
            
            if (auto* line = dynamic_cast<Line*>(entity.get())) {
                oss << "Tipo: LÍNEA\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Punto 1: (" << line->p1.x << ", " << line->p1.y << ")\n";
                oss << "  Punto 2: (" << line->p2.x << ", " << line->p2.y << ")\n";
                double dx = line->p2.x - line->p1.x;
                double dy = line->p2.y - line->p1.y;
                double len = std::sqrt(dx * dx + dy * dy);
                double angle = std::atan2(dy, dx) * 180.0 / std::numbers::pi;
                oss << "  Longitud: " << len << "\n";
                oss << "  Ángulo: " << angle << "°\n";
                oss << "  Capa: " << line->layerName << "\n\n";
            }
            else if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
                oss << "Tipo: CÍRCULO\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Centro: (" << circle->center.x << ", " << circle->center.y << ")\n";
                oss << "  Radio: " << circle->radius << "\n";
                oss << "  Diámetro: " << circle->radius * 2.0 << "\n";
                oss << "  Circunferencia: " << 2.0 * std::numbers::pi * circle->radius << "\n";
                oss << "  Área: " << std::numbers::pi * circle->radius * circle->radius << "\n";
                oss << "  Capa: " << circle->layerName << "\n\n";
            }
            else if (auto* arc = dynamic_cast<Arc*>(entity.get())) {
                oss << "Tipo: ARCO\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Centro: (" << arc->center.x << ", " << arc->center.y << ")\n";
                oss << "  Radio: " << arc->radius << "\n";
                oss << "  Ángulo inicio: " << arc->startAngle << "°\n";
                oss << "  Ángulo final: " << arc->endAngle << "°\n";
                double angleDiff = arc->endAngle - arc->startAngle;
                if (angleDiff < 0) angleDiff += 360.0;
                oss << "  Longitud de arco: " << (angleDiff / 360.0) * 2.0 * std::numbers::pi * arc->radius << "\n";
                oss << "  Capa: " << arc->layerName << "\n\n";
            }
            else if (auto* poly = dynamic_cast<Polyline*>(entity.get())) {
                oss << "Tipo: POLILÍNEA\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Número de vértices: " << poly->points.size() << "\n";
                double totalLen = 0.0;
                for (size_t i = 1; i < poly->points.size(); ++i) {
                    double dx = poly->points[i].x - poly->points[i-1].x;
                    double dy = poly->points[i].y - poly->points[i-1].y;
                    totalLen += std::sqrt(dx * dx + dy * dy);
                }
                oss << "  Longitud total: " << totalLen << "\n";
                oss << "  Cerrada: " << (poly->points.size() > 2 && 
                    poly->points.front().x == poly->points.back().x && 
                    poly->points.front().y == poly->points.back().y ? "Sí" : "No") << "\n";
                oss << "  Capa: " << poly->layerName << "\n\n";
            }
            else if (auto* polygon = dynamic_cast<Polygon*>(entity.get())) {
                oss << "Tipo: POLÍGONO\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Centro: (" << polygon->center.x << ", " << polygon->center.y << ")\n";
                oss << "  Lados: " << polygon->sides << "\n";
                oss << "  Radio: " << polygon->radius << "\n";
                double area = 0.5 * polygon->sides * polygon->radius * polygon->radius * 
                            std::sin(2.0 * std::numbers::pi / polygon->sides);
                oss << "  Área: " << area << "\n";
                oss << "  Perímetro: " << 2.0 * polygon->sides * polygon->radius * 
                    std::sin(std::numbers::pi / polygon->sides) << "\n";
                oss << "  Capa: " << polygon->layerName << "\n\n";
            }
            else if (auto* ellipse = dynamic_cast<Ellipse*>(entity.get())) {
                oss << "Tipo: ELIPSE\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Centro: (" << ellipse->center.x << ", " << ellipse->center.y << ")\n";
                oss << "  Eje mayor: " << ellipse->majorRadius << "\n";
                oss << "  Eje menor: " << ellipse->minorRadius << "\n";
                oss << "  Rotación: " << ellipse->rotationAngle * 180.0 / std::numbers::pi << "°\n";
                oss << "  Área: " << std::numbers::pi * ellipse->majorRadius * ellipse->minorRadius << "\n";
                oss << "  Capa: " << ellipse->layerName << "\n\n";
            }
            else if (auto* dim = dynamic_cast<Dimension*>(entity.get())) {
                oss << "Tipo: COTA\n";
                oss << std::fixed << std::setprecision(2);
                oss << "  Valor: " << dim->value << "\n";
                oss << "  Tipo: ";
                if (dim->type == DimType::HORIZONTAL) oss << "Horizontal";
                else if (dim->type == DimType::VERTICAL) oss << "Vertical";
                else if (dim->type == DimType::ALIGNED) oss << "Alineada";
                else if (dim->type == DimType::RADIUS) oss << "Radio";
                else if (dim->type == DimType::DIAMETER) oss << "Diámetro";
                else if (dim->type == DimType::ANGULAR) oss << "Angular";
                oss << "\n";
                oss << "  Capa: " << dim->layerName << "\n\n";
            }
            else {
                oss << "Tipo: DESCONOCIDA\n";
                oss << "  Capa: " << entity->layerName << "\n\n";
            }
        }
        
        return oss.str();
    }

} // namespace cad