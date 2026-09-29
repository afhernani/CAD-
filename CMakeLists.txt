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