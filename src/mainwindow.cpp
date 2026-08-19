#include "mainwindow.h"
#include "tagservice.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPixmap>
#include <QRegularExpression>
#include <QSet>
#include <QSignalBlocker>
#include <QScrollArea>
#include <QSplitter>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

namespace {
// Presentación compacta de una duración; no modifica propiedades del audio.
QString durationText(int seconds)
{
    return QStringLiteral("%1:%2").arg(seconds / 60).arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Crea los widgets descritos en forms/mainwindow.ui.
    ui->setupUi(this);
    setupLanguages();
    setWindowTitle(QStringLiteral("AlbumTagger"));
    // Reorganización dinámica: tabla y panel lateral se colocan en un splitter.
    // Así ambos lados pueden redimensionarse sin duplicar widgets en el .ui.
    ui->contentLayout->removeWidget(ui->trackTable);
    ui->contentLayout->removeItem(ui->sideLayout);
    auto *sidePanel = new QWidget(this);
    sidePanel->setLayout(ui->sideLayout);
    sidePanel->setMinimumWidth(430);
    sidePanel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    auto *sideScroll = new QScrollArea(this);
    sideScroll->setObjectName(QStringLiteral("sideScroll"));
    sideScroll->setWidgetResizable(true);
    sideScroll->setFrameShape(QFrame::NoFrame);
    sideScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sideScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    sideScroll->setMinimumWidth(290);
    sideScroll->setWidget(sidePanel);
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("contentSplitter"));
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(ui->trackTable);
    splitter->addWidget(sideScroll);
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({820, 380});
    ui->contentLayout->addWidget(splitter);

