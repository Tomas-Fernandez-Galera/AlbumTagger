# AlbumTagger

Copyright © 2026 Tomás Fernández Galera.

AlbumTagger is a Windows desktop application that detects and fixes metadata differences capable of splitting one album into several “ghost albums”. It is especially optimized for preparing music libraries used with moOde Audio, while remaining compatible with other players and servers that support standard tags.

AlbumTagger is an independent project and is not officially affiliated with or endorsed by moOde Audio.

The source code is distributed under the [GNU General Public License version 3](LICENSE). The AlbumTagger name, icon and visual identity are not licensed under the GPL; see [TRADEMARKS.md](TRADEMARKS.md).

## Main features

- Recursive scanning of MP3, FLAC, OGG, Opus, M4A and other TagLib-compatible formats.
- Editing of title, track artist, album, album artist, genre, year and track number.
- Compilation support with different artists and years for individual tracks.
- Detection of missing or inconsistent tags and duplicate track numbers.
- Visual reordering and optional track renumbering.
- External JPG/PNG artwork as `folder.*` or `cover.*`; artwork is not embedded in audio files.
- MusicBrainz and Cover Art Archive lookup.
- JSON backup before tag changes are written.
- User interface available in 15 languages.

## Open in Qt Creator

Requirements:

- Windows 10 or 11.
- Qt Creator with a Qt 6.5 or later desktop kit.
- CMake 3.24 or later.
- Git and an Internet connection during the first configuration to download TagLib 2.3.1.

Steps:

1. Run `Abrir-AlbumTagger-en-Qt-Creator.cmd` or open `AlbumTagger.pro`.
2. Select a Qt 6 desktop kit when Qt Creator asks.
3. Choose Debug for development or Release for distribution.
4. Click **Configure Project** and build the `AlbumTagger` target.

`AlbumTagger.pro` provides a stable Qt Creator entry point and immediately displays all project files. The actual build remains defined exclusively in `CMakeLists.txt`; the QMake target delegates to CMake without duplicating build rules.

## Build from a Qt-configured terminal

```powershell
cmake -S . -B out/build/debug -G Ninja -DCMAKE_PREFIX_PATH="PATH_TO_QT"
cmake --build out/build/debug
```

## Create a portable build

After building the Release configuration:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-portable.ps1
```

The script locates `windeployqt.exe` through `PATH`. You can also specify the Qt installation with `-QtRoot "PATH_TO_QT"`. The result is written to `dist/AlbumTagger-1.0.0-Windows-x64` and a ZIP file with the same name.

## Safety

Test the application on a copy of an album before using it with an important collection. Original tags are recorded in `.albumtagger/backups` before each write operation. Keep these backups until the result has been verified in your usual music player.

---

## Español

Aplicación de escritorio para Windows que detecta y corrige diferencias de metadatos capaces de dividir un álbum en varios álbumes «fantasma». Está especialmente optimizada para preparar bibliotecas musicales utilizadas con moOde Audio, aunque funciona con otros reproductores y servidores compatibles con etiquetas estándar.

AlbumTagger es un proyecto independiente y no está afiliado ni respaldado oficialmente por moOde Audio.

El código fuente se distribuye bajo la [GNU General Public License versión 3](LICENSE). El nombre AlbumTagger, su icono y su identidad visual no se conceden bajo la GPL; consulta [TRADEMARKS.md](TRADEMARKS.md).

### Funciones principales

- Escaneo recursivo de MP3, FLAC, OGG, Opus, M4A y formatos compatibles con TagLib.
- Edición de título, artista, álbum, artista del álbum, género, año y pista.
- Recopilatorios con artistas y años diferentes por pista.
- Detección de etiquetas ausentes o inconsistentes y pistas repetidas.
- Reordenación visual y renumeración opcional.
- Portadas externas JPG/PNG como `folder.*` o `cover.*`; no se incrustan en el audio.
- Consulta de MusicBrainz y Cover Art Archive.
- Copia de seguridad JSON antes de modificar etiquetas.
- Interfaz en 15 idiomas.

### Abrir en Qt Creator

Requisitos:

- Windows 10 u 11.
- Qt Creator con un kit de escritorio Qt 6.5 o posterior.
- CMake 3.24 o posterior.
- Git y conexión a Internet durante la primera configuración, para descargar TagLib 2.3.1.

Pasos:

1. Ejecuta `Abrir-AlbumTagger-en-Qt-Creator.cmd` o abre `AlbumTagger.pro`.
2. Cuando Qt Creator lo solicite, selecciona el kit de escritorio Qt 6.
3. Elige Debug para desarrollar o Release para preparar la distribución.
4. Pulsa **Configure Project** y compila el objetivo `AlbumTagger`.

`AlbumTagger.pro` sirve como entrada estable para Qt Creator y muestra todos los archivos inmediatamente. La compilación real continúa definida exclusivamente en `CMakeLists.txt`; el objetivo QMake delega en CMake sin duplicar reglas.

### Compilar desde una terminal configurada para Qt

```powershell
cmake -S . -B out/build/debug -G Ninja -DCMAKE_PREFIX_PATH="RUTA_A_QT"
cmake --build out/build/debug
```

### Crear una versión portable

Después de compilar la configuración Release:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-portable.ps1
```

El script localiza `windeployqt.exe` mediante `PATH`. También puede indicarse la instalación explícitamente con `-QtRoot "RUTA_A_QT"`. El resultado queda en `dist/AlbumTagger-1.0.0-Windows-x64` y en el ZIP del mismo nombre.

### Seguridad

Antes de utilizarlo con una colección importante, prueba con una copia de un álbum. Las etiquetas originales se registran en `.albumtagger/backups` antes de cada escritura. Conserva las copias de seguridad hasta haber comprobado el resultado en tu reproductor habitual.
