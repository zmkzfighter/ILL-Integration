cmake_minimum_required(VERSION 3.21)
project(ImpossibleLevelsGeode VERSION 1.0.0)

# Ajoute tous les .cpp du dossier src
file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Impossible de trouver le SDK Geode. Installe le CLI Geode et fais `geode sdk install`.")
else()
    message(STATUS "Utilisation du SDK Geode : $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