    // La tabla tiene más columnas que el ancho habitual de pantalla. Se permiten
    // tamaños y posiciones manuales y se mantiene el scroll horizontal.
    auto *header = ui->trackTable->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Interactive);
    header->setSectionsMovable(true);
    header->setStretchLastSection(false);
    header->setMinimumSectionSize(45);
    ui->trackTable->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    ui->trackTable->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // El encabezado vertical es el asa desde la que se arrastran las filas.
    auto *rowHeader = ui->trackTable->verticalHeader();
    rowHeader->setSectionsMovable(true);
    rowHeader->setFirstSectionMovable(true);
    rowHeader->setHighlightSections(true);
    rowHeader->setMinimumSectionSize(24);
    rowHeader->setDefaultSectionSize(28);
    rowHeader->setToolTip(QStringLiteral("Arrastra el asa ☰ para cambiar el orden de las canciones"));
    ui->trackTable->setEditTriggers(QAbstractItemView::DoubleClicked |
                                    QAbstractItemView::SelectedClicked |
                                    QAbstractItemView::EditKeyPressed);
    ui->trackTable->setColumnWidth(0, 48);
    ui->trackTable->setColumnWidth(1, 220);
    ui->trackTable->setColumnWidth(2, 210);
    ui->trackTable->setColumnWidth(3, 170);
    ui->trackTable->setColumnWidth(4, 220);
    ui->trackTable->setColumnWidth(5, 190);
    ui->trackTable->setColumnWidth(6, 140);
    ui->trackTable->setColumnWidth(7, 65);
    ui->trackTable->setColumnWidth(8, 78);
    ui->trackTable->setColumnWidth(9, 340);

    // Controles alternativos al arrastre. Son especialmente útiles con varias
    // filas seleccionadas o para usuarios que trabajan solo con teclado.
    auto *moveUpButton = new QToolButton(this);
    moveUpButton->setObjectName(QStringLiteral("moveTracksUpButton"));
    moveUpButton->setText(QStringLiteral("▲"));
    moveUpButton->setToolTip(QStringLiteral("Subir las canciones seleccionadas"));
    auto *moveDownButton = new QToolButton(this);
    moveDownButton->setObjectName(QStringLiteral("moveTracksDownButton"));
    moveDownButton->setText(QStringLiteral("▼"));
    moveDownButton->setToolTip(QStringLiteral("Bajar las canciones seleccionadas"));
    auto *restoreOrderButton = new QToolButton(this);
    restoreOrderButton->setObjectName(QStringLiteral("restoreTrackOrderButton"));
    restoreOrderButton->setText(QStringLiteral("↶"));
    restoreOrderButton->setToolTip(QStringLiteral("Restaurar el orden original"));
    saveAlbumButton_ = new QPushButton(QStringLiteral("Guardar álbum"), this);
    saveAlbumButton_->setObjectName(QStringLiteral("saveAlbumButton"));
    saveAlbumButton_->setEnabled(false);
    saveAlbumButton_->setToolTip(QStringLiteral("Guardar todos los cambios realizados en el álbum"));
    const int controlsPosition = std::max(0, ui->albumFilterLayout->count() - 1);
    ui->albumFilterLayout->insertWidget(controlsPosition, moveUpButton);
    ui->albumFilterLayout->insertWidget(controlsPosition + 1, moveDownButton);
    ui->albumFilterLayout->insertWidget(controlsPosition + 2, restoreOrderButton);
    ui->albumFilterLayout->insertWidget(controlsPosition + 3, saveAlbumButton_);
    connect(moveUpButton, &QToolButton::clicked, this, [this] { moveSelectedRows(-1); });
    connect(moveDownButton, &QToolButton::clicked, this, [this] { moveSelectedRows(1); });
    connect(restoreOrderButton, &QToolButton::clicked, this, &MainWindow::restoreOriginalOrder);
    connect(saveAlbumButton_, &QPushButton::clicked, this, &MainWindow::saveAlbum);

    connect(ui->openFolderButton, &QPushButton::clicked, this, &MainWindow::chooseFolder);
    connect(ui->aboutButton, &QToolButton::clicked, this, &MainWindow::showAbout);
    connect(ui->createAlbumButton, &QPushButton::clicked, this, &MainWindow::createAlbum);
    connect(ui->addFilesButton, &QPushButton::clicked, this, &MainWindow::addAudioFiles);
    connect(ui->rescanButton, &QPushButton::clicked, this, &MainWindow::rescan);
    connect(ui->applyButton, &QPushButton::clicked, this, &MainWindow::applyAlbumFields);
    connect(ui->suggestButton, &QPushButton::clicked, this, &MainWindow::applySuggestions);
    connect(ui->albumEdit, &QLineEdit::textEdited, this, [this] {
        commonFieldsDirty_ = true;
        albumFieldDirty_ = true;
        // En la vista con varios nombres, escribir uno expresamente autoriza a
        // unificarlos. La mayoría detectada sigue siendo solo una sugerencia.
        ui->applyButton->setEnabled(!tracks_.isEmpty() && !ui->albumEdit->text().trimmed().isEmpty());
        ui->applyButton->setToolTip(QStringLiteral("Aplicar este nombre de álbum a todas las pistas visibles"));
    });
    connect(ui->artistEdit, &QLineEdit::textEdited, this, [this] { commonFieldsDirty_ = true; });
    connect(ui->albumArtistEdit, &QLineEdit::textEdited, this, [this] { commonFieldsDirty_ = true; });
    ui->genreEdit->addItems({QStringLiteral("Pop"), QStringLiteral("Rock"), QStringLiteral("Alternativo"),
        QStringLiteral("Indie"), QStringLiteral("Electrónica"), QStringLiteral("Dance"),
        QStringLiteral("Hip-Hop"), QStringLiteral("Rap"), QStringLiteral("R&B"),
        QStringLiteral("Soul"), QStringLiteral("Funk"), QStringLiteral("Jazz"),
        QStringLiteral("Blues"), QStringLiteral("Folk"), QStringLiteral("Country"),
        QStringLiteral("Reggae"), QStringLiteral("Metal"), QStringLiteral("Punk"),
        QStringLiteral("Clásica"), QStringLiteral("Flamenco"), QStringLiteral("Latina"),
        QStringLiteral("Banda sonora"), QStringLiteral("Otro")});
    ui->genreEdit->setCurrentIndex(-1);
    connect(ui->genreEdit, &QComboBox::editTextChanged, this, [this] { commonFieldsDirty_ = true; });
    connect(ui->yearSpin, &QSpinBox::editingFinished, this, [this] { commonFieldsDirty_ = true; });
    connect(ui->yearDownButton, &QToolButton::clicked, ui->yearSpin, &QSpinBox::stepDown);
    connect(ui->yearUpButton, &QToolButton::clicked, ui->yearSpin, &QSpinBox::stepUp);
    connect(ui->yearDownButton, &QToolButton::clicked, this, [this] { commonFieldsDirty_ = true; });
    connect(ui->yearUpButton, &QToolButton::clicked, this, [this] { commonFieldsDirty_ = true; });
    connect(ui->compilationCheck, &QCheckBox::clicked, this, [this] { commonFieldsDirty_ = true; });
    connect(ui->compilationCheck, &QCheckBox::clicked, this, [this] {
        populateTable();
        updateSummary();
    });
    // Las ediciones pendientes se pintan de azul. Al poblar programáticamente la
    // tabla se usa QSignalBlocker para que este callback no produzca falsos cambios.
    connect(ui->trackTable, &QTableWidget::itemChanged, this, [this](QTableWidgetItem *item) {
        item->setBackground(QColor(222, 235, 255));
        item->setToolTip(QStringLiteral("Cambio pendiente de guardar"));
        statusBar()->showMessage(QStringLiteral("Hay cambios pendientes. Pulsa Aplicar cambios para guardarlos."));
    });
    // En recopilatorios se conserva el artista individual de cada pista.
    connect(ui->compilationCheck, &QCheckBox::toggled, this, [this](bool compilation) {
        ui->artistEdit->setEnabled(!compilation);
        if (compilation && ui->albumArtistEdit->text().trimmed().isEmpty())
            ui->albumArtistEdit->setText(QStringLiteral("Various Artists"));
        updateArtistPlaceholder(compilation);
    });
    connect(ui->localCoverButton, &QPushButton::clicked, this, &MainWindow::chooseLocalCover);
    connect(ui->downloadCoverButton, &QPushButton::clicked, this, &MainWindow::downloadCover);
    connect(ui->musicBrainzButton, &QPushButton::clicked, this, &MainWindow::searchMusicBrainz);
    connect(ui->saveCoverButton, &QPushButton::clicked, this, &MainWindow::saveCoverAs);
    connect(ui->albumFilterCombo, &QComboBox::currentIndexChanged, this, &MainWindow::albumFilterChanged);
    connect(ui->renumberCheck, &QCheckBox::toggled, this, [this](bool enabled) {
        if (enabled) renumberVisibleRows();
        statusBar()->showMessage(enabled
            ? QStringLiteral("Los números de pista seguirán el orden visible al guardar")
            : QStringLiteral("El orden visual no modificará los números de pista"), 5000);
    });
    connect(rowHeader, &QHeaderView::sectionMoved, this, [this](int, int, int) {
        rowOrderDirty_ = true;
        if (saveAlbumButton_) saveAlbumButton_->setEnabled(true);
        if (ui->renumberCheck->isChecked()) renumberVisibleRows();
    });
    // setupLanguages se ejecuta antes de crear estos controles dinámicos. Una
    // segunda aplicación traduce también los botones añadidos por código.
    applyLanguage(languageCode_);
    statusBar()->showMessage(QStringLiteral("Selecciona una carpeta de música para comenzar"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::chooseFolder()
{
    const QString selected = QFileDialog::getExistingDirectory(this, QStringLiteral("Seleccionar carpeta del álbum"), folder_);
    if (!selected.isEmpty()) scan(selected);
}

void MainWindow::createAlbum()
{
    // Flujo: elegir carpeta padre -> sanear nombre -> crear directorio -> copiar
    // una selección múltiple. Los caracteres inválidos de Windows se sustituyen.
    const QString parent = QFileDialog::getExistingDirectory(this, QStringLiteral("Dónde crear el álbum"), folder_);
    if (parent.isEmpty()) return;
    bool accepted = false;
    QString name = QInputDialog::getText(this, QStringLiteral("Crear álbum"),
        QStringLiteral("Nombre de la carpeta del álbum:"), QLineEdit::Normal, {}, &accepted).trimmed();
    if (!accepted || name.isEmpty()) return;
    name.replace(QRegularExpression(QStringLiteral("[<>:\"/\\\\|?*]")), QStringLiteral("_"));
    name.remove(QRegularExpression(QStringLiteral("[ .]+$")));
    if (name.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Nombre no válido"), QStringLiteral("Escribe un nombre de carpeta válido."));
        return;
    }
    const QString albumFolder = QDir(parent).filePath(name);
    if (QFileInfo::exists(albumFolder)) {
        if (QMessageBox::question(this, QStringLiteral("La carpeta ya existe"),
            QStringLiteral("La carpeta ya existe. ¿Quieres utilizarla?") ) != QMessageBox::Yes) return;
    } else if (!QDir().mkpath(albumFolder)) {
        QMessageBox::critical(this, QStringLiteral("No se pudo crear"),
                              QStringLiteral("No se pudo crear la carpeta:\n%1").arg(QDir::toNativeSeparators(albumFolder)));
        return;
    }
    scan(albumFolder);
    addAudioFiles();
}

void MainWindow::addAudioFiles()
{
    if (folder_.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Primero crea o abre un álbum"),
                                 QStringLiteral("Selecciona la carpeta de destino antes de añadir canciones."));
        return;
    }
    const QStringList files = selectAudioFilesFromMultipleFolders();
    if (!files.isEmpty()) importAudioFiles(files);
}

QStringList MainWindow::selectAudioFilesFromMultipleFolders()
{
    QStringList patterns;
    for (const QString &extension : TagService::supportedExtensions())
        patterns << QStringLiteral("*.%1").arg(extension);
    const QString filter = QStringLiteral("Archivos de audio (%1);;Todos los archivos (*)")
                               .arg(patterns.join(QLatin1Char(' ')));

    // QFileDialog permite seleccionar muchos archivos, pero únicamente dentro
    // de una carpeta cada vez. Este bucle acumula lotes de distintos orígenes y
    // realiza una sola importación al terminar.
    QStringList selectedFiles;
    QSet<QString> selectedPaths;
    QString sourceFolder;
    while (true) {
        const QStringList batch = QFileDialog::getOpenFileNames(
            this, selectedFiles.isEmpty()
                      ? QStringLiteral("Seleccionar canciones para el álbum")
                      : QStringLiteral("Añadir otros archivos"),
            sourceFolder, filter);
        if (batch.isEmpty()) break;

        sourceFolder = QFileInfo(batch.first()).absolutePath();
        for (const QString &path : batch) {
            const QString absolutePath = QFileInfo(path).absoluteFilePath();
            if (!selectedPaths.contains(absolutePath)) {
                selectedPaths.insert(absolutePath);
                selectedFiles.append(absolutePath);
            }
        }

        const auto answer = QMessageBox::question(
            this, QStringLiteral("Archivos seleccionados"),
            QStringLiteral("Hay %1 canciones seleccionadas.\n\n¿Quieres añadir otros archivos?")
                .arg(selectedFiles.size()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer == QMessageBox::No) break;
    }
    return selectedFiles;
}

void MainWindow::importAudioFiles(const QStringList &paths)
{
    // Regla de seguridad: copiar siempre. Nunca mover o eliminar los originales.
    int copied = 0;
    QStringList errors;
    const QStringList extensions = TagService::supportedExtensions();
    for (const QString &source : paths) {
        const QFileInfo sourceInfo(source);
        if (!sourceInfo.isFile() || !extensions.contains(sourceInfo.suffix().toLower())) continue;
        QString target = QDir(folder_).filePath(sourceInfo.fileName());
        if (QFileInfo(source).absoluteFilePath().compare(QFileInfo(target).absoluteFilePath(), Qt::CaseInsensitive) == 0)
            continue;
        int suffix = 2;
        // Si el destino existe se genera "nombre (2).ext" en lugar de sobrescribir.
        while (QFileInfo::exists(target)) {
            target = QDir(folder_).filePath(QStringLiteral("%1 (%2).%3")
                .arg(sourceInfo.completeBaseName()).arg(suffix++).arg(sourceInfo.suffix()));
        }
        if (QFile::copy(source, target)) ++copied;
        else errors << sourceInfo.fileName();
    }
    scan(folder_);
    if (errors.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("%1 archivos añadidos al álbum").arg(copied), 6000);
    } else {
        QMessageBox::warning(this, QStringLiteral("Algunos archivos no se copiaron"), errors.join(QLatin1Char('\n')));
    }
}

void MainWindow::rescan()
{
    if (!folder_.isEmpty()) scan(folder_);
}

void MainWindow::scan(const QString &folder)
{
    // Punto único de refresco. Después de copiar o guardar se vuelve a leer el
    // disco para que la vista no dependa de una instantánea desactualizada.
    folder_ = folder;
    ui->folderPath->setText(QDir::toNativeSeparators(folder_));
    ui->addFilesButton->setEnabled(true);
    setBusy(true, QStringLiteral("Analizando archivos…"));
    tracks_ = TagService::scanFolder(folder_);
    commonFieldsDirty_ = false;
    albumFieldDirty_ = false;
    resetRowOrderOnNextPopulate_ = true;
    rowOrderDirty_ = false;
    if (saveAlbumButton_) saveAlbumButton_->setEnabled(!tracks_.isEmpty());
    loadExistingCover();
    updateAlbumFilter();
    populateTable();
    updateSummary();
    setBusy(false, QStringLiteral("%1 archivos analizados").arg(tracks_.size()));
}

void MainWindow::loadExistingCover()
{
    // Las portadas son exclusivamente archivos externos. Se prioriza folder.*
    // sobre cover.* y la búsqueda ignora mayúsculas/minúsculas.
    coverData_.clear();
    ui->coverPreview->setPixmap({});
    ui->coverPreview->setText(uiText(QStringLiteral("noCover")));
    ui->coverInfoLabel->setText(QStringLiteral("JPG o PNG"));
    ui->saveCoverButton->setEnabled(false);

    const QFileInfoList images = QDir(folder_).entryInfoList(
        {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"), QStringLiteral("*.png")},
        QDir::Files, QDir::Name | QDir::IgnoreCase);
    const QStringList preferredNames = {QStringLiteral("folder"), QStringLiteral("cover")};
    for (const QString &preferred : preferredNames) {
        for (const QFileInfo &image : images) {
            if (image.completeBaseName().compare(preferred, Qt::CaseInsensitive) != 0)
                continue;
            QFile file(image.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly)) {
                setCoverImage(file.readAll(), image.suffix());
                ui->coverInfoLabel->setText(ui->coverInfoLabel->text() +
                                            QStringLiteral(" · %1").arg(image.fileName()));
                return;
            }
        }
    }
}

QString MainWindow::commonText(QString TrackInfo::*member) const
{
    // El puntero a miembro permite reutilizar esta comparación con album, artist,
    // genre, etc. Un resultado vacío también puede significar "valores distintos".
    QString common;
    bool first = true;
    for (const auto &track : tracks_) {
        if (!track.readable || !trackMatchesFilter(track)) continue;
        const QString value = track.*member;
        if (first) { common = value; first = false; }
        else if (value.trimmed().compare(common.trimmed(), Qt::CaseInsensitive) != 0) return {};
    }
    return common;
}

bool MainWindow::trackMatchesFilter(const TrackInfo &track) const
{
    const QString selectedAlbum = ui->albumFilterCombo->currentData().toString();
    return selectedAlbum.isEmpty() || track.album == selectedAlbum;
}

QList<TrackInfo> MainWindow::filteredTracks() const
{
    QList<TrackInfo> result;
    for (const auto &track : tracks_)
        if (trackMatchesFilter(track)) result.append(track);
    return result;
}

void MainWindow::updateAlbumFilter()
{
    // "Todos los álbumes" sirve para diagnóstico. El guardado se desactiva en esa
    // vista para impedir que una corrección masiva mezcle álbumes diferentes.
    const QSignalBlocker blocker(ui->albumFilterCombo);
    ui->albumFilterCombo->clear();
    QSet<QString> albums;
    for (const auto &track : tracks_)
        if (track.readable && !track.album.trimmed().isEmpty()) albums.insert(track.album);
    QStringList sortedAlbums = albums.values();
    sortedAlbums.sort(Qt::CaseInsensitive);
    if (sortedAlbums.size() > 1)
        ui->albumFilterCombo->addItem(QStringLiteral("Todos los álbumes — solo diagnóstico"), QString{});
    for (const QString &album : sortedAlbums)
        ui->albumFilterCombo->addItem(album, album);
    if (ui->albumFilterCombo->count() == 0)
        ui->albumFilterCombo->addItem(QStringLiteral("Álbum sin identificar"), QString{});
}

void MainWindow::albumFilterChanged()
{
    populateTable();
    updateSummary();
}

void MainWindow::populateTable()
{
    // Reconstruir desde tracks_ simplifica filtros y relecturas. QSignalBlocker
    // impide que la carga inicial se interprete como cambios del usuario.
    const QList<TrackInfo> visibleTracks = filteredTracks();
    const QSignalBlocker blocker(ui->trackTable);
    ui->trackTable->setRowCount(visibleTracks.size());
    if (resetRowOrderOnNextPopulate_) {
        // Los números recién guardados ya han determinado el nuevo orden lógico
        // en TagService::scanFolder. Se elimina la antigua permutación visual
        // para no aplicarla una segunda vez sobre esas mismas canciones.
        QHeaderView *rowHeader = ui->trackTable->verticalHeader();
        const QSignalBlocker headerBlocker(rowHeader);
        for (int logicalRow = 0; logicalRow < rowHeader->count(); ++logicalRow)
            rowHeader->moveSection(rowHeader->visualIndex(logicalRow), logicalRow);
        resetRowOrderOnNextPopulate_ = false;
    }
    for (int row = 0; row < visibleTracks.size(); ++row) {
        auto *handle = new QTableWidgetItem(QStringLiteral("☰"));
        handle->setTextAlignment(Qt::AlignCenter);
        handle->setToolTip(QStringLiteral("Arrastra para mover esta canción"));
        ui->trackTable->setVerticalHeaderItem(row, handle);
    }
    // Las frecuencias calculan la propuesta mayoritaria. Una propuesta solo se
    // muestra en rojo; no llega al archivo hasta confirmación explícita.
    QHash<QString, int> albumArtistCounts;
    QHash<QString, int> albumCounts;
    QHash<QString, int> genreCounts;
    QHash<unsigned int, int> yearCounts;
    for (const auto &track : visibleTracks) {
        if (!track.album.trimmed().isEmpty()) ++albumCounts[track.album.trimmed()];
        if (!track.albumArtist.trimmed().isEmpty()) ++albumArtistCounts[track.albumArtist.trimmed()];
        if (!track.genre.trimmed().isEmpty()) ++genreCounts[track.genre.trimmed()];
        if (track.year > 0) ++yearCounts[track.year];
    }
    bool compilationMode = ui->compilationCheck->isChecked();
    for (const auto &track : visibleTracks) compilationMode = compilationMode || track.compilation;
    QString suggestedAlbumArtist;
    int bestArtistCount = 0;
    for (auto it = albumArtistCounts.cbegin(); it != albumArtistCounts.cend(); ++it)
        if (it.value() > bestArtistCount) { bestArtistCount = it.value(); suggestedAlbumArtist = it.key(); }
    QString suggestedAlbum;
    int bestAlbumCount = 0;
    for (auto it = albumCounts.cbegin(); it != albumCounts.cend(); ++it)
        if (it.value() > bestAlbumCount) { bestAlbumCount = it.value(); suggestedAlbum = it.key(); }
    unsigned int suggestedYear = 0;
    int bestYearCount = 0;
    for (auto it = yearCounts.cbegin(); it != yearCounts.cend(); ++it)
        if (it.value() > bestYearCount) { bestYearCount = it.value(); suggestedYear = it.key(); }
    QString suggestedGenre;
    int bestGenreCount = 0;
    for (auto it = genreCounts.cbegin(); it != genreCounts.cend(); ++it)
        if (it.value() > bestGenreCount) { bestGenreCount = it.value(); suggestedGenre = it.key(); }
    for (int row = 0; row < visibleTracks.size(); ++row) {
        const auto &track = visibleTracks.at(row);
        const QStringList values = {
            track.track ? QString::number(track.track) : QStringLiteral("—"), track.fileName,
            track.title, track.artist, track.album, track.albumArtist, track.genre,
            track.year ? QString::number(track.year) : QString{}, durationText(track.durationSeconds),
            track.issues.join(QStringLiteral(" · "))
        };
        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            item->setData(Qt::UserRole, track.path);
            if (column == 1 || column >= 8) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            // Cada celda decide si está incompleta/inconsistente y ofrece una
            // explicación concreta mediante tooltip.
            bool problem = false;
            QString suggestion;
            if (column == 0 && track.track == 0) { problem = true; suggestion = QStringLiteral("Introduce el número de pista"); }
            if (column == 2 && track.title.trimmed().isEmpty()) { problem = true; suggestion = QFileInfo(track.path).completeBaseName(); }
            if (column == 3 && track.artist.trimmed().isEmpty()) { problem = true; suggestion = QStringLiteral("Introduce el artista de la pista"); }
            if (column == 4 && (track.album.trimmed().isEmpty() || track.issues.contains(QStringLiteral("Nombre de álbum inconsistente")))) {
                problem = true;
                suggestion = suggestedAlbum;
                if (suggestion.isEmpty()) suggestion = ui->albumFilterCombo->currentData().toString();
            }
            if (column == 5 && (track.albumArtist.trimmed().isEmpty() || track.issues.contains(QStringLiteral("Artista del álbum inconsistente")))) {
                problem = true;
                suggestion = suggestedAlbumArtist;
                if (suggestion.isEmpty() && !ui->compilationCheck->isChecked()) suggestion = commonText(&TrackInfo::artist);
                if (suggestion.isEmpty() && ui->compilationCheck->isChecked()) suggestion = QStringLiteral("Various Artists");
            }
            if (column == 6 && track.issues.contains(QStringLiteral("Género inconsistente"))) {
                problem = true; suggestion = suggestedGenre;
            }
            // En recopilatorios el año es siempre un dato individual: no se
            // colorea ni se propone un valor común, incluso si alguna pista no
            // lo tiene o la mayoría coincide en el mismo año.
            if (column == 7 && !compilationMode &&
                (track.year == 0 || (yearCounts.size() > 1 && track.year != suggestedYear))) {
                problem = true; suggestion = suggestedYear ? QString::number(suggestedYear) : QStringLiteral("Introduce el año");
            }
            item->setData(Qt::UserRole + 1, problem);
            if (problem) {
                item->setBackground(QColor(255, 221, 221));
                item->setForeground(QColor(150, 25, 25));
                item->setToolTip(QStringLiteral("Inconsistente. Sugerencia: %1").arg(suggestion));
            } else {
                item->setToolTip(column == 1 ? track.path : values.at(column));
            }
            if (column == 9 && !track.issues.isEmpty()) {
                item->setBackground(QColor(255, 232, 232));
                item->setForeground(QColor(150, 25, 25));
            }
            ui->trackTable->setItem(row, column, item);
        }
    }
}

