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

		if (upperCmd == "L" || upperCmd == "LINE" || upperCmd == "LINEA") {
			activeCommand_ = std::make_unique<LineCommand>(); currentMode = Mode::DRAW_LINE;
		} else if (upperCmd == "C" || upperCmd == "CIRCLE" || upperCmd == "CIRCULO") {
			activeCommand_ = std::make_unique<CircleCommand>(); currentMode = Mode::DRAW_CIRCLE;
		} else if (upperCmd == "A" || upperCmd == "ARC" || upperCmd == "ARCO") {
			activeCommand_ = std::make_unique<ArcCommand>(); currentMode = Mode::DRAW_ARC;
		} else if (upperCmd == "PL" || upperCmd == "POLILINEA") {
			activeCommand_ = std::make_unique<PolylineCommand>(); currentMode = Mode::DRAW_POLYLINE;
		} else if (upperCmd == "POL" || upperCmd == "POLIGONO") {
			activeCommand_ = std::make_unique<PolygonCommand>(); currentMode = Mode::DRAW_POLYGON;
		} else if (upperCmd == "EL" || upperCmd == "ELLIPSE" || upperCmd == "ELIPSE") {
			activeCommand_ = std::make_unique<EllipseCommand>(); currentMode = Mode::DRAW_ELLIPSE;
		} else if (upperCmd == "DIM" || upperCmd == "COTA" || upperCmd == "ACOTAR") {
			activeCommand_ = std::make_unique<DimensionCommand>(); currentMode = Mode::DRAW_DIMENSION;
		} else if (upperCmd == "Q" || upperCmd == "QUIT" || upperCmd == "SALIR") {
			statusMessage = "Usa el botón de cerrar ventana para salir."; return;
		} else if (upperCmd == "Z" || upperCmd == "BORRAR") {
			saveState(); doc.clear(); lastPoint = {0.0, 0.0}; statusMessage = "Dibujo borrado."; return;
		} else if (upperCmd == "UNDO" || upperCmd == "DESHACER") { undo(); return; }
		else if (upperCmd == "REDO" || upperCmd == "REHACER") { redo(); return; }
		else if (upperCmd == "AXIS" || upperCmd == "EJES") {
			statusMessage = "Usa el boton en la barra de herramientas para activar/desactivar ejes"; return;
		} else if (upperCmd == "LA" || upperCmd == "LAYER" || upperCmd == "CAPA") {
			currentMode = Mode::LAYER_COMMAND; statusMessage = "CAPA | ON <nombre> | OFF <nombre> | NEW <nombre> | SET <nombre> | LIST"; return;
		} else if (upperCmd == "M" || upperCmd == "MOVE" || upperCmd == "MOVER") {
			if (selectedEntities.empty()) { statusMessage = "MOVER | Primero selecciona entidades."; return; }
			activeCommand_ = std::make_unique<MoveCommand>(); currentMode = Mode::MOVE;
		} else if (upperCmd == "CO" || upperCmd == "COPY" || upperCmd == "COPIAR") {
			if (selectedEntities.empty()) { statusMessage = "COPIAR | Primero selecciona entidades."; return; }
			activeCommand_ = std::make_unique<CopyCommand>(); currentMode = Mode::COPY;
		} else if (upperCmd == "RO" || upperCmd == "ROTATE" || upperCmd == "ROTAR") {
			if (selectedEntities.empty()) { statusMessage = "ROTAR | Primero selecciona entidades."; return; }
			activeCommand_ = std::make_unique<RotateCommand>(); currentMode = Mode::ROTATE;
		} else if (upperCmd == "SC" || upperCmd == "SCALE" || upperCmd == "ESCALAR") {
			if (selectedEntities.empty()) { statusMessage = "ESCALAR | Primero selecciona entidades."; return; }
			activeCommand_ = std::make_unique<ScaleCommand>(); currentMode = Mode::SCALE;
		} else if (upperCmd == "SI" || upperCmd == "SYM" || upperCmd == "MIRROR" || upperCmd == "SIMETRIA") {
			if (selectedEntities.empty()) { statusMessage = "SIMETRIA | Primero selecciona entidades."; return; }
			activeCommand_ = std::make_unique<MirrorCommand>(); currentMode = Mode::MIRROR;
		} else if (upperCmd == "OF" || upperCmd == "OFFSET" || upperCmd == "DESPLAZAR") {
			activeCommand_ = std::make_unique<OffsetCommand>(); currentMode = Mode::OFFSET;
		} else if (upperCmd == "F" || upperCmd == "FILLET" || upperCmd == "EMPALME") {
			activeCommand_ = std::make_unique<FilletCommand>(); currentMode = Mode::FILLET;
		} else if (upperCmd == "CHA" || upperCmd == "CHAMFER" || upperCmd == "CHAFLAN") {
			activeCommand_ = std::make_unique<ChamferCommand>(); currentMode = Mode::CHAMFER;
		} else if (upperCmd == "TR" || upperCmd == "TRIM" || upperCmd == "RECORTAR") {
			activeCommand_ = std::make_unique<TrimCommand>(); currentMode = Mode::TRIM;
		} else if (upperCmd == "EX" || upperCmd == "EXTEND" || upperCmd == "ALARGAR") {
			activeCommand_ = std::make_unique<ExtendCommand>(); currentMode = Mode::EXTEND;
		} else if (upperCmd == "DIST" || upperCmd == "MEDIR") {
			activeCommand_ = std::make_unique<MeasureCommand>(); currentMode = Mode::MEASURE_DIST;
		} else if (upperCmd == "ARR" || upperCmd == "ARRAY") {
			activeCommand_ = std::make_unique<ArrayCommand>(); currentMode = Mode::ARRAY;
		} else if (upperCmd == "S" || upperCmd == "STRETCH" || upperCmd == "ESTIRAR") {
			activeCommand_ = std::make_unique<StretchCommand>(); currentMode = Mode::STRETCH;
		} else if (upperCmd == "GRID" || upperCmd == "REJILLA") {
			toggleGrid(); statusMessage = gridEnabled ? "Rejilla activada." : "Rejilla desactivada."; return;
		} else if (upperCmd == "BLOCK" || upperCmd == "BLOQUE") {
			activeCommand_ = std::make_unique<BlockCreateCommand>(); currentMode = Mode::BLOCK_CREATE;
		} else if (upperCmd == "INSERT" || upperCmd == "INSERTAR") {
			activeCommand_ = std::make_unique<BlockInsertCommand>(); currentMode = Mode::BLOCK_INSERT;
		} else if (upperCmd == "GRIP" || upperCmd == "GRIPS" || upperCmd == "EDIT") {
			if (selectedEntities.empty()) { statusMessage = "GRIP EDIT | Primero selecciona entidades."; return; }
			currentMode = Mode::GRIP_EDIT; activeGripEntity = nullptr; activeGripIndex = -1; gripBackup.reset();
			statusMessage = "GRIP EDIT | Selecciona un grip para mover:"; return;
		} else if (upperCmd == "LIST" || upperCmd == "LISTA") {
			statusMessage = "LISTA | Panel de propiedades activado/desactivado"; return;
		} else if (upperCmd == "HELP" || upperCmd == "AYUDA" || upperCmd == "?") {
			statusMessage = "Ayuda: escribe HELP <comando> para más detalles"; return;
		} else {
			statusMessage = "Comando desconocido: " + std::string(cmd); return;
		}
		
		statusMessage = activeCommand_->getStatusMessage();
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
		selectionManager.selectByWindow(p1, p2, doc.entities, selectedEntities, addToSelection);
		statusMessage = std::to_string(selectedEntities.size()) + " entidades seleccionadas.";
	}

	std::string Engine::getHelpText(std::string_view topic) {
		std::string upperTopic(topic);
		std::transform(upperTopic.begin(), upperTopic.end(), upperTopic.begin(), ::toupper);
		upperTopic.erase(0, upperTopic.find_first_not_of(' '));
		upperTopic.erase(upperTopic.find_last_not_of(' ') + 1);
		std::ostringstream oss;
		if (upperTopic.empty()) {
			oss << "========================================\n  CAD+ v1.0 - LISTA DE COMANDOS\n========================================\n";
			oss << "[ DIBUJO ]\n  L, LINEA\n  C, CIRCULO\n  A, ARCO\n  PL, POLILINEA\n  POL, POLIGONO\n  EL, ELIPSE\n";
			oss << "[ COTA ]\n  DIM, COTA\n";
			oss << "[ MODIFICACION ]\n  M, MOVER\n  CO, COPIAR\n  RO, ROTAR\n  SC, ESCALAR\n  SI, SIMETRIA\n  TR, RECORTAR\n  EX, ALARGAR\n  AR, ARRAY\n  S, STRETCH\n";
			oss << "[ EDICION Y SISTEMA ]\n  Z, BORRAR\n  LA, CAPA\n  DIST, MEDIR\n  GRID, REJILLA\n  AYUDA, ?\n  SAVE, GUARDAR\n  LOAD, CARGAR\n========================================\n";
			return oss.str();
		}
		oss << "Comando no reconocido: " << topic << "\nUsa HELP para ver comandos disponibles.\n";
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

} // namespace cad