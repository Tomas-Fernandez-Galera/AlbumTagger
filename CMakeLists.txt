cmake_minimum_required(VERSION 3.24)

project(AlbumTagger VERSION 1.0.0 LANGUAGES CXX)

# C++20 es la única variante admitida por el proyecto. Desactivar extensiones
# evita depender accidentalmente de comportamientos exclusivos de MinGW.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Automatizaciones de Qt: señales/slots, formularios .ui y recursos .qrc.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/forms")

# Widgets construye la interfaz de escritorio; Network atiende MusicBrainz y CAA.
find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Network)

# TagLib se fija a una versión concreta para que una actualización remota no
# cambie el comportamiento de escritura de tags sin revisión del proyecto.
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    TagLib
    GIT_REPOSITORY https://github.com/taglib/taglib.git
    GIT_TAG v2.3.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(TagLib)

# WIN32 evita abrir una consola adicional al ejecutar la aplicación gráfica.
qt_add_executable(AlbumTagger WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/languages.cpp
    src/trackinfo.h
    src/tagservice.cpp
    src/tagservice.h
    forms/mainwindow.ui
    assets/assets.qrc
    assets/windows/appicon.rc
)

# TagLib 2.3.1 no publica todas sus rutas de headers cuando se integra mediante
# FetchContent; por eso se añaden explícitamente las tres rutas siguientes.
target_link_libraries(AlbumTagger PRIVATE Qt6::Widgets Qt6::Network tag)
target_include_directories(AlbumTagger PRIVATE
    "${taglib_SOURCE_DIR}/taglib"
    "${taglib_SOURCE_DIR}/taglib/toolkit"
    "${taglib_BINARY_DIR}"
)
target_compile_definitions(AlbumTagger PRIVATE
    ALBUMTAGGER_VERSION="${PROJECT_VERSION}"
)

# Avisos estrictos durante desarrollo para detectar conversiones y código dudoso.
if(MINGW)
    target_compile_options(AlbumTagger PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS AlbumTagger RUNTIME DESTINATION .)
