cmake_minimum_required(VERSION 3.28)
project(CAD+ LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 2.6 COMPONENTS graphics window system REQUIRED)

# Recopilar todos los .cpp de la carpeta src/
file(GLOB SOURCES "src/*.cpp")

add_executable(CAD+ ${SOURCES})

# Añadir la carpeta include para que encuentre los .hpp
target_include_directories(CAD+ PRIVATE include)

target_link_libraries(CAD+ PRIVATE sfml-graphics sfml-window sfml-system)

add_custom_command(TARGET CAD+ POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:CAD+>/assets
)

# Test de acentos
add_executable(test_acentos test/test_acentos.cpp)
target_link_libraries(test_acentos PRIVATE sfml-graphics sfml-window sfml-system)