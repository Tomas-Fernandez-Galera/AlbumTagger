#pragma once

#include <QString>
#include <QStringList>

/**
 * Snapshot de una pista tal como fue leída del archivo.
 *
 * La interfaz trabaja con copias de esta estructura. De ese modo puede mostrar
 * y editar valores sin tocar el archivo hasta que el usuario pulse
 * "Aplicar cambios". `issues` pertenece al análisis, no se guarda como tag.
 */
struct TrackInfo
{
    // Identidad del archivo. `path` es absoluto; `fileName` se usa en la tabla.
    QString path;
    QString fileName;

    // Tags musicales que pueden mostrarse o modificarse.
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    // Se conserva como texto porque formatos como "1/2" son válidos.
    QString discText;
    unsigned int year = 0;
    unsigned int track = 0;

    // Propiedad técnica de solo lectura; no forma parte de los cambios de tags.
    int durationSeconds = 0;

    // Estado de lectura y clasificación del archivo.
    bool readable = false;
    bool compilation = false;
    QString error;
    QStringList issues;
};

/**
 * Valores comunes que el panel derecho puede aplicar a varias pistas.
 *
 * En recopilatorios `artist` puede quedar vacío intencionadamente para no
 * sustituir los artistas individuales de cada canción.
 */
struct AlbumFields
{
    QString album;
    QString artist;
    QString albumArtist;
    QString genre;
    unsigned int year = 0;
    bool compilation = false;
};