QString MainWindow::majorityTableValue(int column, bool ignoreEmpty) const
{
    QHash<QString, int> counts;
    for (int row = 0; row < ui->trackTable->rowCount(); ++row) {
        const QString value = ui->trackTable->item(row, column)->text().trimmed();
        if (!ignoreEmpty || !value.isEmpty()) ++counts[value];
    }
    QString result;
    int best = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        if (it.value() > best) { best = it.value(); result = it.key(); }
    return result;
}

void MainWindow::applySuggestions()
{
    // Solo modifica celdas visibles. itemChanged las marca en azul para que el
    // usuario pueda revisarlas antes de ejecutar el guardado real.
    const QString suggestedAlbum = ui->albumFilterCombo->currentData().toString().isEmpty()
        ? majorityTableValue(4) : ui->albumFilterCombo->currentData().toString();
    const QString suggestedArtist = majorityTableValue(3);
    QString suggestedAlbumArtist = majorityTableValue(5);
    if (suggestedAlbumArtist.isEmpty())
        suggestedAlbumArtist = ui->compilationCheck->isChecked() ? QStringLiteral("Various Artists") : suggestedArtist;
    const QString suggestedGenre = majorityTableValue(6);
    const QString suggestedYear = majorityTableValue(7);
    for (int row = 0; row < ui->trackTable->rowCount(); ++row) {
        for (int column = 0; column <= 7; ++column) {
            QTableWidgetItem *item = ui->trackTable->item(row, column);
            if (!item->data(Qt::UserRole + 1).toBool()) continue;
            QString suggestion;
            if (column == 2 && item->text().trimmed().isEmpty())
                suggestion = QFileInfo(item->data(Qt::UserRole).toString()).completeBaseName();
            else if (column == 3 && item->text().trimmed().isEmpty() && !ui->compilationCheck->isChecked()) suggestion = suggestedArtist;
            else if (column == 4) suggestion = suggestedAlbum;
            else if (column == 5) suggestion = suggestedAlbumArtist;
            else if (column == 6) suggestion = suggestedGenre;
            else if (column == 7) suggestion = suggestedYear;
            if (!suggestion.isEmpty()) item->setText(suggestion);
        }
    }
}

