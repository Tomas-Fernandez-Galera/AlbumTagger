#include "tagservice.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <algorithm>

#include <fileref.h>
#include <tag.h>
#include <tpropertymap.h>

namespace {
// TagLib no usa QString. Estas dos funciones son el único punto de conversión
// y fuerzan UTF-8 para conservar correctamente títulos internacionales.
QString fromTagString(const TagLib::String &value)
{
    return QString::fromUtf8(value.toCString(true));
}

TagLib::String toTagString(const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    return TagLib::String(utf8.constData(), TagLib::String::UTF8);
}

QString property(const TagLib::PropertyMap &properties, const char *name)
{
    const auto values = properties[toTagString(QString::fromLatin1(name))];
    return values.isEmpty() ? QString{} : fromTagString(values.front());
}

QString comparisonKey(QString value)
{
    // Dos tags visualmente iguales pueden diferir por Unicode, espacios o
    // mayúsculas. La clave normalizada evita falsos álbumes "fantasma".
    value = value.normalized(QString::NormalizationForm_C).trimmed();
    value.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return value.toCaseFolded();
}

}

QStringList TagService::supportedExtensions()
{
    return {QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("ogg"),
            QStringLiteral("opus"), QStringLiteral("m4a"), QStringLiteral("mp4"),
            QStringLiteral("aac"), QStringLiteral("wav"), QStringLiteral("aiff"),
            QStringLiteral("ape"), QStringLiteral("wv"), QStringLiteral("wma")};
}

QList<TrackInfo> TagService::scanFolder(const QString &folder)
{
    QList<TrackInfo> tracks;
    const QStringList extensions = supportedExtensions();
    QDirIterator it(folder, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString path = it.next();
        if (extensions.contains(QFileInfo(path).suffix().toLower()))
            tracks.append(readFile(path));
    }
    // Orden inicial estable: disco, número de pista y, como desempate, archivo.
    // El usuario puede alterar después el orden visual desde la tabla.
    std::sort(tracks.begin(), tracks.end(), [](const TrackInfo &a, const TrackInfo &b) {
        if (a.discText != b.discText)
            return a.discText.localeAwareCompare(b.discText) < 0;
        if (a.track != b.track)
            return a.track < b.track;
        return a.fileName.localeAwareCompare(b.fileName) < 0;
    });
    analyze(tracks);
    return tracks;
}

TrackInfo TagService::readFile(const QString &path)
{
    TrackInfo result;
    result.path = path;
    result.fileName = QFileInfo(path).fileName();

    const QByteArray nativePath = QFile::encodeName(path);
    // AudioProperties::Fast evita analizar todo el flujo de audio solo para
    // obtener duración y tags, algo importante en carpetas grandes.
    TagLib::FileRef file(nativePath.constData(), true, TagLib::AudioProperties::Fast);
    if (file.isNull() || !file.tag()) {
        result.error = QStringLiteral("No se pudieron leer las etiquetas");
        return result;
    }

    const TagLib::Tag *tag = file.tag();
    result.title = fromTagString(tag->title());
    result.artist = fromTagString(tag->artist());
    result.album = fromTagString(tag->album());
    result.genre = fromTagString(tag->genre());
    result.year = tag->year();
    result.track = tag->track();
    if (file.audioProperties())
        result.durationSeconds = file.audioProperties()->lengthInSeconds();

    // La API Tag básica no expone Album Artist, Disc o Compilation. PropertyMap
    // ofrece una representación común para los diferentes formatos.
    const auto properties = file.file()->properties();
    result.albumArtist = property(properties, "ALBUMARTIST");
    if (result.albumArtist.isEmpty())
        result.albumArtist = property(properties, "ALBUM ARTIST");
    result.discText = property(properties, "DISCNUMBER");
    const QString compilation = property(properties, "COMPILATION").trimmed().toLower();
    result.compilation = compilation == QStringLiteral("1") || compilation == QStringLiteral("true") ||
                         compilation == QStringLiteral("yes");
    result.readable = true;
    return result;
}

