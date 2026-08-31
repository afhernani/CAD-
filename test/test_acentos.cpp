#include <SFML/Graphics.hpp>
#include <iostream>
#include <cstring>

// Función auxiliar para crear sf::String desde UTF-8
sf::String fromUtf8(const char* str) {
    return sf::String::fromUtf8(str, str + std::strlen(str));
}

int main() {
    sf::RenderWindow window(sf::VideoMode(700, 400), "Test Acentos SFML");
    
    sf::Font font;
    if (!font.loadFromFile("assets/arial.ttf")) {
        std::cerr << "No se pudo cargar la fuente" << std::endl;
        return 1;
    }
    
    // SIN u8 - el archivo ya está en UTF-8
    sf::Text t1;
    t1.setFont(font);
    t1.setString(fromUtf8("Texto directo: á é í ó ú ñ Á É Í Ó Ú Ñ"));
    t1.setCharacterSize(20);
    t1.setFillColor(sf::Color::White);
    t1.setPosition(20, 30);
    
    sf::Text t2;
    t2.setFont(font);
    t2.setString(fromUtf8("Polígono, Círculo, Línea, Simetría, Recortar"));
    t2.setCharacterSize(20);
    t2.setFillColor(sf::Color::Yellow);
    t2.setPosition(20, 70);
    
    sf::Text t3;
    t3.setFont(font);
    t3.setString(fromUtf8("Símbolos: ¿ ¡ ° € · º ª"));
    t3.setCharacterSize(20);
    t3.setFillColor(sf::Color::White);
    t3.setPosition(20, 110);
    
    sf::Text t4;
    t4.setFont(font);
    t4.setString(fromUtf8("Entidades borradas. Capa activada. Comando cancelado."));
    t4.setCharacterSize(16);
    t4.setFillColor(sf::Color(200, 200, 200));
    t4.setPosition(20, 150);
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        
        window.clear(sf::Color(30, 30, 30));
        window.draw(t1);
        window.draw(t2);
        window.draw(t3);
        window.draw(t4);
        window.display();
    }
    
    return 0;
}