void MainWindow::renumberVisibleRows()
{
    // Tras arrastrar, el índice visual ya no coincide con el lógico del modelo.
    // logicalIndex identifica la fila real que debe recibir el nuevo número.
    const QSignalBlocker blocker(ui->trackTable);
    QHeaderView *header = ui->trackTable->verticalHeader();
    for (int visualRow = 0; visualRow < ui->trackTable->rowCount(); ++visualRow) {
        const int logicalRow = header->logicalIndex(visualRow);
        QTableWidgetItem *item = ui->trackTable->item(logicalRow, 0);
        if (!item) continue;
        item->setText(QString::number(visualRow + 1));
        item->setBackground(QColor(222, 235, 255));
        item->setForeground(QColor(35, 70, 135));
        item->setToolTip(QStringLiteral("Nuevo número de pista pendiente de guardar"));
    }
}

void MainWindow::moveSelectedRows(int direction)
{
    if (direction != -1 && direction != 1) return;
    QHeaderView *header = ui->trackTable->verticalHeader();
    const QModelIndexList selected = ui->trackTable->selectionModel()->selectedRows();
    if (selected.isEmpty()) return;

    QSet<int> logicalRows;
    for (const QModelIndex &index : selected) logicalRows.insert(index.row());
    QList<int> visualRows;
    for (int logicalRow : logicalRows) visualRows.append(header->visualIndex(logicalRow));
    std::sort(visualRows.begin(), visualRows.end());
    if (direction > 0) std::reverse(visualRows.begin(), visualRows.end());

    for (int visualRow : visualRows) {
        const int destination = visualRow + direction;
        if (destination < 0 || destination >= header->count()) continue;
        const int destinationLogical = header->logicalIndex(destination);
        if (logicalRows.contains(destinationLogical)) continue;
        header->moveSection(visualRow, destination);
    }
    if (ui->renumberCheck->isChecked()) renumberVisibleRows();
    statusBar()->showMessage(QStringLiteral("Orden visual modificado"), 3000);
}

