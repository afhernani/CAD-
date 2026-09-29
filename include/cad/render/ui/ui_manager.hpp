#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include "cad/commands/engine.hpp"

namespace cad {

class UIManager {
public:
    UIManager(sf::Font& font);

    // Método principal de renderizado de la UI
    void draw(sf::RenderWindow& window, const Engine& engine, 
              const std::string& inputBuffer, const std::vector<std::string>& commandHistory,
              int commandScrollOffset, bool isTyping, bool showEntityListPanel,
              const std::string& entityListText, int entityListScrollOffset,
              bool showAxes, float viewScale, const Point2D& mouseWorldPos) const;

private:
    // Constantes de layout
    static constexpr unsigned int WINDOW_WIDTH = 1280;
    static constexpr unsigned int WINDOW_HEIGHT = 720;
    static constexpr unsigned int MENU_HEIGHT = 30;
    static constexpr unsigned int TOOLBAR_HEIGHT = 60;
    static constexpr unsigned int COMMAND_HEIGHT = 90;
    static constexpr unsigned int STATUS_HEIGHT = 25;

    void drawMenuBar(sf::RenderWindow& window) const;
    void drawToolbar(sf::RenderWindow& window, bool showAxes) const;
    void drawCommandWindow(sf::RenderWindow& window, const std::string& inputBuffer, 
                           const std::vector<std::string>& commandHistory, int scrollOffset,
                           bool isTyping, bool showEntityListPanel) const;
    void drawStatusBar(sf::RenderWindow& window, const Engine& engine, float viewScale, 
                       const Point2D& mouseWorldPos, bool showEntityListPanel) const;
    void drawEntityListPanel(sf::RenderWindow& window, const std::string& text, int scrollOffset) const;

    // Helper para convertir UTF-8 a sf::String
    sf::String toSfString(const std::string& utf8Str) const {
        return sf::String::fromUtf8(utf8Str.begin(), utf8Str.end());
    }

    sf::Font& font_;
};

} // namespace cad