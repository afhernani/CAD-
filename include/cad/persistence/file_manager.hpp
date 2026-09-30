#pragma once
#include <string>
#include "cad/core/document/document.hpp"

namespace cad {

class FileManager {
public:
    // Guardar documento en archivo
    static bool saveDocument(const Document& doc, const std::string& filePath);
    
    // Cargar documento desde archivo (modifica el doc existente)
    static bool loadDocument(Document& doc, const std::string& filePath);
    
    // Verificar si el archivo existe
    static bool fileExists(const std::string& filePath);
    
    // Obtener extensión del archivo
    static std::string getFileExtension(const std::string& filePath);
};

} // namespace cad