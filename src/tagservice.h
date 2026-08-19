#pragma once

#include "trackinfo.h"

#include <QList>
#include <QString>

/**
 * Frontera entre la interfaz Qt y TagLib.
 *
 * Centralizar aquí el acceso a archivos evita que MainWindow conozca las
 * diferencias entre ID3, Vorbis Comments, MP4 atoms, etc. Todos los métodos son
 * estáticos porque el servicio no conserva estado entre operaciones.
 */
class TagService
{
public:
    /// Busca audio recursivamente, lee sus tags, ordena y analiza las pistas.
    static QList<TrackInfo> scanFolder(const QString &folder);

    /// Rellena TrackInfo::issues sin modificar ningún archivo.
    static void analyze(QList<TrackInfo> &tracks);

    /**
     * Escribe únicamente campos comunes de álbum.
     * Se conserva para operaciones masivas sencillas; la tabla usa
     * writeTrackFields(), que admite valores distintos por pista.
     */
    static bool writeAlbumFields(const TrackInfo &track, const AlbumFields &fields,
                                 QString *errorMessage);

    /**
     * Guarda una fila editada.
     * `original` identifica el archivo y `edited` contiene la vista previa.
     * Si applyCommonFields es true, los valores comunes explícitos tienen
     * prioridad sobre los de la fila.
     */
    static bool writeTrackFields(const TrackInfo &original, const TrackInfo &edited,
                                 const AlbumFields &albumFields, bool applyCommonFields,
                                 QString *errorMessage);

    /**
     * Crea un JSON recuperable antes de cualquier escritura.
     * La operación de guardado debe cancelarse si este método devuelve false.
     */
    static bool createBackup(const QString &albumFolder, const QList<TrackInfo> &tracks,
                             QString *backupPath, QString *errorMessage);

    /// Extensiones que AlbumTagger permite escanear o copiar a un álbum.
    static QStringList supportedExtensions();

private:
    /// Lee un archivo individual y traduce TagLib a tipos de Qt.
    static TrackInfo readFile(const QString &path);
};
