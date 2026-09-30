cmake_minimum_required(VERSION 3.20)
project(CAD+ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recopilar todos los .cpp de src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(CAD+ ${SOURCES})

# Cabeceras
target_include_directories(CAD+ PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# SFML
find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)
target_link_libraries(CAD+ PRIVATE sfml-graphics sfml-window sfml-system)

# Windows: subsistema
if(WIN32)
    set_target_properties(CAD+  PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

# ==============================================================================
# MEJORA 2: Copiar recursos automáticamente al compilar
# Esto soluciona el error de "No se pudo cargar la fuente" o "config.json no encontrado"
# cuando ejecutas el .exe desde la carpeta build/.
# ==============================================================================
add_custom_command(TARGET CAD+ POST_BUILD
    # 1. Copiar la carpeta de assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CAD+>/assets
    
    # 2. Copiar el archivo de configuración (si existe)
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/config.json $<TARGET_FILE_DIR:CAD+>/config.json
)