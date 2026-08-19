#include <QCoreApplication>
#include <QFileInfo>
#include <QProcess>

/**
 * Puente exclusivo para el proyecto AlbumTagger.pro.
 *
 * Qt Creator exige que los proyectos QMake tengan un ejecutable asociado para
 * habilitar el botón Ejecutar. La aplicación real se construye mediante CMake;
 * este pequeño proceso únicamente la inicia y termina inmediatamente.
 */
int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    const QString executable = QString::fromUtf8(ALBUMTAGGER_EXECUTABLE);
    if (!QFileInfo::exists(executable))
        return 1;

    return QProcess::startDetached(executable) ? 0 : 2;
}
