# AlbumTagger

Copyright © 2026 Tomás Fernández Galera.

El código fuente se distribuye bajo la [GNU General Public License versión 3](LICENSE). El nombre AlbumTagger, su icono y su identidad visual no se conceden bajo la GPL; consulta [TRADEMARKS.md](TRADEMARKS.md).

Aplicación de escritorio para Windows que detecta y corrige diferencias de metadatos capaces de dividir un álbum en varios álbumes «fantasma». Está especialmente optimizada para preparar bibliotecas musicales utilizadas con moOde Audio, aunque funciona con otros reproductores y servidores compatibles con etiquetas estándar.

AlbumTagger es un proyecto independiente y no está afiliado ni respaldado oficialmente por moOde Audio.

## Funciones principales

- Escaneo recursivo de MP3, FLAC, OGG, Opus, M4A y formatos compatibles con TagLib.
- Edición de título, artista, álbum, artista del álbum, género, año y pista.
- Recopilatorios con artistas y años diferentes por pista.
- Detección de etiquetas ausentes o inconsistentes y pistas repetidas.
- Reordenación visual y renumeración opcional.
- Portadas externas JPG/PNG como `folder.*` o `cover.*`; no se incrustan en el audio.
- Consulta de MusicBrainz y Cover Art Archive.
- Copia de seguridad JSON antes de modificar etiquetas.
- Interfaz en 15 idiomas.

## Abrir en Qt Creator

Requisitos:

- Windows 10 u 11.
- Qt Creator con un kit de escritorio Qt 6.5 o posterior.
- CMake 3.24 o posterior.
- Git y conexión a Internet durante la primera configuración, para descargar TagLib 2.3.1.

Pasos:

1. Ejecuta `Abrir-AlbumTagger-en-Qt-Creator.cmd` o abre `AlbumTagger.pro`.
2. Cuando Qt Creator lo solicite, selecciona el kit de escritorio Qt 6.
3. Elige Debug para desarrollar o Release para preparar la distribución.
4. Pulsa «Configure Project» y compila el objetivo `AlbumTagger`.

`AlbumTagger.pro` solo sirve como entrada estable para Qt Creator y muestra todos los archivos inmediatamente. La compilación real continúa definida exclusivamente en `CMakeLists.txt`; el objetivo QMake delega en CMake sin duplicar reglas.

## Compilar desde una terminal configurada para Qt

```powershell
cmake -S . -B out/build/debug -G Ninja -DCMAKE_PREFIX_PATH="RUTA_A_QT"
cmake --build out/build/debug
```

## Crear una versión portable

Después de compilar el preset Release:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\package-portable.ps1
```

El script localiza `windeployqt.exe` mediante `PATH`. También puede indicarse la instalación explícitamente con `-QtRoot "RUTA_A_QT"`. El resultado queda en `dist/AlbumTagger-1.0.0-Windows-x64` y en el ZIP del mismo nombre.

## Seguridad

Antes de utilizarlo con una colección importante, prueba con una copia de un álbum. Las etiquetas originales se registran en `.albumtagger/backups` antes de cada escritura. Conserva las copias de seguridad hasta haber comprobado el resultado en tu reproductor habitual.