void TagService::analyze(QList<TrackInfo> &tracks)
{
    // La clave incluye álbum y disco: la pista 1 de dos álbumes diferentes o
    // del disco 2 no debe considerarse duplicada.
    QSet<QString> trackNumbers;

    // Una carpeta de álbum debería tener un único nombre de álbum. Agrupar por
    // directorio físico evita comparar entre sí álbumes válidos encontrados al
    // escanear recursivamente una carpeta de música completa.
    QHash<QString, QHash<QString, int>> albumCountsByDirectory;
    for (const auto &track : tracks) {
        if (!track.readable || track.album.trimmed().isEmpty()) continue;
        const QString directory = QFileInfo(track.path).absolutePath();
        ++albumCountsByDirectory[directory][comparisonKey(track.album)];
    }
    QHash<QString, QString> dominantAlbumByDirectory;
    QSet<QString> directoriesWithoutUniqueMajority;
    for (auto directoryIt = albumCountsByDirectory.cbegin(); directoryIt != albumCountsByDirectory.cend(); ++directoryIt) {
        QString dominant;
        int bestCount = 0;
        bool tied = false;
        for (auto albumIt = directoryIt.value().cbegin(); albumIt != directoryIt.value().cend(); ++albumIt) {
            if (albumIt.value() > bestCount) {
                dominant = albumIt.key();
                bestCount = albumIt.value();
                tied = false;
            } else if (albumIt.value() == bestCount) {
                tied = true;
            }
        }
        if (directoryIt.value().size() > 1) {
            if (tied) directoriesWithoutUniqueMajority.insert(directoryIt.key());
            else dominantAlbumByDirectory.insert(directoryIt.key(), dominant);
        }
    }

    for (auto &track : tracks) {
        track.issues.clear();
        if (!track.readable) {
            track.issues << QStringLiteral("readError");
            continue;
        }
        // Los avisos se guardan como identificadores estables. La ventana los
        // traduce al mostrarlos; la validación nunca depende del idioma activo.
        if (track.title.trimmed().isEmpty()) track.issues << QStringLiteral("missingTitle");
        if (track.artist.trimmed().isEmpty()) track.issues << QStringLiteral("missingArtist");
        if (track.album.trimmed().isEmpty()) track.issues << QStringLiteral("missingAlbum");
        if (track.albumArtist.trimmed().isEmpty()) track.issues << QStringLiteral("missingAlbumArtist");
        if (track.track == 0) track.issues << QStringLiteral("missingTrackNumber");
        const QString directory = QFileInfo(track.path).absolutePath();
        if (!track.album.trimmed().isEmpty() &&
            (directoriesWithoutUniqueMajority.contains(directory) ||
             (dominantAlbumByDirectory.contains(directory) &&
              comparisonKey(track.album) != dominantAlbumByDirectory.value(directory)))) {
            track.issues << QStringLiteral("inconsistentAlbumName");
        }
        const QString trackKey = comparisonKey(track.album) + QLatin1Char('|') + track.discText +
                                 QLatin1Char('|') + QString::number(track.track);
        if (track.track > 0 && trackNumbers.contains(trackKey))
            track.issues << QStringLiteral("duplicateTrackNumber");
        if (track.track > 0) trackNumbers.insert(trackKey);
        // Las coherencias se comprueban solo contra pistas del mismo álbum.
        // Una carpeta raíz puede contener varios álbumes completamente válidos.
        QSet<QString> albumArtists;
        QSet<QString> genres;
        for (const auto &peer : tracks) {
            if (!peer.readable || comparisonKey(peer.album) != comparisonKey(track.album)) continue;
            albumArtists.insert(comparisonKey(peer.albumArtist));
            genres.insert(comparisonKey(peer.genre));
        }
        if (albumArtists.size() > 1) track.issues << QStringLiteral("inconsistentAlbumArtist");
        if (genres.size() > 1) track.issues << QStringLiteral("inconsistentGenre");
    }
}

