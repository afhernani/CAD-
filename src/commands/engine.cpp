#include "cad/commands/engine.hpp"
// >>> AQUÍ VAN LOS INCLUDES ESPECÍFICOS (El .cpp sí puede conocer los detalles)
// Comandos de dibujo
#include "cad/commands/draw/line_command.hpp"
#include "cad/commands/draw/circle_command.hpp"
#include "cad/commands/draw/arc_command.hpp"
#include "cad/commands/draw/polyline_command.hpp"
#include "cad/commands/draw/polygon_command.hpp"
#include "cad/commands/draw/ellipse_command.hpp"
#include "cad/commands/draw/dimension_command.hpp"
#include "cad/commands/draw/text_command.hpp"

// Comandos de modificación
#include "cad/commands/modify/move_command.hpp"
#include "cad/commands/modify/copy_command.hpp"
#include "cad/commands/modify/rotate_command.hpp"
#include "cad/commands/modify/scale_command.hpp"
#include "cad/commands/modify/mirror_command.hpp"
#include "cad/commands/modify/offset_command.hpp"
#include "cad/commands/modify/fillet_command.hpp"
#include "cad/commands/modify/chamfer_command.hpp"
#include "cad/commands/modify/trim_command.hpp"
#include "cad/commands/modify/extend_command.hpp"
#include "cad/commands/modify/measure_command.hpp"
#include "cad/commands/modify/array_command.hpp"
#include "cad/commands/modify/stretch_command.hpp"

// Comandos de bloque
#include "cad/commands/block/block_create_command.hpp"
#include "cad/commands/block/block_insert_command.hpp"

