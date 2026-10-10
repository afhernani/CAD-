#pragma once

// 1. INCLUDES MÍNIMOS (Solo lo que Engine necesita conocer directamente)
#include "cad/core/document/document.hpp"
#include "command.hpp" // ¡Solo la interfaz base!
#include "cad/core/selection/selection_manager.hpp"

#include <unordered_map>
#include <functional>
#include <string>
#include <optional>
#include <string_view>
#include <vector>
#include <memory>


namespace cad {
	enum class Mode {
		 IDLE, DRAW_LINE, DRAW_CIRCLE, DRAW_ARC, DRAW_POLYLINE, DRAW_POLYGON,
		 DRAW_ELLIPSE, DRAW_DIMENSION, DIM_OPTIONS, DRAW_DIM_ALIGNED,
		 DRAW_DIM_RADIUS, DRAW_DIM_DIAMETER, DRAW_DIM_ANGULAR, LAYER_COMMAND,
		 COPY, ROTATE, SCALE, MOVE, MIRROR, MEASURE_DIST, TRIM, EXTEND,
		 UNDO, ARRAY, STRETCH, REDO, OFFSET, FILLET, CHAMFER,
		 BLOCK_CREATE, BLOCK_INSERT, GRIP_EDIT, DRAW_TEXT, CONFIRM_NEW, 
		 DRAW_HATCH, DRAW_TRIANGLE, WAITING_SAVE_AS_NAME, WAITING_OPEN_FILE
	};

	enum class ArrayType { RECTANGULAR, POLAR };

	class Engine {
	public:
		Engine();
		Document doc;
		Mode currentMode = Mode::IDLE;
		std::string statusMessage = "Listo";
		
		std::unique_ptr<ICommand> activeCommand_;
		Point2D lastPoint;
		double viewScale = 1.0;  

		void processInput(std::string_view input);
		void cancelCommand();
		
		std::string getHelpForTopic(std::string_view topic, int maxCharsPerLine = 80) { return getHelpText(topic, maxCharsPerLine); }
		std::string getEntityList() const;
		
		std::vector<Entity*> selectedEntities;
		void clearSelection();
		void selectEntity(const Point2D& clickPoint, double tolerance);
		void deleteSelected();
		
		Entity* activeGripEntity = nullptr;
		int activeGripIndex = -1;
		std::unique_ptr<Entity> gripBackup;
		
		std::vector<std::string> getAllCommands() const;
		
		std::vector<std::vector<std::unique_ptr<Entity>>> undoStack;
		std::vector<std::vector<std::unique_ptr<Entity>>> redoStack;
		void saveState();
		void undo();
		void redo();
		void clearDocument();
		
		bool gridEnabled = false;
		void toggleGrid() { gridEnabled = !gridEnabled; }
		
		Point2D tempDimP1, tempDimP2, tempDimP3, tempDimP2_line2;
		double tempDimAngle = 0.0;
		DimType currentDimType = DimType::HORIZONTAL;

		SelectionManager selectionManager;  // Miembro de seleccion
		// Método público que App llamará
		void performWindowSelection(const Point2D& p1, const Point2D& p2, bool addToSelection = false);


	private:
		static bool checkSelection(Engine& eng);
		// >>> NUEVO: Sistema de Fábrica / Registro
		using CommandFactory = std::function<std::unique_ptr<ICommand>(Engine&)>;
		std::unordered_map<std::string, CommandFactory> commandRegistry_;

		void initializeCommands();
		void registerCommand(const std::string& name, CommandFactory factory);
		
		void executeCommand(std::string_view cmd);
		void processCoordinate(std::string_view coordStr);
		void processLayerCommand(std::string_view input);
		bool isNumericValue(std::string_view str) const;
		std::string getHelpText(std::string_view topic, int maxCharsPerLine = 80);
		[[nodiscard]] std::optional<Point2D> parseCoordinate(std::string_view str);
	};
} // namespace cad