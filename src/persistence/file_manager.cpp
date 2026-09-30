#include "cad/persistence/file_manager.hpp"
#include "cad/persistence/serializer.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace cad {

bool FileManager::saveDocument(const Document& doc, const std::string& filePath) {
    try {
        // 1. Usamos serialize que ya definimos para las entidades
        std::string json = Serializer::serialize(doc.entities);
        
        // 2. Escribir a archivo
        std::ofstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[FileManager] No se pudo abrir: " << filePath << std::endl;
            return false;
        }
        
        file << json;
        file.close();
        
        std::cout << "[FileManager] Documento guardado: " << filePath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FileManager] Error al guardar: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::loadDocument(Document& doc, const std::string& filePath) {
    try {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::cerr << "[FileManager] No se pudo abrir: " << filePath << std::endl;
            return false;
        }
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string json = buffer.str();
        file.close();
        
        // Limpiar entidades actuales y cargar las nuevas
        doc.entities.clear();
        doc.entities = Serializer::deserialize(json);
        
        std::cout << "[FileManager] Documento cargado: " << filePath 
                  << " (" << doc.entities.size() << " entidades)" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[FileManager] Error al cargar: " << e.what() << std::endl;
        return false;
    }
}

bool FileManager::fileExists(const std::string& filePath) {
    return std::filesystem::exists(filePath);
}

std::string FileManager::getFileExtension(const std::string& filePath) {
    return std::filesystem::path(filePath).extension().string();
}

} // namespace cad