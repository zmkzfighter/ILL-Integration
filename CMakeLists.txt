cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Doit etre defini AVANT project() sinon macOS build en mono-arch et casse
if (NOT DEFINED CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()

project(ImpossibleLevelsGeode VERSION 1.0.0)

# Tous les .cpp du dossier src, sous-dossiers inclus
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

# Sur Windows, Geode force-inclut un PCH avant le contenu de nos .cpp : un
# `#define _CRT_SECURE_NO_WARNINGS` en tete de fichier arrive donc trop tard.
# Il faut le definir au niveau de la cible pour faire taire les avertissements
# de depreciation MSVC (sscanf & co).
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Impossible de trouver le SDK Geode. Installe le CLI Geode et fais `geode sdk install`.")
else()
    message(STATUS "Utilisation du SDK Geode : $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

setup_geode_mod(${PROJECT_NAME})
