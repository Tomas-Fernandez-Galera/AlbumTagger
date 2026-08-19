# Proyecto de entrada para Qt Creator.
#
# La compilación real permanece en CMakeLists.txt. Este archivo evita que una
# instalación nueva de Qt Creator muestre únicamente CMakeLists.txt mientras
# espera que el usuario configure un kit. QMake presenta inmediatamente todo el
# árbol y delega el botón Compilar en CMake.

TEMPLATE = app
QT = core
TARGET = AlbumTaggerQtCreator
CONFIG -= console debug_and_release debug_and_release_target release
CONFIG += debug

# Este ejecutable diminuto solo proporciona a Qt Creator un botón Ejecutar
# válido. Inicia el AlbumTagger real, que continúa compilándose con CMake.
SOURCES = scripts/qtcreator_launcher.cpp

OTHER_FILES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/languages.cpp \
    src/tagservice.cpp \
    src/mainwindow.h \
    src/tagservice.h \
    src/trackinfo.h \
    forms/mainwindow.ui \
    assets/assets.qrc \
    CMakeLists.txt \
    README.md \
    LICENSE \
    COPYRIGHT.md \
    TRADEMARKS.md \
    scripts/package-portable.ps1 \
    assets/app-icon.svg \
    assets/windows/appicon.rc

BUILD_DIR = $$clean_path($$PWD/out/build/qtcreator)
QT_CMAKE = $$clean_path($$[QT_INSTALL_BINS]/qt-cmake.bat)
DEFINES += ALBUMTAGGER_EXECUTABLE=\\\"$$BUILD_DIR/AlbumTagger.exe\\\"

cmake_configure.target = cmake-configure
cmake_configure.commands = $$shell_quote($$QT_CMAKE) -S $$shell_quote($$PWD) -B $$shell_quote($$BUILD_DIR) -G Ninja -DCMAKE_BUILD_TYPE=Debug

cmake_build.target = cmake-build
cmake_build.depends = cmake_configure
cmake_build.commands = cmake --build $$shell_quote($$BUILD_DIR) --parallel 4

first.depends = cmake_build
PRE_TARGETDEPS += cmake-build
QMAKE_EXTRA_TARGETS += cmake_configure cmake_build first
