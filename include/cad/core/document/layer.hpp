#pragma once
#include <string>
#include <SFML/Graphics/Color.hpp>

namespace cad {

    struct Layer {
        std::string name;
        bool visible = true;        // Se muestra en pantalla
        bool frozen = false;        // Congelada (no se regenera, más rápido)
        bool locked = false;        // Bloqueada (no se puede modificar)
        sf::Color color{255, 255, 255}; // Color por defecto (blanco)
        bool isCurrent = false;     // Capa activa para nuevo dibujo
        
        Layer() = default;
        explicit Layer(const std::string& n ) : name(n) {}
    };

}