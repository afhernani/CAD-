#pragma once
#include "cad/core/document/document.hpp"
#include <string>

namespace cad {

    class Serializer {
    public:
        // >>> AHORA RECIBE EL DOCUMENTO COMPLETO <<<
        static std::string serialize(const Document& doc);
        
        // >>> AHORA MODIFICA EL DOCUMENTO COMPLETO <<<
        static void deserialize(const std::string& jsonStr, Document& doc);
    };

} // namespace cad