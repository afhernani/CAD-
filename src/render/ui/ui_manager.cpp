#include "cad/render/ui/ui_manager.hpp"
#include <chrono>
#include <sstream>
#include <algorithm>

namespace cad {

UIManager::UIManager(sf::Font& font) : font_(font) {}

void UIManager::draw(sf::RenderWindow& window, const Engine& engine, 
                     const std::string& inputBuffer, const std::vector<std::string>& commandHistory,
                     int commandScrollOffset, bool isTyping, bool showEntityListPanel,
                     const std::string& entityListText, int entityListScrollOffset,
                     bool showAxes, float viewScale, const Point2D& mouseWorldPos) const {
    drawMenuBar(window);
    drawToolbar(window, showAxes);
    drawCommandWindow(window, inputBuffer, commandHistory, commandScrollOffset, isTyping, showEntityListPanel);
    drawStatusBar(window, engine, viewScale, mouseWorldPos, showEntityListPanel);
    
    if (showEntityListPanel) {
        drawEntityListPanel(window, entityListText, entityListScrollOffset);
    }
}

void UIManager::drawMenuBar(sf::RenderWindow& window) const {
    // Barra de Menú (Arriba)
    sf::RectangleShape menu(sf::Vector2f(WINDOW_WIDTH, MENU_HEIGHT));
    menu.setFillColor(sf::Color(45, 45, 48));
    menu.setPosition(0, 0);
    window.draw(menu);

    sf::Text menuTxt;
    menuTxt.setFont(font_);
    menuTxt.setString(toSfString("Archivo  Editar  Ver  Dibujar  Modificar  Ayuda"));
    menuTxt.setCharacterSize(14);
    menuTxt.setFillColor(sf::Color(220, 220, 220));
    menuTxt.setPosition(10, 8);
    window.draw(menuTxt);
}

void UIManager::drawToolbar(sf::RenderWindow& window, bool showAxes) const {
    // Barra de Herramientas (Debajo del menú)
    sf::RectangleShape toolbar(sf::Vector2f(WINDOW_WIDTH, TOOLBAR_HEIGHT));
    toolbar.setFillColor(sf::Color(45, 45, 48));
    toolbar.setPosition(0, MENU_HEIGHT);
    window.draw(toolbar);

    // Línea separadora inferior
    sf::RectangleShape separator(sf::Vector2f(WINDOW_WIDTH, 1));
    separator.setFillColor(sf::Color(80, 80, 80));
    separator.setPosition(0, MENU_HEIGHT + TOOLBAR_HEIGHT - 1);
    window.draw(separator);

    // --- Botón Línea (L) ---
    sf::RectangleShape btnLine(sf::Vector2f(40, 40));
    btnLine.setFillColor(sf::Color(70, 70, 75));
    btnLine.setOutlineColor(sf::Color(100, 100, 100));
    btnLine.setOutlineThickness(1.f);
    btnLine.setPosition(10, MENU_HEIGHT + 10);
    window.draw(btnLine);

    // Icono de línea (diagonal blanca)
    sf::Vertex lineIcon[] = {
        sf::Vertex(sf::Vector2f(18, MENU_HEIGHT + 38), sf::Color::White),
        sf::Vertex(sf::Vector2f(42, MENU_HEIGHT + 14), sf::Color::White)
    };
    window.draw(lineIcon, 2, sf::Lines);

    // --- Botón Círculo (C) ---
    sf::RectangleShape btnCircle(sf::Vector2f(40, 40));
    btnCircle.setFillColor(sf::Color(70, 70, 75));
    btnCircle.setOutlineColor(sf::Color(100, 100, 100));
    btnCircle.setOutlineThickness(1.f);
    btnCircle.setPosition(60, MENU_HEIGHT + 10);
    window.draw(btnCircle);

    // Icono de círculo
    sf::CircleShape circleIcon(12.f);
    circleIcon.setFillColor(sf::Color::Transparent);
    circleIcon.setOutlineColor(sf::Color::White);
    circleIcon.setOutlineThickness(2.f);
    circleIcon.setPosition(72, MENU_HEIGHT + 22);
    window.draw(circleIcon);

    // --- Botón Arco (A) ---
    sf::RectangleShape btnArc(sf::Vector2f(40, 40));
    btnArc.setFillColor(sf::Color(70, 70, 75));
    btnArc.setOutlineColor(sf::Color(100, 100, 100));
    btnArc.setOutlineThickness(1.f);
    btnArc.setPosition(110, MENU_HEIGHT + 10);
    window.draw(btnArc);

    // Icono de arco (semicírculo)
    sf::CircleShape arcIcon(12.f);
    arcIcon.setFillColor(sf::Color::Transparent);
    arcIcon.setOutlineColor(sf::Color::White);
    arcIcon.setOutlineThickness(2.f);
    arcIcon.setRadius(10.f);
    arcIcon.setPosition(122, MENU_HEIGHT + 24);
    window.draw(arcIcon);

    // Línea base del arco
    sf::Vertex arcBase[] = {
        sf::Vertex(sf::Vector2f(122, MENU_HEIGHT + 34), sf::Color::White),
        sf::Vertex(sf::Vector2f(142, MENU_HEIGHT + 34), sf::Color::White)
    };
    window.draw(arcBase, 2, sf::Lines);

    // --- Separador vertical ---
    sf::RectangleShape sep1(sf::Vector2f(1, 40));
    sep1.setFillColor(sf::Color(80, 80, 80));
    sep1.setPosition(160, MENU_HEIGHT + 10);
    window.draw(sep1);

    // --- Botón Ejes ON/OFF ---
    sf::RectangleShape btnAxes(sf::Vector2f(40, 40));
    btnAxes.setFillColor(showAxes ? sf::Color(0, 100, 0) : sf::Color(70, 70, 75));
    btnAxes.setOutlineColor(sf::Color(100, 100, 100));
    btnAxes.setOutlineThickness(1.f);
    btnAxes.setPosition(170, MENU_HEIGHT + 10);
    window.draw(btnAxes);

    // Icono de ejes (X rojo, Y verde)
    sf::Vertex axisX[] = {
        sf::Vertex(sf::Vector2f(178, MENU_HEIGHT + 30), sf::Color::Red),
        sf::Vertex(sf::Vector2f(202, MENU_HEIGHT + 30), sf::Color::Red)
    };
    window.draw(axisX, 2, sf::Lines);

    sf::Vertex axisY[] = {
        sf::Vertex(sf::Vector2f(190, MENU_HEIGHT + 38), sf::Color::Green),
        sf::Vertex(sf::Vector2f(190, MENU_HEIGHT + 14), sf::Color::Green)
    };
    window.draw(axisY, 2, sf::Lines);

    // --- Botón Borrar (papelera) ---
    sf::RectangleShape btnClear(sf::Vector2f(40, 40));
    btnClear.setFillColor(sf::Color(70, 70, 75));
    btnClear.setOutlineColor(sf::Color(100, 100, 100));
    btnClear.setOutlineThickness(1.f);
    btnClear.setPosition(220, MENU_HEIGHT + 10);
    window.draw(btnClear);

    // Icono de papelera (rectángulo con tapa)
    sf::RectangleShape trashBody(sf::Vector2f(16, 14));
    trashBody.setFillColor(sf::Color::Transparent);
    trashBody.setOutlineColor(sf::Color::White);
    trashBody.setOutlineThickness(1.5f);
    trashBody.setPosition(228, MENU_HEIGHT + 22);
    window.draw(trashBody);

    sf::RectangleShape trashLid(sf::Vector2f(20, 3));
    trashLid.setFillColor(sf::Color::White);
    trashLid.setPosition(226, MENU_HEIGHT + 19);
    window.draw(trashLid);

    // Líneas verticales de la papelera
    sf::Vertex trashLine1[] = {
        sf::Vertex(sf::Vector2f(233, MENU_HEIGHT + 22), sf::Color::White),
        sf::Vertex(sf::Vector2f(233, MENU_HEIGHT + 36), sf::Color::White)
    };
    window.draw(trashLine1, 2, sf::Lines);

    sf::Vertex trashLine2[] = {
        sf::Vertex(sf::Vector2f(238, MENU_HEIGHT + 22), sf::Color::White),
        sf::Vertex(sf::Vector2f(238, MENU_HEIGHT + 36), sf::Color::White)
    };
    window.draw(trashLine2, 2, sf::Lines);

    // --- Separador vertical ---
    sf::RectangleShape sep2(sf::Vector2f(1, 40));
    sep2.setFillColor(sf::Color(80, 80, 80));
    sep2.setPosition(270, MENU_HEIGHT + 10);
    window.draw(sep2);

    // --- Botón Ayuda (?) ---
    sf::RectangleShape btnHelp(sf::Vector2f(40, 40));
    btnHelp.setFillColor(sf::Color(70, 70, 75));
    btnHelp.setOutlineColor(sf::Color(100, 100, 100));
    btnHelp.setOutlineThickness(1.f);
    btnHelp.setPosition(280, MENU_HEIGHT + 10);
    window.draw(btnHelp);

    // Icono de interrogación
    sf::Text helpIcon("?", font_, 24);
    helpIcon.setFillColor(sf::Color::White);
    helpIcon.setPosition(288, MENU_HEIGHT + 14);
    window.draw(helpIcon);
}

void UIManager::drawCommandWindow(sf::RenderWindow& window, const std::string& inputBuffer, 
                                   const std::vector<std::string>& commandHistory, int scrollOffset,
                                   bool isTyping, bool showEntityListPanel) const {
    int cmdWidth = showEntityListPanel ? (WINDOW_WIDTH - 350) : WINDOW_WIDTH;

    // Fondo de la ventana de comandos
    sf::RectangleShape cmdBg(sf::Vector2f(cmdWidth, COMMAND_HEIGHT));
    cmdBg.setFillColor(sf::Color(60, 60, 60));
    cmdBg.setPosition(0, WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT);
    window.draw(cmdBg);

    int lineHeight = 20;
    int maxLines = (COMMAND_HEIGHT - 10) / lineHeight;
    int startY = WINDOW_HEIGHT - STATUS_HEIGHT - COMMAND_HEIGHT + 5;
    int totalLines = commandHistory.size() + 1;
    int maxOffset = std::max(0, totalLines - maxLines);

    // Asegurar que el offset no exceda el máximo
    scrollOffset = std::min(scrollOffset, maxOffset);
    int startIdx = std::max(0, totalLines - maxLines - scrollOffset);
    int lineCount = 0;

    // Dibujar líneas del historial
    for (int i = startIdx; i < commandHistory.size() && lineCount < maxLines - 1; ++i) {
        sf::Text histText;
        histText.setFont(font_);
        histText.setString(toSfString("> " + commandHistory[i]));
        histText.setCharacterSize(12);
        histText.setFillColor(sf::Color(180, 180, 180));
        histText.setPosition(10, startY + lineCount * lineHeight);
        window.draw(histText);
        lineCount++;
    }

    // Dibujar línea de comando actual (siempre en la parte inferior)
    std::string prompt = "Comando: " + inputBuffer;
    if (isTyping && std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() % 1000 < 500) {
        prompt += "_";  // Cursor parpadeante
    }

    sf::Text cmdText;
    cmdText.setFont(font_);
    cmdText.setString(toSfString(prompt));
    cmdText.setCharacterSize(12);
    cmdText.setFillColor(sf::Color::White);
    cmdText.setPosition(10, startY + (maxLines - 1) * lineHeight);
    window.draw(cmdText);

    // Indicador de scroll DINÁMICO
    if (totalLines > maxLines) {
        float scrollRatio = (maxOffset > 0) ? (float)scrollOffset / maxOffset : 0.0f;
        float availableHeight = (maxLines - 1) * lineHeight;
        float indicatorHeight = 30.f;
        float maxIndicatorY = startY + availableHeight - indicatorHeight;
        float indicatorY = startY + scrollRatio * (maxIndicatorY - startY);

        sf::RectangleShape scrollIndicator(sf::Vector2f(5, indicatorHeight));
        scrollIndicator.setFillColor(sf::Color(100, 100, 100));
        scrollIndicator.setPosition(cmdWidth - 10, indicatorY);
        window.draw(scrollIndicator);
    }
}

void UIManager::drawStatusBar(sf::RenderWindow& window, const Engine& engine, float viewScale, 
                               const Point2D& mouseWorldPos, bool showEntityListPanel) const {
    int statusWidth = showEntityListPanel ? WINDOW_WIDTH : WINDOW_WIDTH;

    // Barra de Estado (Muy abajo)
    sf::RectangleShape statusBg(sf::Vector2f(statusWidth, STATUS_HEIGHT));
    statusBg.setFillColor(sf::Color(0, 122, 204));
    statusBg.setPosition(0, WINDOW_HEIGHT - STATUS_HEIGHT);
    window.draw(statusBg);

    // Texto de estado con coordenadas en tiempo real
    std::ostringstream oss;
    oss << engine.statusMessage
        << " | X: " << mouseWorldPos.x
        << ", Y: " << mouseWorldPos.y
        << " | Zoom: " << viewScale << "x";

    if (showEntityListPanel) {
        oss << " | Panel: ON";
    }

    sf::Text statusTxt;
    statusTxt.setFont(font_);
    statusTxt.setString(toSfString(oss.str()));
    statusTxt.setCharacterSize(12);
    statusTxt.setFillColor(sf::Color::White);
    statusTxt.setPosition(10, WINDOW_HEIGHT - STATUS_HEIGHT + 6);
    window.draw(statusTxt);
}

void UIManager::drawEntityListPanel(sf::RenderWindow& window, const std::string& text, int scrollOffset) const {
    const int panelWidth = 350;
    const int panelHeight = WINDOW_HEIGHT - MENU_HEIGHT - TOOLBAR_HEIGHT - STATUS_HEIGHT;
    const int panelX = WINDOW_WIDTH - panelWidth;
    const int panelY = MENU_HEIGHT + TOOLBAR_HEIGHT;

    // Fondo del panel
    sf::RectangleShape panelBg(sf::Vector2f(panelWidth, panelHeight));
    panelBg.setFillColor(sf::Color(40, 40, 45));
    panelBg.setPosition(panelX, panelY);
    window.draw(panelBg);

    // Borde superior
    sf::RectangleShape panelBorder(sf::Vector2f(panelWidth, 2));
    panelBorder.setFillColor(sf::Color(80, 80, 80));
    panelBorder.setPosition(panelX, panelY);
    window.draw(panelBorder);

    // Título
    sf::Text title;
    title.setFont(font_);
    title.setString("PROPIEDADES DE ENTIDADES");
    title.setCharacterSize(14);
    title.setFillColor(sf::Color(220, 220, 220));
    title.setPosition(panelX + 10, panelY + 10);
    window.draw(title);

    // Separador
    sf::RectangleShape separator(sf::Vector2f(panelWidth - 20, 1));
    separator.setFillColor(sf::Color(100, 100, 100));
    separator.setPosition(panelX + 10, panelY + 35);
    window.draw(separator);

    // Contenido scrolleable
    const int lineHeight = 16;
    const int startY = panelY + 45;
    const int maxHeight = panelHeight - 55;
    const int maxLines = maxHeight / lineHeight;

    // Dividir el texto en líneas
    std::vector<std::string> lines;
    std::string line;
    for (char c : text) {
        if (c == '\n') {
            lines.push_back(line);
            line.clear();
        } else {
            line += c;
        }
    }
    if (!line.empty()) {
        lines.push_back(line);
    }

    // Limitar scroll
    int totalLines = lines.size();
    int maxOffset = std::max(0, totalLines - maxLines);
    scrollOffset = std::min(scrollOffset, maxOffset);

    // Dibujar líneas visibles
    int startIdx = scrollOffset;
    int lineCount = 0;
    for (int i = startIdx; i < totalLines && lineCount < maxLines; ++i) {
        sf::Text lineText;
        lineText.setFont(font_);
        lineText.setString(lines[i]);
        lineText.setCharacterSize(11);

        // Color diferente para títulos y contenido
        if (lines[i].find("---") != std::string::npos ||
            lines[i].find("===") != std::string::npos ||
            lines[i].find("LISTA") != std::string::npos) {
            lineText.setFillColor(sf::Color(100, 200, 255));
        } else if (lines[i].find("Tipo:") != std::string::npos) {
            lineText.setFillColor(sf::Color(255, 220, 100));
        } else {
            lineText.setFillColor(sf::Color(200, 200, 200));
        }

        lineText.setPosition(panelX + 10, startY + lineCount * lineHeight);
        window.draw(lineText);
        lineCount++;
    }

    // Indicador de scroll si hay más contenido
    if (totalLines > maxLines) {
        float scrollRatio = (maxOffset > 0) ? (float)scrollOffset / maxOffset : 0.0f;
        float indicatorHeight = 30.f;
        float maxIndicatorY = startY + maxHeight - indicatorHeight;
        float indicatorY = startY + scrollRatio * (maxIndicatorY - startY);

        sf::RectangleShape scrollIndicator(sf::Vector2f(5, indicatorHeight));
        scrollIndicator.setFillColor(sf::Color(100, 100, 100));
        scrollIndicator.setPosition(panelX + panelWidth - 10, indicatorY);
        window.draw(scrollIndicator);
    }
}

} // namespace cad