#include <algorithm>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace cad {
	
	Engine::Engine() {
       // ... tus inicializaciones ...
       initializeCommands(); // ¡IMPORTANTE!
   }

	void Engine::cancelCommand() {
		activeCommand_.reset();
		currentMode = Mode::IDLE;
		statusMessage = "Comando cancelado.";
	}

	bool Engine::isNumericValue(std::string_view str) const {
		std::string s(str);
		s.erase(0, s.find_first_not_of(' '));
		s.erase(s.find_last_not_of(' ') + 1);
		if (s.empty()) return false;
		if (s.find(',') != std::string::npos || s.find('@') != std::string::npos || s.find('<') != std::string::npos) return false;
		try { std::stod(s); return true; } catch (...) { return false; }
	}

	void Engine::processInput(std::string_view input) {
		std::string cleanInput(input);
		cleanInput.erase(0, cleanInput.find_first_not_of(' '));
		cleanInput.erase(cleanInput.find_last_not_of(' ') + 1);

		// 1. SI HAY UN COMANDO ACTIVO, DELEGAR TODO (incluido Enter vacío)
		if (activeCommand_ && !activeCommand_->isComplete()) {
			activeCommand_->execute(cleanInput, *this);
			statusMessage = activeCommand_->getStatusMessage();
			if (activeCommand_->isComplete()) {
				activeCommand_.reset();
				currentMode = Mode::IDLE;
			}
			return;
		}

		// 2. DETECTAR HELP
		if (cleanInput.size() >= 4) {
			std::string upperClean(cleanInput);
			std::transform(upperClean.begin(), upperClean.end(), upperClean.begin(), ::toupper);
			if (upperClean.substr(0, 4) == "HELP" || upperClean.substr(0, 5) == "AYUDA") {
				std::string topic = (upperClean.size() > 5) ? cleanInput.substr(5) : "";
				getHelpText(topic);
				return;
			}
		}

		// 3. PROCESAMIENTO NORMAL
		if (currentMode == Mode::IDLE) {
			executeCommand(cleanInput);
		} else if (currentMode == Mode::LAYER_COMMAND) {
			processLayerCommand(cleanInput);
		}else {
			processCoordinate(cleanInput);
		}
	}

	void Engine::executeCommand(std::string_view cmd) {
		std::string upperCmd(cmd);
		std::transform(upperCmd.begin(), upperCmd.end(), upperCmd.begin(), ::toupper);

		// 1. Intentar ejecutar como comando registrado en la fábrica
        auto it = commandRegistry_.find(upperCmd);
        if (it != commandRegistry_.end()) {
            auto newCommand = it->second(*this); // Llama a la fábrica pasando 'this'
            if (newCommand) {
                activeCommand_ = std::move(newCommand);
                statusMessage = activeCommand_->getStatusMessage();
            }
            // Si newCommand es nullptr, la fábrica ya estableció el statusMessage (ej. "Primero selecciona entidades")
            return;
        }

        // 2. Comandos del sistema que NO son ICommand
        if (upperCmd == "Q" || upperCmd == "QUIT" || upperCmd == "SALIR") {
            statusMessage = "Usa el botón de cerrar ventana para salir."; return;
        } 
        if (upperCmd == "Z" || upperCmd == "BORRAR") {
            saveState(); doc.clear(); lastPoint = {0.0, 0.0}; statusMessage = "Dibujo borrado."; return;
        } 
        if (upperCmd == "UNDO" || upperCmd == "DESHACER") { undo(); return; }
        if (upperCmd == "REDO" || upperCmd == "REHACER") { redo(); return; }
        if (upperCmd == "AXIS" || upperCmd == "EJES") {
            statusMessage = "Usa el boton en la barra de herramientas para activar/desactivar ejes"; return;
        } 
        if (upperCmd == "LA" || upperCmd == "LAYER" || upperCmd == "CAPA") {
            currentMode = Mode::LAYER_COMMAND; statusMessage = "CAPA | ON <nombre> | OFF <nombre> | NEW <nombre> | SET <nombre> | LIST"; return;
        } 
        if (upperCmd == "GRID" || upperCmd == "REJILLA") {
            toggleGrid(); statusMessage = gridEnabled ? "Rejilla activada." : "Rejilla desactivada."; return;
        } 
        if (upperCmd == "LIST" || upperCmd == "LISTA") {
            statusMessage = "LISTA | Panel de propiedades activado/desactivado"; return;
        } 
        if (upperCmd == "HELP" || upperCmd == "AYUDA" || upperCmd == "?") {
            statusMessage = "Ayuda: escribe HELP <comando> para más detalles"; return;
        }

        // 3. Comando desconocido
        statusMessage = "Comando desconocido: " + std::string(cmd);
	}

	void Engine::processLayerCommand(std::string_view input) {
		std::string upperInput(input);
		std::transform(upperInput.begin(), upperInput.end(), upperInput.begin(), ::toupper);
		std::istringstream iss(upperInput);
		std::string subCmd, layerName;
		iss >> subCmd >> layerName;

		if (subCmd == "NEW" || subCmd == "NUEVA") {
			if (!layerName.empty()) { doc.addLayer(layerName); statusMessage = "Capa '" + layerName + "' creada."; }
			else { statusMessage = "Error: Especifica un nombre."; }
		} else if (subCmd == "SET" || subCmd == "ACTUAL") {
			if (!layerName.empty() && doc.layers.find(layerName) != doc.layers.end()) {
				doc.setCurrentLayer(layerName); statusMessage = "Capa actual: '" + layerName + "'.";
			} else { statusMessage = "Error: Capa no encontrada."; }
		} else if (subCmd == "ON" || subCmd == "ENCENDER") {
			if (!layerName.empty()) { doc.setLayerVisibility(layerName, true); statusMessage = "Capa '" + layerName + "' activada."; }
		} else if (subCmd == "OFF" || subCmd == "APAGAR") {
			if (!layerName.empty()) { doc.setLayerVisibility(layerName, false); statusMessage = "Capa '" + layerName + "' desactivada."; }
		} else if (subCmd == "LIST" || subCmd == "LISTA") {
			std::string list = "Capas: ";
			for (const auto& pair : doc.layers) list += pair.first + (pair.second.isCurrent ? " (Actual) " : " ");
			statusMessage = list;
		} else { statusMessage = "Subcomando no reconocido."; }
		currentMode = Mode::IDLE;
	}

	void Engine::processCoordinate(std::string_view coordStr) {
		if (activeCommand_ && !activeCommand_->isComplete()) {
			auto p = parseCoordinate(coordStr);
			if (p.has_value()) {
				activeCommand_->onPoint(p.value(), *this);
				statusMessage = activeCommand_->getStatusMessage();
				lastPoint = p.value();
				if (activeCommand_->isComplete()) { activeCommand_.reset(); currentMode = Mode::IDLE; }
			}
			return;
		}
		// Modos antiguos sin comando activo (si los hubiera) irían aquí.
	}

	std::optional<Point2D> Engine::parseCoordinate(std::string_view str) {
		Point2D p{0.0, 0.0};
		std::string_view s = str;
		bool isRelative = false;
		if (s.empty()) return std::nullopt;
		if (s[0] == '@') { isRelative = true; s = s.substr(1); }
		try {
			size_t anglePos = s.find('<');
			if (anglePos != std::string_view::npos) {
				double dist = std::stod(std::string(s.substr(0, anglePos)));
				double angleDeg = std::stod(std::string(s.substr(anglePos + 1)));
				double angleRad = angleDeg * std::numbers::pi / 180.0;
				double dx = dist * std::cos(angleRad), dy = dist * std::sin(angleRad);
				p.x = isRelative ? lastPoint.x + dx : dx;
				p.y = isRelative ? lastPoint.y + dy : dy;
			} else {
				size_t commaPos = s.find(',');
				if (commaPos != std::string_view::npos) {
					double x = std::stod(std::string(s.substr(0, commaPos)));
					double y = std::stod(std::string(s.substr(commaPos + 1)));
					p.x = isRelative ? lastPoint.x + x : x;
					p.y = isRelative ? lastPoint.y + y : y;
				} else {
					p.x = std::stod(std::string(s)); p.y = 0.0;
				}
			}
		} catch (...) { statusMessage = "Error: Formato de coordenada inválido."; return std::nullopt; }
		return p;
	}

	void Engine::clearSelection() { selectedEntities.clear(); }

	void Engine::selectEntity(const Point2D& clickPoint, double tolerance) {
		Entity* closest = nullptr;
		for (auto& entity : doc.entities) {
			if (entity->isNear(clickPoint, tolerance)) { closest = entity.get(); break; }
		}
		if (closest) {
			auto it = std::find(selectedEntities.begin(), selectedEntities.end(), closest);
			if (it != selectedEntities.end()) selectedEntities.erase(it);
			else selectedEntities.push_back(closest);
		} else { selectedEntities.clear(); }
	}

	void Engine::deleteSelected() {
		if (selectedEntities.empty()) { statusMessage = "Nada seleccionado."; return; }
		saveState();
		doc.entities.erase(std::remove_if(doc.entities.begin(), doc.entities.end(),
			[this](const std::unique_ptr<Entity>& e) {
				return std::find(selectedEntities.begin(), selectedEntities.end(), e.get()) != selectedEntities.end();
			}), doc.entities.end());
		selectedEntities.clear();
		statusMessage = "Entidades borradas.";
	}
	//
	void Engine::performWindowSelection(const Point2D& p1, const Point2D& p2, bool addToSelection) {
		selectionManager.selectByWindow(p1, p2, doc.entities, selectedEntities, addToSelection, doc);
		statusMessage = std::to_string(selectedEntities.size()) + " entidades seleccionadas.";
	}

	std::string Engine::getHelpText(std::string_view topic, int maxCharsPerLine) {
        std::string upperTopic(topic);
        std::transform(upperTopic.begin(), upperTopic.end(), upperTopic.begin(), ::toupper);
        upperTopic.erase(0, upperTopic.find_first_not_of(' '));
        upperTopic.erase(upperTopic.find_last_not_of(' ') + 1);

        std::ostringstream oss;

        if (upperTopic.empty()) {
            oss << "COMANDOS DISPONIBLES:\n";
            
            std::vector<std::string> commands;
            for (const auto& pair : commandRegistry_) {
                commands.push_back(pair.first);
            }
            std::sort(commands.begin(), commands.end());

            // >>> USAR maxCharsPerLine en lugar de 80 fijo <<<
            std::string currentLine = "";
            
            for (size_t i = 0; i < commands.size(); ++i) {
                const std::string& cmd = commands[i];
                std::string separator = (i < commands.size() - 1) ? ", " : "";
                int neededSpace = static_cast<int>(cmd.length() + separator.length());
                
                if (!currentLine.empty() && 
                    (static_cast<int>(currentLine.length()) + neededSpace > maxCharsPerLine)) {
                    oss << currentLine << "\n";
                    currentLine = "";
                }
                currentLine += cmd + separator;
            }
            
            if (!currentLine.empty()) {
                oss << currentLine << "\n";
            }
            
            return oss.str();
        }

        auto it = commandRegistry_.find(upperTopic);
        if (it != commandRegistry_.end()) {
            oss << "El comando '" << upperTopic << "' está registrado y listo para usarse.\n";
            return oss.str();
        }

        oss << "Comando no reconocido: '" << topic << "'\n";
        oss << "Usa HELP para ver la lista completa de comandos válidos.\n";
        return oss.str();
    }

	std::vector<std::string> Engine::getAllCommands() const {
		return {"LINEA", "CIRCULO", "ARCO", "POLILINEA", "POLIGONO", "ELIPSE", "COTA", "ACOTAR", "DIM", "DIST", "MEDIR",
				"MOVER", "COPIAR", "ROTAR", "ESCALAR", "SIMETRIA", "RECORTAR", "ALARGAR", "OFFSET", "FILLET", "EMPALME",
				"CHAFLAN", "CHAMFER", "ARRAY", "MATRIZ", "STRETCH", "ESTIRAR", "BLOQUE", "BLOCK", "INSERTAR", "INSERT",
				"BORRAR", "CAPA", "AYUDA", "GUARDAR", "CARGAR", "DESHACER", "REHACER", "GRID", "REJILLA", "AREA", "LISTA"};
	}

	void Engine::saveState() {
		std::vector<std::unique_ptr<Entity>> state;
		for (const auto& e : doc.entities) state.push_back(e->clone());
		undoStack.push_back(std::move(state));
		redoStack.clear();
		if (undoStack.size() > 50) undoStack.erase(undoStack.begin());
	}

	void Engine::undo() {
		if (undoStack.empty()) { statusMessage = "No hay nada que deshacer."; return; }
		std::vector<std::unique_ptr<Entity>> currentState;
		for (const auto& e : doc.entities) currentState.push_back(e->clone());
		redoStack.push_back(std::move(currentState));
		doc.entities.clear();
		for (auto& e : undoStack.back()) doc.entities.push_back(e->clone());
		undoStack.pop_back();
		selectedEntities.clear();
		statusMessage = "Deshacer.";
	}

	void Engine::redo() {
		if (redoStack.empty()) { statusMessage = "No hay nada que rehacer."; return; }
		std::vector<std::unique_ptr<Entity>> currentState;
		for (const auto& e : doc.entities) currentState.push_back(e->clone());
		undoStack.push_back(std::move(currentState));
		doc.entities.clear();
		for (auto& e : redoStack.back()) doc.entities.push_back(e->clone());
		redoStack.pop_back();
		selectedEntities.clear();
		statusMessage = "Rehacer.";
	}

	std::string Engine::getEntityList() const {
		std::ostringstream oss;
		oss << "========================================\n  LISTA DE ENTIDADES (" << doc.entities.size() << ")\n========================================\n";
		if (doc.entities.empty()) { oss << "No hay entidades en el dibujo.\n"; return oss.str(); }
		int count = 0;
		for (const auto& entity : doc.entities) {
			count++;
			oss << "--- Entidad #" << count << " ---\n";
			if (auto* line = dynamic_cast<Line*>(entity.get())) {
				oss << "Tipo: LÍNEA\n  Capa: " << line->layerName << "\n";
			} else if (auto* circle = dynamic_cast<Circle*>(entity.get())) {
				oss << "Tipo: CÍRCULO\n  Radio: " << circle->radius << "\n  Capa: " << circle->layerName << "\n";
			} else {
				oss << "Tipo: DESCONOCIDA\n  Capa: " << entity->layerName << "\n";
			}
		}
		return oss.str();
	}

	void Engine::registerCommand(const std::string& name, CommandFactory factory) {
        std::string upperName = name;
        std::transform(upperName.begin(), upperName.end(), upperName.begin(), ::toupper);
        commandRegistry_[upperName] = factory;
    }

	// Implementación del método estático
	bool Engine::checkSelection(Engine& eng) {
		if (eng.selectedEntities.empty()) {
			eng.statusMessage = "Primero selecciona entidades.";
			return false;
		}
		return true;
	}

    void Engine::initializeCommands() {
        // --- COMANDOS DE DIBUJO ---
        registerCommand("L", [](Engine& eng) { eng.currentMode = Mode::DRAW_LINE; return std::make_unique<LineCommand>(); });
        registerCommand("LINE", [](Engine& eng) { eng.currentMode = Mode::DRAW_LINE; return std::make_unique<LineCommand>(); });
        registerCommand("LINEA", [](Engine& eng) { eng.currentMode = Mode::DRAW_LINE; return std::make_unique<LineCommand>(); });

        registerCommand("C", [](Engine& eng) { eng.currentMode = Mode::DRAW_CIRCLE; return std::make_unique<CircleCommand>(); });
        registerCommand("CIRCLE", [](Engine& eng) { eng.currentMode = Mode::DRAW_CIRCLE; return std::make_unique<CircleCommand>(); });
        registerCommand("CIRCULO", [](Engine& eng) { eng.currentMode = Mode::DRAW_CIRCLE; return std::make_unique<CircleCommand>(); });

        registerCommand("A", [](Engine& eng) { eng.currentMode = Mode::DRAW_ARC; return std::make_unique<ArcCommand>(); });
        registerCommand("ARC", [](Engine& eng) { eng.currentMode = Mode::DRAW_ARC; return std::make_unique<ArcCommand>(); });
        registerCommand("ARCO", [](Engine& eng) { eng.currentMode = Mode::DRAW_ARC; return std::make_unique<ArcCommand>(); });

        registerCommand("PL", [](Engine& eng) { eng.currentMode = Mode::DRAW_POLYLINE; return std::make_unique<PolylineCommand>(); });
        registerCommand("POLILINEA", [](Engine& eng) { eng.currentMode = Mode::DRAW_POLYLINE; return std::make_unique<PolylineCommand>(); });

        registerCommand("POL", [](Engine& eng) { eng.currentMode = Mode::DRAW_POLYGON; return std::make_unique<PolygonCommand>(); });
        registerCommand("POLIGONO", [](Engine& eng) { eng.currentMode = Mode::DRAW_POLYGON; return std::make_unique<PolygonCommand>(); });

        registerCommand("EL", [](Engine& eng) { eng.currentMode = Mode::DRAW_ELLIPSE; return std::make_unique<EllipseCommand>(); });
        registerCommand("ELLIPSE", [](Engine& eng) { eng.currentMode = Mode::DRAW_ELLIPSE; return std::make_unique<EllipseCommand>(); });
        registerCommand("ELIPSE", [](Engine& eng) { eng.currentMode = Mode::DRAW_ELLIPSE; return std::make_unique<EllipseCommand>(); });

        registerCommand("DIM", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::ALIGNED); });
        registerCommand("COTA", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::ALIGNED); });
        registerCommand("ACOTAR", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::ALIGNED); });
        registerCommand("DIMH", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::HORIZONTAL); });
        registerCommand("DIMV", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::VERTICAL); });
        registerCommand("DIMR", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::RADIUS); });
        registerCommand("DIMDIA", [](Engine& eng) { eng.currentMode = Mode::DRAW_DIMENSION; return std::make_unique<DimensionCommand>(DimType::DIAMETER); });

		registerCommand("T", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::DRAW_TEXT; return std::make_unique<TextCommand>(); });
		registerCommand("TEXT", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::DRAW_TEXT; return std::make_unique<TextCommand>(); });
		registerCommand("TEXTO", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::DRAW_TEXT; return std::make_unique<TextCommand>(); });
        // --- COMANDOS DE MODIFICACION ---

        registerCommand("M", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::MOVE; return std::make_unique<MoveCommand>(); });
        registerCommand("MOVE", [](Engine& eng)-> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::MOVE; return std::make_unique<MoveCommand>(); });
        registerCommand("MOVER", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::MOVE; return std::make_unique<MoveCommand>(); });

        registerCommand("CO", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::COPY; return std::make_unique<CopyCommand>(); });
        registerCommand("COPY", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::COPY; return std::make_unique<CopyCommand>(); });
        registerCommand("COPIAR", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::COPY; return std::make_unique<CopyCommand>(); });

        registerCommand("RO", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::ROTATE; return std::make_unique<RotateCommand>(); });
        registerCommand("ROTATE", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::ROTATE; return std::make_unique<RotateCommand>(); });
        registerCommand("ROTAR", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::ROTATE; return std::make_unique<RotateCommand>(); });

        registerCommand("SC", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::SCALE; return std::make_unique<ScaleCommand>(); });
        registerCommand("SCALE", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::SCALE; return std::make_unique<ScaleCommand>(); });
        registerCommand("ESCALAR", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::SCALE; return std::make_unique<ScaleCommand>(); });

        registerCommand("SI", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::MIRROR; return std::make_unique<MirrorCommand>(); });
        registerCommand("MIRROR", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::MIRROR; return std::make_unique<MirrorCommand>(); });
        registerCommand("SIMETRIA", [](Engine& eng) -> std::unique_ptr<ICommand> { if (!Engine::checkSelection(eng)) return nullptr; eng.currentMode = Mode::MIRROR; return std::make_unique<MirrorCommand>(); });

        registerCommand("OF", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::OFFSET; return std::make_unique<OffsetCommand>(); });
        registerCommand("OFFSET", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::OFFSET; return std::make_unique<OffsetCommand>(); });
        registerCommand("DESPLAZAR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::OFFSET; return std::make_unique<OffsetCommand>(); });

        registerCommand("F", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::FILLET; return std::make_unique<FilletCommand>(); });
        registerCommand("FILLET", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::FILLET; return std::make_unique<FilletCommand>(); });
        registerCommand("EMPALME", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::FILLET; return std::make_unique<FilletCommand>(); });

        registerCommand("CHA", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::CHAMFER; return std::make_unique<ChamferCommand>(); });
        registerCommand("CHAMFER", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::CHAMFER; return std::make_unique<ChamferCommand>(); });
        registerCommand("CHAFLAN", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::CHAMFER; return std::make_unique<ChamferCommand>(); });

        registerCommand("TR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::TRIM; return std::make_unique<TrimCommand>(); });
        registerCommand("TRIM", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::TRIM; return std::make_unique<TrimCommand>(); });
        registerCommand("RECORTAR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::TRIM; return std::make_unique<TrimCommand>(); });

        registerCommand("EX", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::EXTEND; return std::make_unique<ExtendCommand>(); });
        registerCommand("EXTEND", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::EXTEND; return std::make_unique<ExtendCommand>(); });
        registerCommand("ALARGAR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::EXTEND; return std::make_unique<ExtendCommand>(); });

        registerCommand("DIST", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::MEASURE_DIST; return std::make_unique<MeasureCommand>(); });
        registerCommand("MEDIR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::MEASURE_DIST; return std::make_unique<MeasureCommand>(); });

        registerCommand("ARR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::ARRAY; return std::make_unique<ArrayCommand>(); });
        registerCommand("ARRAY", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::ARRAY; return std::make_unique<ArrayCommand>(); });

        registerCommand("S", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::STRETCH; return std::make_unique<StretchCommand>(); });
        registerCommand("STRETCH", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::STRETCH; return std::make_unique<StretchCommand>(); });
        registerCommand("ESTIRAR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::STRETCH; return std::make_unique<StretchCommand>(); });

        registerCommand("BLOCK", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::BLOCK_CREATE; return std::make_unique<BlockCreateCommand>(); });
        registerCommand("BLOQUE", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::BLOCK_CREATE; return std::make_unique<BlockCreateCommand>(); });

        registerCommand("INSERT", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::BLOCK_INSERT; return std::make_unique<BlockInsertCommand>(); });
        registerCommand("INSERTAR", [](Engine& eng) -> std::unique_ptr<ICommand> { eng.currentMode = Mode::BLOCK_INSERT; return std::make_unique<BlockInsertCommand>(); });

        registerCommand("GRIP", [](Engine& eng) -> std::unique_ptr<ICommand> {
            if (eng.selectedEntities.empty()) { eng.statusMessage = "Primero selecciona entidades."; return nullptr; }
            eng.currentMode = Mode::GRIP_EDIT; eng.activeGripEntity = nullptr; eng.activeGripIndex = -1; eng.gripBackup.reset();
            eng.statusMessage = "GRIP EDIT | Selecciona un grip para mover:"; return nullptr;
        });
        registerCommand("GRIPS", [](Engine& eng) -> std::unique_ptr<ICommand> {
            if (eng.selectedEntities.empty()) { eng.statusMessage = "Primero selecciona entidades."; return nullptr; }
            eng.currentMode = Mode::GRIP_EDIT; eng.activeGripEntity = nullptr; eng.activeGripIndex = -1; eng.gripBackup.reset();
            eng.statusMessage = "GRIP EDIT | Selecciona un grip para mover:"; return nullptr;
        });
        registerCommand("EDIT", [](Engine& eng) -> std::unique_ptr<ICommand> {
            if (eng.selectedEntities.empty()) { eng.statusMessage = "Primero selecciona entidades."; return nullptr; }
            eng.currentMode = Mode::GRIP_EDIT; eng.activeGripEntity = nullptr; eng.activeGripIndex = -1; eng.gripBackup.reset();
            eng.statusMessage = "GRIP EDIT | Selecciona un grip para mover:"; return nullptr;
        });
    }

} // namespace cad