void MainWindow::restoreOriginalOrder()
{
    QHeaderView *header = ui->trackTable->verticalHeader();
    // Cada fila lógica conserva el orden con el que se cargó desde disco.
    for (int logicalRow = 0; logicalRow < header->count(); ++logicalRow)
        header->moveSection(header->visualIndex(logicalRow), logicalRow);
    if (ui->renumberCheck->isChecked()) renumberVisibleRows();
    statusBar()->showMessage(QStringLiteral("Orden original restaurado"), 3000);
}

void MainWindow::saveAlbum()
{
    if (ui->trackTable->rowCount() == 0) return;
    // Si se movieron filas, el orden visible se convierte primero en números de
    // pista. applyAlbumFields guarda después esos números junto con el resto de
    // ediciones, utilizando una única copia de seguridad y confirmación.
    if (rowOrderDirty_) renumberVisibleRows();
    saveAlbumToDisk();
}

void MainWindow::applyAlbumFields()
{
    if (ui->trackTable->rowCount() == 0) return;
    const bool compilation = ui->compilationCheck->isChecked();
    const QString album = ui->albumEdit->text().trimmed();
    const QString artist = ui->artistEdit->text().trimmed();
    const QString albumArtist = ui->albumArtistEdit->text().trimmed();
    const QString genre = ui->genreEdit->currentText().trimmed();
    const unsigned int year = static_cast<unsigned int>(ui->yearSpin->value());

    // Aplicar significa preparar la tabla para revisión. La escritura real se
    // reserva exclusivamente al botón Guardar álbum.
    for (int row = 0; row < ui->trackTable->rowCount(); ++row) {
        if (!album.isEmpty()) ui->trackTable->item(row, 4)->setText(album);
        if (!compilation && !artist.isEmpty()) ui->trackTable->item(row, 3)->setText(artist);
        if (!albumArtist.isEmpty()) ui->trackTable->item(row, 5)->setText(albumArtist);
        if (!genre.isEmpty()) ui->trackTable->item(row, 6)->setText(genre);
        if (!compilation && year > 0) ui->trackTable->item(row, 7)->setText(QString::number(year));
    }
    if (saveAlbumButton_) saveAlbumButton_->setEnabled(true);
    statusBar()->showMessage(QStringLiteral("Cambios aplicados a la tabla. Pulsa Guardar álbum para escribirlos."), 6000);
}

