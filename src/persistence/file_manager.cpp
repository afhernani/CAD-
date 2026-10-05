#include "cad/persistence/file_manager.hpp"
#include "cad/persistence/serializer.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <sstream>

namespace cad {

    bool FileManager::saveDocument(Document& doc, const std::string& filePath) {
        try {
            // >>> PASAR EL DOCUMENTO COMPLETO AL SERIALIZER <<<
            std::string json = Serializer::serialize(doc);
            
            std::ofstream file(filePath);
            if (!file.is_open()) {
                std::cerr << "[FileManager] No se pudo abrir: " << filePath << std::endl;
                return false;
            }
            
            file << json;
            file.close();
            
            std::cout << "[FileManager] Documento guardado: " << filePath << std::endl;
            doc.isModified = false; // Marcar como no modificado después de guardar
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
            
            // >>> DESERIALIZAR DIRECTAMENTE EN EL DOCUMENTO <<<
            Serializer::deserialize(json, doc);
            
            std::cout << "[FileManager] Documento cargado: " << filePath 
                    << " (" << doc.entities.size() << " entidades, " 
                    << doc.layers.size() << " capas)" << std::endl;
            doc.isModified = false; // Marcar como no modificado después de cargar
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