bool TagService::createBackup(const QString &albumFolder, const QList<TrackInfo> &tracks,
                              QString *backupPath, QString *errorMessage)
{
    QDir root(albumFolder);
    if (!root.mkpath(QStringLiteral(".albumtagger/backups"))) {
        if (errorMessage) *errorMessage = QStringLiteral("No se pudo crear la carpeta de copias de seguridad");
        return false;
    }
    // Los milisegundos evitan colisiones si se guardan dos veces rápidamente.
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString path = root.filePath(QStringLiteral(".albumtagger/backups/tags-%1.json").arg(stamp));
    QJsonArray items;
    for (const auto &track : tracks) {
        QJsonObject item;
        item.insert(QStringLiteral("path"), root.relativeFilePath(track.path));
        item.insert(QStringLiteral("title"), track.title);
        item.insert(QStringLiteral("artist"), track.artist);
        item.insert(QStringLiteral("album"), track.album);
        item.insert(QStringLiteral("albumArtist"), track.albumArtist);
        item.insert(QStringLiteral("genre"), track.genre);
        item.insert(QStringLiteral("year"), static_cast<int>(track.year));
        item.insert(QStringLiteral("track"), static_cast<int>(track.track));
        item.insert(QStringLiteral("disc"), track.discText);
        item.insert(QStringLiteral("compilation"), track.compilation);
        items.append(item);
    }
    QJsonObject document;
    document.insert(QStringLiteral("formatVersion"), 1);
    document.insert(QStringLiteral("createdAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    document.insert(QStringLiteral("albumFolder"), QDir::toNativeSeparators(albumFolder));
    document.insert(QStringLiteral("tracks"), items);
    // QSaveFile escribe primero en un temporal y hace commit atómico. Así no
    // queda un backup JSON truncado si la aplicación o el equipo se detienen.
    QSaveFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(QJsonDocument(document).toJson(QJsonDocument::Indented)) < 0 || !output.commit()) {
        if (errorMessage) *errorMessage = QStringLiteral("No se pudo escribir la copia de seguridad: %1").arg(output.errorString());
        return false;
    }
    if (backupPath) *backupPath = path;
    return true;
}

bool TagService::writeAlbumFields(const TrackInfo &track, const AlbumFields &fields,
                                  QString *errorMessage)
{
    const QByteArray nativePath = QFile::encodeName(track.path);
    TagLib::FileRef file(nativePath.constData());
    if (file.isNull() || !file.tag()) {
        if (errorMessage) *errorMessage = QStringLiteral("No se puede abrir %1").arg(track.fileName);
        return false;
    }

    file.tag()->setAlbum(toTagString(fields.album));
    if (!fields.artist.isEmpty())
        file.tag()->setArtist(toTagString(fields.artist));
    file.tag()->setGenre(toTagString(fields.genre));
    file.tag()->setYear(fields.year);
    auto properties = file.file()->properties();
    properties.replace(toTagString(QStringLiteral("ALBUMARTIST")),
                       TagLib::StringList(toTagString(fields.albumArtist)));
    if (fields.compilation) {
        properties.replace(toTagString(QStringLiteral("COMPILATION")),
                           TagLib::StringList(toTagString(QStringLiteral("1"))));
    } else {
        properties.erase(toTagString(QStringLiteral("COMPILATION")));
    }
    file.file()->setProperties(properties);
    if (!file.save()) {
        if (errorMessage) *errorMessage = QStringLiteral("No se pudo guardar %1").arg(track.fileName);
        return false;
    }
    return true;
}

bool TagService::writeTrackFields(const TrackInfo &original, const TrackInfo &edited,
                                  const AlbumFields &albumFields, bool applyCommonFields,
                                  QString *errorMessage)
{
    const QByteArray nativePath = QFile::encodeName(original.path);
    TagLib::FileRef file(nativePath.constData());
    if (file.isNull() || !file.tag()) {
        if (errorMessage) *errorMessage = QStringLiteral("No se puede abrir %1").arg(original.fileName);
        return false;
    }
    file.tag()->setTitle(toTagString(edited.title));
    file.tag()->setArtist(toTagString(edited.artist));
    file.tag()->setAlbum(toTagString(edited.album));
    file.tag()->setGenre(toTagString(edited.genre));
    file.tag()->setYear(edited.year);
    file.tag()->setTrack(edited.track);

    // Album Artist y Compilation deben escribirse mediante PropertyMap para que
    // TagLib los traduzca al contenedor adecuado (ID3, Vorbis, MP4, etc.).
    auto properties = file.file()->properties();
    const QString albumArtist = applyCommonFields && !albumFields.albumArtist.isEmpty()
        ? albumFields.albumArtist : edited.albumArtist;
    properties.replace(toTagString(QStringLiteral("ALBUMARTIST")),
                       TagLib::StringList(toTagString(albumArtist)));
    if (albumFields.compilation) {
        properties.replace(toTagString(QStringLiteral("COMPILATION")),
                           TagLib::StringList(toTagString(QStringLiteral("1"))));
    } else {
        properties.erase(toTagString(QStringLiteral("COMPILATION")));
    }
    file.file()->setProperties(properties);
    if (!file.save()) {
        if (errorMessage) *errorMessage = QStringLiteral("No se pudo guardar %1").arg(original.fileName);
        return false;
    }
    return true;
}