void MainWindow::updateSummary()
{
    // El panel derecho diferencia un valor común real de una sugerencia por
    // mayoría. Las sugerencias se colorean, pero no activan commonFieldsDirty_.
    const QSignalBlocker genreBlocker(ui->genreEdit);
    const QList<TrackInfo> visibleTracks = filteredTracks();
    int warnings = 0;
    for (const auto &track : visibleTracks) warnings += track.issues.size();
    ui->trackCountLabel->setText(uiText(QStringLiteral("tracks"), visibleTracks.size()));
    ui->warningCountLabel->setText(warnings == 0 ? uiText(QStringLiteral("noIssues"))
                                                  : uiText(QStringLiteral("warnings"), warnings));
    if (!albumFieldDirty_) ui->albumEdit->setText(commonText(&TrackInfo::album));
    ui->artistEdit->setText(commonText(&TrackInfo::artist));
    ui->albumArtistEdit->setText(commonText(&TrackInfo::albumArtist));
    ui->genreEdit->setCurrentText(commonText(&TrackInfo::genre));
    QHash<unsigned int, int> yearCounts;
    QHash<QString, int> genreCounts;
    QHash<QString, int> albumArtistCounts;
    for (const auto &track : visibleTracks) {
        if (!track.readable) continue;
        if (track.year > 0) ++yearCounts[track.year];
        if (!track.genre.trimmed().isEmpty()) ++genreCounts[track.genre.trimmed()];
        if (!track.albumArtist.trimmed().isEmpty()) ++albumArtistCounts[track.albumArtist.trimmed()];
    }
    auto majorityText = [](const QHash<QString, int> &counts) {
        QString value; int best = 0;
        for (auto it = counts.cbegin(); it != counts.cend(); ++it)
            if (it.value() > best) { best = it.value(); value = it.key(); }
        return value;
    };
    unsigned int majorityYear = 0; int bestYear = 0;
    for (auto it = yearCounts.cbegin(); it != yearCounts.cend(); ++it)
        if (it.value() > bestYear) { bestYear = it.value(); majorityYear = it.key(); }
    const QString majorityGenre = majorityText(genreCounts);
    QString majorityAlbumArtist = majorityText(albumArtistCounts);
    if (majorityAlbumArtist.isEmpty()) majorityAlbumArtist = commonText(&TrackInfo::artist);
    int readableCount = 0;
    for (const auto &track : visibleTracks) if (track.readable) ++readableCount;
    const auto valueCount = [](const auto &counts) {
        int total = 0; for (auto it = counts.cbegin(); it != counts.cend(); ++it) total += it.value(); return total;
    };
    bool detectedCompilation = false;
    for (const auto &track : visibleTracks) detectedCompilation = detectedCompilation || track.compilation;
    const bool compilationMode = ui->compilationCheck->isChecked() || detectedCompilation;
    // En un recopilatorio los años diferentes son válidos. Se preservan por pista
    // y no se propone un año común salvo que el usuario lo escriba expresamente.
    const bool yearNeedsSuggestion = !compilationMode &&
        (yearCounts.size() > 1 || valueCount(yearCounts) < readableCount);
    const bool genreNeedsSuggestion = genreCounts.size() > 1 || valueCount(genreCounts) < readableCount;
    const bool albumArtistNeedsSuggestion = albumArtistCounts.size() > 1 || valueCount(albumArtistCounts) < readableCount;

    // Un recopilatorio no tiene un año común sugerido. El cero se presenta como
    // campo vacío y, al guardar, impide sobrescribir los años de las canciones.
    ui->yearSpin->setValue(compilationMode ? 0 : static_cast<int>(majorityYear));
    if (ui->genreEdit->currentText().isEmpty()) ui->genreEdit->setCurrentText(majorityGenre);
    if (ui->albumArtistEdit->text().isEmpty()) ui->albumArtistEdit->setText(majorityAlbumArtist);
    const QString suggestionStyle = QStringLiteral("background: #ffe2e2; border: 1px solid #dd6b6b; color: #8f2020;");
    ui->yearSpin->setStyleSheet(yearNeedsSuggestion ? suggestionStyle : QString{});
    ui->genreEdit->setStyleSheet(genreNeedsSuggestion ? suggestionStyle : QString{});
    ui->albumArtistEdit->setStyleSheet(albumArtistNeedsSuggestion ? suggestionStyle : QString{});
    ui->yearSpin->setToolTip(compilationMode
        ? QStringLiteral("Sin año común: se conservan los años individuales de cada canción")
        : (yearNeedsSuggestion ? QStringLiteral("Sugerencia: año mayoritario del álbum") : QString{}));
    ui->genreEdit->setToolTip(genreNeedsSuggestion ? QStringLiteral("Sugerencia: género mayoritario del álbum") : QString{});
    ui->albumArtistEdit->setToolTip(albumArtistCounts.isEmpty() ? QStringLiteral("Sugerencia basada en el artista de las pistas") : QString{});
    bool compilation = false;
    QSet<QString> artists;
    for (const auto &track : visibleTracks) {
        compilation = compilation || track.compilation;
        if (track.readable && !track.artist.trimmed().isEmpty())
            artists.insert(track.artist.trimmed().toCaseFolded());
    }
    if (!commonFieldsDirty_) ui->compilationCheck->setChecked(compilation);
    ui->multiArtistHint->setVisible(artists.size() > 1);
    const bool multipleAlbumsView = ui->albumFilterCombo->currentData().toString().isEmpty() &&
                                    ui->albumFilterCombo->count() > 1;
    ui->albumEdit->setPlaceholderText(multipleAlbumsView
        ? QStringLiteral("Escribe aquí el nombre correcto para unificar todas las pistas") : QString{});
    const bool explicitAlbumUnification = multipleAlbumsView && albumFieldDirty_ &&
                                          !ui->albumEdit->text().trimmed().isEmpty();
    ui->applyButton->setEnabled(!visibleTracks.isEmpty() && (!multipleAlbumsView || explicitAlbumUnification));
    ui->applyButton->setToolTip(multipleAlbumsView
        ? (explicitAlbumUnification
            ? QStringLiteral("Aplicar este nombre de álbum a todas las pistas visibles")
            : QStringLiteral("Escribe el nombre correcto del álbum para habilitar la corrección"))
        : QString{});
}

