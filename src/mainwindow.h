#pragma once

#include "trackinfo.h"

#include <QMainWindow>
#include <QNetworkAccessManager>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QPushButton;

/**
 * Ventana principal y coordinador de la aplicación.
 *
 * Mantiene una instantánea de las pistas (`tracks_`), refleja esa instantánea
 * en la tabla y delega el acceso real a los tags en TagService. Las operaciones
 * de red son asíncronas para no bloquear la interfaz.
 */
class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Gestión de carpetas y archivos de audio.
    void chooseFolder();
    void createAlbum();
    void addAudioFiles();
    void showAbout();
    void rescan();

    // Aplica los datos comunes únicamente a la tabla; todavía no escribe tags.
    void applyAlbumFields();

    // Portadas externas. Por decisión del proyecto nunca se incrustan en audio.
    void chooseLocalCover();
    void downloadCover();
    void searchMusicBrainz();
    void saveCoverAs();

    // Filtros, sugerencias y orden manual de la tabla.
    void albumFilterChanged();
    void applySuggestions();
    void renumberVisibleRows();
    void saveAlbum();
    void moveSelectedRows(int direction);
    void restoreOriginalOrder();
    void saveAlbumToDisk();

private:
    /// Ejecuta el ciclo completo leer -> analizar -> mostrar.
    void scan(const QString &folder);

    /// Copia audio al álbum sin mover originales ni sobrescribir destinos.
    void importAudioFiles(const QStringList &paths);

    /// Acumula selecciones realizadas en tantas carpetas de origen como hagan falta.
    QStringList selectAudioFilesFromMultipleFolders();

    // Reconstrucción y resumen de la vista a partir de tracks_.
    void populateTable();
    void updateSummary();
    void updateAlbumFilter();

    // Gestión de archivos de portada externos.
    void loadExistingCover();
    void setCoverImage(const QByteArray &data, const QString &sourceName);
    void fetchCoverUrl(const QUrl &url);

    /// Consulta MusicBrainz con reintentos limitados ante 429/503.
    void requestMusicBrainz(const QUrl &url, int attempt);

    /// Bloqueo breve usado solo por trabajos locales síncronos.
    void setBusy(bool busy, const QString &message = {});

    // Internacionalización propia de los textos visibles.
    void setupLanguages();
    void applyLanguage(const QString &code);
    void updateArtistPlaceholder(bool compilation);
    QString uiText(const QString &key, int number = -1) const;
    /// Convierte un identificador de validación estable en texto visible.
    QString issueText(const QString &issueId, const QString &detail = {}) const;
    QString localizedIssues(const TrackInfo &track) const;

    // Utilidades para calcular valores comunes y el subconjunto visible.
    QString commonText(QString TrackInfo::*member) const;
    bool trackMatchesFilter(const TrackInfo &track) const;
    QList<TrackInfo> filteredTracks() const;
    QString majorityTableValue(int column, bool ignoreEmpty = true) const;

    // Objeto generado automáticamente desde forms/mainwindow.ui.
    Ui::MainWindow *ui;

    // Estado de la sesión actual; los archivos solo cambian al guardar.
    QList<TrackInfo> tracks_;
    QString folder_;

    // Imagen de portada en memoria hasta que se guarda como folder.* o cover.*.
    QByteArray coverData_;
    QString coverExtension_ = QStringLiteral("jpg");

    // Un único gestor reutilizable para MusicBrainz, CAA y URLs directas.
    QNetworkAccessManager network_;

    // Distingue campos comunes editados por el usuario de valores mostrados
    // automáticamente. Evita sobrescribir artistas/años individuales.
    bool commonFieldsDirty_ = false;
    // Permite distinguir una decisión explícita sobre el nombre del álbum de
    // una mera sugerencia estadística mostrada por la interfaz.
    bool albumFieldDirty_ = false;
    // Tras releer desde disco, el modelo ya viene ordenado por los nuevos
    // números de pista y no debe conservar además la permutación visual previa.
    bool resetRowOrderOnNextPopulate_ = false;
    bool rowOrderDirty_ = false;
    QPushButton *saveAlbumButton_ = nullptr;
    QString languageCode_ = QStringLiteral("es");
};
