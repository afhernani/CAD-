cmake_minimum_required(VERSION 3.20)
project(CAD+ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Recopilar todos los .cpp de src/
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(CAD+ 
    ${SOURCES} 
    ${CMAKE_SOURCE_DIR}/resources.rc
)

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
# ==============================================================================
# INTEGRACIÓN DE DEAR IMGUI (Solución definitiva a prueba de fallos)
# ==============================================================================
include(FetchContent)

# 1. Descargar ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.1
)
FetchContent_MakeAvailable(imgui)

# 2. Descargar imgui-sfml
FetchContent_Declare(
    imgui_sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG        v2.6
)

# Mantenemos ON para que imgui-sfml encuentre el SFML compartido de Pixi sin errores
set(BUILD_SHARED_LIBS ON)
set(IMGUI_SFML_FIND_IMGUI OFF)
set(IMGUI_SFML_IMGUI_DEMO OFF)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

FetchContent_MakeAvailable(imgui_sfml)

# 3. Compilar el núcleo de ImGui como estático (nuestros propios archivos)
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})

# 4. Enlazar todo a tu ejecutable
target_link_libraries(CAD+ PRIVATE 
    sfml-graphics 
    sfml-window 
    sfml-system
    ImGui-SFML::ImGui-SFML
    imgui_lib
)

# 5. ¡LA CLAVE! Copiar automáticamente la DLL de ImGui-SFML a la carpeta del .exe
# Esto soluciona el error de "no se encontró la DLL" en tiempo de ejecución.
add_custom_command(TARGET CAD+ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:ImGui-SFML::ImGui-SFML>
    $<TARGET_FILE_DIR:CAD+>
)