void MainWindow::saveAlbumToDisk()
{
    // Barreras de seguridad: álbum concreto, confirmación y backup JSON. Si no se
    // puede crear la copia, el proceso termina sin escribir ningún tag.
    const QList<TrackInfo> targetTracks = filteredTracks();
    if (targetTracks.isEmpty()) return;
    const bool multipleAlbumsView = ui->albumFilterCombo->currentData().toString().isEmpty() &&
                                    ui->albumFilterCombo->count() > 1;
    const bool unifyAlbumOnly = multipleAlbumsView && albumFieldDirty_ &&
                                !ui->albumEdit->text().trimmed().isEmpty();
    const auto answer = QMessageBox::question(this, QStringLiteral("Guardar etiquetas"),
        unifyAlbumOnly
            ? QStringLiteral("Se aplicará el nombre de álbum «%1» a %2 archivos. Los demás campos se conservarán. ¿Continuar?")
                  .arg(ui->albumEdit->text().trimmed()).arg(targetTracks.size())
            : QStringLiteral("Se guardarán el orden, las ediciones de las pistas y los datos comunes en %1 archivos. ¿Continuar?")
                  .arg(targetTracks.size()));
    if (answer != QMessageBox::Yes) return;

    AlbumFields fields{ui->albumEdit->text().trimmed(),
                       ui->compilationCheck->isChecked() ? QString{} : ui->artistEdit->text().trimmed(),
                       ui->albumArtistEdit->text().trimmed(), ui->genreEdit->currentText().trimmed(),
                       static_cast<unsigned int>(ui->yearSpin->value()), ui->compilationCheck->isChecked()};
    QString backupPath;
    QString backupError;
    if (!TagService::createBackup(folder_, targetTracks, &backupPath, &backupError)) {
        QMessageBox::critical(this, QStringLiteral("No se guardaron cambios"),
                              backupError + QStringLiteral("\n\nLa operación se ha cancelado por seguridad."));
        return;
    }
    QStringList errors;
    setBusy(true, QStringLiteral("Guardando etiquetas…"));
    // Se reconstruye cada TrackInfo desde la tabla para respetar ediciones por
    // pista. Los campos comunes prevalecen solo si el usuario los ha tocado.
    for (int row = 0; row < ui->trackTable->rowCount(); ++row) {
        const QString path = ui->trackTable->item(row, 0)->data(Qt::UserRole).toString();
        auto originalIt = std::find_if(targetTracks.cbegin(), targetTracks.cend(), [&path](const TrackInfo &track) { return track.path == path; });
        if (originalIt == targetTracks.cend()) continue;
        TrackInfo edited = *originalIt;
        edited.track = ui->trackTable->item(row, 0)->text().toUInt();
        edited.title = ui->trackTable->item(row, 2)->text().trimmed();
        edited.artist = ui->trackTable->item(row, 3)->text().trimmed();
        edited.album = ui->trackTable->item(row, 4)->text().trimmed();
        edited.albumArtist = ui->trackTable->item(row, 5)->text().trimmed();
        edited.genre = ui->trackTable->item(row, 6)->text().trimmed();
        edited.year = ui->trackTable->item(row, 7)->text().toUInt();
        if (unifyAlbumOnly) {
            edited.album = fields.album;
        } else if (commonFieldsDirty_) {
            if (!fields.album.isEmpty()) edited.album = fields.album;
            if (!fields.artist.isEmpty() && !fields.compilation) edited.artist = fields.artist;
            if (!fields.albumArtist.isEmpty()) edited.albumArtist = fields.albumArtist;
            if (!fields.genre.isEmpty()) edited.genre = fields.genre;
            if (fields.year > 0) edited.year = fields.year;
        }
        QString error;
        AlbumFields writeFields = fields;
        if (unifyAlbumOnly || !commonFieldsDirty_)
            writeFields.compilation = originalIt->compilation;
        if (!TagService::writeTrackFields(*originalIt, edited, writeFields,
                                          commonFieldsDirty_ && !unifyAlbumOnly, &error)) errors << error;
    }
    scan(folder_);
    if (errors.isEmpty()) QMessageBox::information(this, QStringLiteral("AlbumTagger"),
        QStringLiteral("Etiquetas guardadas correctamente.\n\nCopia de seguridad:\n%1").arg(QDir::toNativeSeparators(backupPath)));
    else QMessageBox::warning(this, QStringLiteral("Algunos archivos fallaron"), errors.join(QLatin1Char('\n')));
}

void MainWindow::chooseLocalCover()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Seleccionar portada"), {},
                                                       QStringLiteral("Imágenes (*.jpg *.jpeg *.png)"));
    if (path.isEmpty()) return;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) setCoverImage(file.readAll(), QFileInfo(path).suffix());
}

void MainWindow::downloadCover()
{
    const QUrl url = QUrl::fromUserInput(ui->coverUrlEdit->text().trimmed());
    if (!url.isValid() || (url.scheme() != QStringLiteral("http") && url.scheme() != QStringLiteral("https"))) {
        QMessageBox::warning(this, QStringLiteral("URL no válida"), QStringLiteral("Introduce una dirección HTTP o HTTPS válida."));
        return;
    }
    fetchCoverUrl(url);
}

void MainWindow::fetchCoverUrl(const QUrl &url)
{
    // La red no bloquea el panel central: únicamente se deshabilitan los botones
    // relacionados mientras la petición asíncrona está activa.
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("AlbumTagger/" ALBUMTAGGER_VERSION
                                     " (https://github.com/Tomas-Fernandez-Galera)"));
    ui->downloadCoverButton->setEnabled(false);
    ui->musicBrainzButton->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("Descargando portada…"));
    QNetworkReply *reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url] {
        reply->deleteLater();
        ui->downloadCoverButton->setEnabled(true);
        ui->musicBrainzButton->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            QString fallback = url.toString();
            // Cover Art Archive no siempre tiene todas las miniaturas. La cadena
            // de alternativas es 1200 px -> 500 px -> imagen original.
            if (fallback.endsWith(QStringLiteral("-1200"))) {
                fallback.chop(5);
                fetchCoverUrl(QUrl(fallback + QStringLiteral("-500")));
                return;
            }
            if (fallback.endsWith(QStringLiteral("-500"))) {
                fallback.chop(4);
                fetchCoverUrl(QUrl(fallback));
                return;
            }
            QMessageBox::warning(this, QStringLiteral("Error de descarga"), reply->errorString());
            return;
        }
        setCoverImage(reply->readAll(), QFileInfo(url.path()).suffix());
    });
}

