#include <SFML/Graphics.hpp>
#include <iostream>

int main() {
    std::cout << "=== TEST MINIMAL SFML ===" << std::endl;
    
    // Configuración mínima explícita
    sf::ContextSettings settings;
    settings.majorVersion = 1;
    settings.minorVersion = 1;
    settings.antialiasingLevel = 0;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    
    std::cout << "Creando ventana..." << std::endl;
    sf::RenderWindow window(sf::VideoMode(800, 600), "Test Minimal", 
                           sf::Style::Default, settings);
    
    if (!window.isOpen()) {
        std::cerr << "ERROR: No se pudo crear la ventana" << std::endl;
        return 1;
    }
    
    std::cout << "Ventana creada. Posición: " 
              << window.getPosition().x << "," << window.getPosition().y << std::endl;
    
    // Forzar posición DESPUÉS de crear
    window.setPosition(sf::Vector2i(100, 100));
    std::cout << "Posición forzada: " 
              << window.getPosition().x << "," << window.getPosition().y << std::endl;
    
    window.setFramerateLimit(30);
    
    std::cout << "Iniciando bucle..." << std::endl;
    
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                std::cout << "Cerrando ventana..." << std::endl;
                window.close();
            }
        }
        
        window.clear(sf::Color::Blue);
        window.display();
    }
    
    std::cout << "Test completado." << std::endl;
    return 0;
}