void MainWindow::searchMusicBrainz()
{
    const QString album = ui->albumEdit->text().trimmed();
    const QString artist = ui->albumArtistEdit->text().trimmed();
    if (album.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Falta el álbum"),
                                 QStringLiteral("Escribe o carga el nombre del álbum antes de buscar."));
        return;
    }
    QUrl url(QStringLiteral("https://musicbrainz.org/ws/2/release/"));
    QUrlQuery query;
    QString expression = QStringLiteral("release:\"%1\"").arg(album);
    if (!artist.isEmpty()) expression += QStringLiteral(" AND artist:\"%1\"").arg(artist);
    query.addQueryItem(QStringLiteral("query"), expression);
    query.addQueryItem(QStringLiteral("fmt"), QStringLiteral("json"));
    query.addQueryItem(QStringLiteral("limit"), QStringLiteral("10"));
    url.setQuery(query);
    requestMusicBrainz(url, 0);
}

void MainWindow::requestMusicBrainz(const QUrl &url, int attempt)
{
    // MusicBrainz requiere User-Agent identificable y limita la frecuencia. Solo
    // 429/503 activan reintentos, con espera progresiva y máximo definido.
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("AlbumTagger/" ALBUMTAGGER_VERSION
                                     " (https://github.com/Tomas-Fernandez-Galera)"));
    request.setRawHeader("Accept", "application/json");
    ui->musicBrainzButton->setEnabled(false);
    statusBar()->showMessage(attempt == 0 ? QStringLiteral("Buscando en MusicBrainz…")
                                          : QStringLiteral("MusicBrainz está ocupado. Reintentando…"));
    QNetworkReply *reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, url, attempt] {
        reply->deleteLater();
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if ((status == 503 || status == 429) && attempt < 3) {
            const int delayMs = 1200 * (attempt + 1);
            statusBar()->showMessage(QStringLiteral("MusicBrainz no está disponible. Nuevo intento en %1 segundos…")
                                     .arg(QString::number(delayMs / 1000.0, 'f', 1)));
            QTimer::singleShot(delayMs, this, [this, url, attempt] { requestMusicBrainz(url, attempt + 1); });
            return;
        }
        ui->musicBrainzButton->setEnabled(true);
        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::warning(this, QStringLiteral("MusicBrainz no responde"),
                status == 503
                    ? QStringLiteral("MusicBrainz continúa temporalmente ocupado después de varios intentos. Espera un minuto y vuelve a probar.")
                    : QStringLiteral("No se pudo consultar MusicBrainz. Comprueba la conexión a Internet y vuelve a intentarlo.\n\n%1")
                          .arg(reply->errorString()));
            return;
        }
        const QJsonArray releases = QJsonDocument::fromJson(reply->readAll()).object().value(QStringLiteral("releases")).toArray();
        QStringList labels;
        QStringList ids;
        for (const auto &value : releases) {
            const QJsonObject release = value.toObject();
            const QString title = release.value(QStringLiteral("title")).toString();
            const QString date = release.value(QStringLiteral("date")).toString();
            const QString country = release.value(QStringLiteral("country")).toString();
            const QString status = release.value(QStringLiteral("status")).toString();
            labels << QStringLiteral("%1  ·  %2  ·  %3  ·  %4").arg(title, date, country, status);
            ids << release.value(QStringLiteral("id")).toString();
        }
        if (labels.isEmpty()) {
            QMessageBox::information(this, QStringLiteral("MusicBrainz"), QStringLiteral("No se encontraron ediciones coincidentes."));
            return;
        }
        bool accepted = false;
        const QString selected = QInputDialog::getItem(this, QStringLiteral("Elegir edición"),
            QStringLiteral("Resultados de MusicBrainz:"), labels, 0, false, &accepted);
        if (!accepted) return;
        const int index = labels.indexOf(selected);
        if (index >= 0)
            fetchCoverUrl(QUrl(QStringLiteral("https://coverartarchive.org/release/%1/front-1200").arg(ids.at(index))));
    });
}

void MainWindow::setCoverImage(const QByteArray &data, const QString &sourceName)
{
    // QImage valida los bytes antes de conservarlos. La extensión de salida se
    // reduce a PNG o JPG, los dos formatos ofrecidos por AlbumTagger.
    QImage image;
    if (!image.loadFromData(data)) {
        QMessageBox::warning(this, QStringLiteral("Imagen no válida"), QStringLiteral("El archivo descargado no es una imagen compatible."));
        return;
    }
    coverData_ = data;
    coverExtension_ = sourceName.toLower() == QStringLiteral("png") ? QStringLiteral("png") : QStringLiteral("jpg");
    ui->coverPreview->setPixmap(QPixmap::fromImage(image).scaled(ui->coverPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->coverInfoLabel->setText(QStringLiteral("%1 × %2 px · %3").arg(image.width()).arg(image.height()).arg(coverExtension_.toUpper()));
    ui->saveCoverButton->setEnabled(!folder_.isEmpty());
}

void MainWindow::saveCoverAs()
{
    // Nunca se incrusta la imagen en el audio. Si folder.* o cover.* ya existe,
    // el usuario debe confirmar expresamente que desea reemplazarlo.
    if (coverData_.isEmpty() || folder_.isEmpty()) return;
    const QString baseName = ui->coverNameCombo->currentText();
    const QString path = QDir(folder_).filePath(baseName + QLatin1Char('.') + coverExtension_);
    if (QFile::exists(path) && QMessageBox::question(this, QStringLiteral("Reemplazar portada"),
        QStringLiteral("%1 ya existe. ¿Quieres reemplazarlo?").arg(QFileInfo(path).fileName())) != QMessageBox::Yes) return;
    QFile output(path);
    if (!output.open(QIODevice::WriteOnly) || output.write(coverData_) != coverData_.size()) {
        QMessageBox::warning(this, QStringLiteral("No se pudo guardar"), output.errorString());
        return;
    }
    statusBar()->showMessage(QStringLiteral("Portada guardada como %1").arg(QFileInfo(path).fileName()), 5000);
}

void MainWindow::setBusy(bool busy, const QString &message)
{
    ui->centralwidget->setEnabled(!busy);
    if (!message.isEmpty()) statusBar()->showMessage(message);
    QApplication::processEvents();
}
