#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QHash>
#include <QIcon>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolButton>

/**
 * Catálogo y aplicación de los idiomas de la interfaz.
 *
 * Este archivo concentra deliberadamente todos los textos traducidos para que
 * mainwindow.cpp conserve una secuencia legible de la lógica funcional. Los
 * idiomas afectan solamente a los widgets; nunca a las etiquetas musicales.
 */

void MainWindow::setupLanguages()
{
    // itemData conserva un código estable; el texto se muestra en la lengua
    // correspondiente y el icono procede de los SVG incluidos en el ejecutable.
    struct Language { const char *code; const char *label; };
    const Language languages[] = {
        {"es", "Español"}, {"en", "English"}, {"fr", "Français"},
        {"de", "Deutsch"}, {"it", "Italiano"}, {"pt", "Português"},
        {"nl", "Nederlands"}, {"pl", "Polski"}, {"ru", "Русский"},
        {"zh_CN", "简体中文"}, {"ja", "日本語"}, {"ko", "한국어"},
        {"ca", "Català"}, {"eu", "Euskara"}, {"gl", "Galego"}
    };
    for (const auto &language : languages) {
        const QString code = QString::fromLatin1(language.code);
        ui->languageCombo->addItem(QIcon(QStringLiteral(":/flags/%1.svg").arg(code)),
                                   QString::fromUtf8(language.label), code);
    }

    QSettings settings;
    QString selectedCode;
    // Una selección manual guardada tiene prioridad. Solo el primer arranque usa
    // la configuración regional de Windows y recurre a inglés si no está soportada.
    if (settings.contains(QStringLiteral("interface/language"))) {
        selectedCode = settings.value(QStringLiteral("interface/language")).toString();
    } else {
        const QString systemLocale = QLocale::system().name();
        selectedCode = systemLocale.startsWith(QStringLiteral("zh"), Qt::CaseInsensitive)
            ? QStringLiteral("zh_CN") : systemLocale.left(2).toLower();
    }
    int selectedIndex = ui->languageCombo->findData(selectedCode);
    if (selectedIndex < 0) selectedIndex = ui->languageCombo->findData(QStringLiteral("en"));
    ui->languageCombo->setCurrentIndex(selectedIndex);
    applyLanguage(ui->languageCombo->currentData().toString());
    ui->languageCombo->setToolTip(QStringLiteral("Idioma de la interfaz · Por defecto se detecta desde Windows"));
    connect(ui->languageCombo, &QComboBox::activated, this, [this](int index) {
        const QString code = ui->languageCombo->itemData(index).toString();
        QSettings().setValue(QStringLiteral("interface/language"), code);
        applyLanguage(code);
        statusBar()->showMessage(ui->languageCombo->currentText(), 3000);
    });
}

void MainWindow::applyLanguage(const QString &code)
{
    // La traducción se aplica en caliente y afecta únicamente a la interfaz;
    // nunca escribe un campo de idioma en MP3, FLAC u otros formatos.
    static const QHash<QString, QStringList> t = {
        {"es", {"Álbumes limpios, coherentes y sin duplicados fantasma","Abrir carpeta de álbum","Volver a analizar","Álbum detectado:","Renumerar según el orden visible","Datos comunes del álbum","Álbum","Artista pistas","Artista álbum","Álbum de varios artistas / recopilatorio","Año","Género","Aplicar sugerencias en rojo","Aplicar cambios","Portada","Elegir imagen…","Buscar en MusicBrainz","Descargar desde URL","Guardar portada en el álbum","Archivo","Título","Artista","Duración","Anomalías","Selecciona la carpeta que contiene el álbum…","Pegar URL de una imagen…"}},
        {"en", {"Clean, consistent albums without ghost duplicates","Open folder","Scan again","Detected album:","Renumber by visible order","Common album data","Album","Track artist","Album artist","Various-artists album / compilation","Year","Genre","Apply red suggestions","Apply changes","Cover art","Choose image…","Search MusicBrainz","Download from URL","Save cover in album","File","Title","Artist","Duration","Issues","Select the folder containing the album…","Paste an image URL…"}},
        {"fr", {"Albums propres et cohérents, sans doublons fantômes","Ouvrir le dossier","Analyser à nouveau","Album détecté :","Renuméroter selon l’ordre visible","Données communes de l’album","Album","Artiste de la piste","Artiste de l’album","Compilation / artistes divers","Année","Genre","Appliquer les suggestions rouges","Appliquer les modifications","Pochette","Choisir une image…","Rechercher sur MusicBrainz","Télécharger depuis l’URL","Enregistrer la pochette","Fichier","Titre","Artiste","Durée","Anomalies","Sélectionnez le dossier de l’album…","Coller l’URL d’une image…"}},
        {"de", {"Saubere, einheitliche Alben ohne Geisterduplikate","Ordner öffnen","Erneut prüfen","Erkanntes Album:","Nach sichtbarer Reihenfolge nummerieren","Gemeinsame Albumdaten","Album","Titelinterpret","Albuminterpret","Sampler / verschiedene Interpreten","Jahr","Genre","Rote Vorschläge anwenden","Änderungen anwenden","Cover","Bild auswählen…","MusicBrainz durchsuchen","Von URL laden","Cover im Album speichern","Datei","Titel","Interpret","Dauer","Probleme","Albumordner auswählen…","Bild-URL einfügen…"}},
        {"it", {"Album puliti e coerenti, senza duplicati fantasma","Apri cartella","Analizza di nuovo","Album rilevato:","Rinumera secondo l’ordine visibile","Dati comuni dell’album","Album","Artista traccia","Artista album","Raccolta / artisti vari","Anno","Genere","Applica suggerimenti rossi","Applica modifiche","Copertina","Scegli immagine…","Cerca su MusicBrainz","Scarica da URL","Salva copertina nell’album","File","Titolo","Artista","Durata","Anomalie","Seleziona la cartella dell’album…","Incolla URL immagine…"}},
        {"pt", {"Álbuns limpos e coerentes, sem duplicados fantasma","Abrir pasta","Analisar novamente","Álbum detetado:","Renumerar pela ordem visível","Dados comuns do álbum","Álbum","Artista da faixa","Artista do álbum","Compilação / vários artistas","Ano","Género","Aplicar sugestões a vermelho","Aplicar alterações","Capa","Escolher imagem…","Pesquisar no MusicBrainz","Transferir do URL","Guardar capa no álbum","Ficheiro","Título","Artista","Duração","Anomalias","Selecione a pasta do álbum…","Cole o URL de uma imagem…"}},
        {"nl", {"Schone, consistente albums zonder spookduplicaten","Map openen","Opnieuw scannen","Gedetecteerd album:","Nummeren volgens zichtbare volgorde","Gemeenschappelijke albumgegevens","Album","Trackartiest","Albumartiest","Verzamelalbum / diverse artiesten","Jaar","Genre","Rode suggesties toepassen","Wijzigingen toepassen","Hoes","Afbeelding kiezen…","Zoeken in MusicBrainz","Downloaden via URL","Hoes in album opslaan","Bestand","Titel","Artiest","Duur","Problemen","Selecteer de albummap…","Plak een afbeeldings-URL…"}},
        {"pl", {"Spójne albumy bez widmowych duplikatów","Otwórz folder","Skanuj ponownie","Wykryty album:","Numeruj według widocznej kolejności","Wspólne dane albumu","Album","Wykonawca utworu","Wykonawca albumu","Składanka / różni wykonawcy","Rok","Gatunek","Zastosuj czerwone sugestie","Zastosuj zmiany","Okładka","Wybierz obraz…","Szukaj w MusicBrainz","Pobierz z URL","Zapisz okładkę w albumie","Plik","Tytuł","Wykonawca","Czas","Problemy","Wybierz folder albumu…","Wklej URL obrazu…"}},
        {"ru", {"Чистые и единообразные альбомы без дубликатов","Открыть папку","Сканировать снова","Найденный альбом:","Нумеровать по видимому порядку","Общие данные альбома","Альбом","Исполнитель трека","Исполнитель альбома","Сборник / разные исполнители","Год","Жанр","Применить красные подсказки","Применить изменения","Обложка","Выбрать изображение…","Поиск в MusicBrainz","Скачать по URL","Сохранить обложку","Файл","Название","Исполнитель","Длительность","Проблемы","Выберите папку альбома…","Вставьте URL изображения…"}},
        {"zh_CN", {"整洁一致的专辑，避免幽灵重复","打开文件夹","重新扫描","检测到的专辑：","按显示顺序重新编号","专辑公共信息","专辑","曲目艺人","专辑艺人","群星合辑","年份","流派","应用红色建议","应用更改","封面","选择图片…","搜索 MusicBrainz","从网址下载","保存封面到专辑","文件","标题","艺人","时长","问题","选择专辑所在文件夹…","粘贴图片网址…"}},
        {"ja", {"重複表示のない、整ったアルバム","フォルダーを開く","再スキャン","検出したアルバム：","表示順に番号を付け直す","アルバム共通情報","アルバム","トラックアーティスト","アルバムアーティスト","複数アーティスト／コンピレーション","年","ジャンル","赤い候補を適用","変更を適用","カバーアート","画像を選択…","MusicBrainzで検索","URLからダウンロード","カバーを保存","ファイル","タイトル","アーティスト","時間","問題","アルバムのフォルダーを選択…","画像URLを貼り付け…"}},
        {"ko", {"중복 표시 없는 깔끔하고 일관된 앨범","폴더 열기","다시 검색","감지된 앨범:","표시 순서대로 번호 다시 매기기","앨범 공통 정보","앨범","트랙 아티스트","앨범 아티스트","여러 아티스트 / 컴필레이션","연도","장르","빨간색 제안 적용","변경 사항 적용","표지","이미지 선택…","MusicBrainz 검색","URL에서 다운로드","앨범에 표지 저장","파일","제목","아티스트","길이","문제","앨범 폴더를 선택하세요…","이미지 URL 붙여넣기…"}}
        ,{"ca", {"Àlbums nets i coherents, sense duplicats fantasma","Obre la carpeta de l’àlbum","Torna a analitzar","Àlbum detectat:","Renumera segons l’ordre visible","Dades comunes de l’àlbum","Àlbum","Artista de la pista","Artista de l’àlbum","Àlbum de diversos artistes / recopilatori","Any","Gènere","Aplica els suggeriments en vermell","Aplica els canvis","Caràtula","Tria una imatge…","Cerca a MusicBrainz","Baixa des d’un URL","Desa la caràtula a l’àlbum","Fitxer","Títol","Artista","Durada","Anomalies","Selecciona la carpeta que conté l’àlbum…","Enganxa l’URL d’una imatge…"}}
        ,{"eu", {"Album garbi eta koherenteak, bikoiztu mamurik gabe","Ireki albumaren karpeta","Aztertu berriro","Detektatutako albuma:","Berriz zenbakitu ikusgai dagoen ordenaren arabera","Albumaren datu komunak","Albuma","Pistaren artista","Albumaren artista","Hainbat artistaren albuma / bilduma","Urtea","Generoa","Aplikatu gorrizko iradokizunak","Aplikatu aldaketak","Azala","Aukeratu irudia…","Bilatu MusicBrainz-en","Deskargatu URLtik","Gorde azala albumean","Fitxategia","Izenburua","Artista","Iraupena","Arazoak","Hautatu albuma duen karpeta…","Itsatsi irudiaren URLa…"}}
        ,{"gl", {"Álbums limpos e coherentes, sen duplicados pantasma","Abrir o cartafol do álbum","Analizar de novo","Álbum detectado:","Renumerar segundo a orde visible","Datos comúns do álbum","Álbum","Artista da pista","Artista do álbum","Álbum de varios artistas / recompilatorio","Ano","Xénero","Aplicar as suxestións en vermello","Aplicar os cambios","Portada","Escoller imaxe…","Buscar en MusicBrainz","Descargar desde URL","Gardar a portada no álbum","Ficheiro","Título","Artista","Duración","Anomalías","Selecciona o cartafol que contén o álbum…","Pega o URL dunha imaxe…"}}
    };
    languageCode_ = t.contains(code) ? code : QStringLiteral("en");
    const QStringList &s = t[languageCode_];
    static const QHash<QString, QStringList> albumActions = {
        {"es", {"Crear álbum", "Añadir archivos"}}, {"en", {"Create album", "Add files"}},
        {"fr", {"Créer un album", "Ajouter des fichiers"}}, {"de", {"Album erstellen", "Dateien hinzufügen"}},
        {"it", {"Crea album", "Aggiungi file"}}, {"pt", {"Criar álbum", "Adicionar ficheiros"}},
        {"nl", {"Album maken", "Bestanden toevoegen"}}, {"pl", {"Utwórz album", "Dodaj pliki"}},
        {"ru", {"Создать альбом", "Добавить файлы"}}, {"zh_CN", {"创建专辑", "添加文件"}},
        {"ja", {"アルバムを作成", "ファイルを追加"}}, {"ko", {"앨범 만들기", "파일 추가"}},
        {"ca", {"Crea un àlbum", "Afegeix fitxers"}}, {"eu", {"Sortu albuma", "Gehitu fitxategiak"}},
        {"gl", {"Crear álbum", "Engadir ficheiros"}}
    };
    const QStringList actions = albumActions.value(languageCode_, albumActions.value(QStringLiteral("en")));
    ui->createAlbumButton->setText(actions[0]); ui->addFilesButton->setText(actions[1]);
    static const QHash<QString, QStringList> orderActions = {
        {"es", {"Guardar álbum", "Subir canciones", "Bajar canciones", "Restaurar el orden original"}},
        {"en", {"Save album", "Move tracks up", "Move tracks down", "Restore original order"}},
        {"fr", {"Enregistrer l’album", "Monter les pistes", "Descendre les pistes", "Rétablir l’ordre initial"}},
        {"de", {"Album speichern", "Titel nach oben", "Titel nach unten", "Ursprüngliche Reihenfolge"}},
        {"it", {"Salva album", "Sposta tracce su", "Sposta tracce giù", "Ripristina ordine originale"}},
        {"pt", {"Guardar álbum", "Subir faixas", "Descer faixas", "Repor ordem original"}},
        {"nl", {"Album opslaan", "Tracks omhoog", "Tracks omlaag", "Oorspronkelijke volgorde"}},
        {"pl", {"Zapisz album", "Przenieś utwory w górę", "Przenieś utwory w dół", "Przywróć kolejność"}},
        {"ru", {"Сохранить альбом", "Переместить треки вверх", "Переместить треки вниз", "Восстановить порядок"}},
        {"zh_CN", {"保存专辑", "上移曲目", "下移曲目", "恢复原始顺序"}},
        {"ja", {"アルバムを保存", "トラックを上へ", "トラックを下へ", "元の順序に戻す"}},
        {"ko", {"앨범 저장", "트랙 위로", "트랙 아래로", "원래 순서 복원"}},
        {"ca", {"Desa l’àlbum", "Puja les pistes", "Baixa les pistes", "Restaura l’ordre original"}},
        {"eu", {"Gorde albuma", "Igo pistak", "Jaitsi pistak", "Leheneratu jatorrizko ordena"}},
        {"gl", {"Gardar álbum", "Subir pistas", "Baixar pistas", "Restaurar a orde orixinal"}}
    };
    const QStringList order = orderActions.value(languageCode_, orderActions.value(QStringLiteral("en")));
    if (saveAlbumButton_) {
        saveAlbumButton_->setText(order[0]);
        saveAlbumButton_->setToolTip(order[0]);
    }
    if (auto *button = findChild<QToolButton *>(QStringLiteral("moveTracksUpButton"))) button->setToolTip(order[1]);
    if (auto *button = findChild<QToolButton *>(QStringLiteral("moveTracksDownButton"))) button->setToolTip(order[2]);
    if (auto *button = findChild<QToolButton *>(QStringLiteral("restoreTrackOrderButton"))) button->setToolTip(order[3]);
    int warnings = 0;
    for (const auto &track : filteredTracks()) warnings += track.issues.size();
    ui->trackCountLabel->setText(uiText(QStringLiteral("tracks"), filteredTracks().size()));
    ui->warningCountLabel->setText(tracks_.isEmpty()
        ? uiText(QStringLiteral("notAnalyzed"))
        : (warnings == 0 ? uiText(QStringLiteral("noIssues")) : uiText(QStringLiteral("warnings"), warnings)));
    if (coverData_.isEmpty()) ui->coverPreview->setText(uiText(QStringLiteral("coverPreview")));
    ui->openFolderButton->setText(s[1]); ui->rescanButton->setText(s[2]);
    ui->albumFilterLabel->setText(s[3]); ui->renumberCheck->setText(s[4]); ui->albumHeader->setText(s[5]);
    ui->albumLabel->setText(s[6]); ui->artistLabel->setText(s[7]); ui->albumArtistLabel->setText(s[8]);
    ui->compilationCheck->setText(s[9]); ui->yearLabel->setText(s[10]); ui->genreLabel->setText(s[11]);
    ui->suggestButton->setText(s[12]); ui->applyButton->setText(s[13]); ui->coverHeader->setText(s[14]);
    ui->localCoverButton->setText(s[15]); ui->musicBrainzButton->setText(s[16]); ui->downloadCoverButton->setText(s[17]);
    ui->saveCoverButton->setText(s[18]);
    const QStringList headers = {QStringLiteral("#"), s[19], s[20], s[21], s[6], s[8], s[11], s[10], s[22], s[23]};
    ui->trackTable->setHorizontalHeaderLabels(headers);
    ui->folderPath->setPlaceholderText(s[24]); ui->coverUrlEdit->setPlaceholderText(s[25]);
    updateArtistPlaceholder(ui->compilationCheck->isChecked());
}

QString MainWindow::uiText(const QString &key, int number) const
{
    // Textos breves que cambian durante la ejecución y no pueden limitarse a
    // las etiquetas estáticas aplicadas por applyLanguage.
    static const QHash<QString, QStringList> texts = {
        {"es", {"%1 pistas", "Sin analizar", "Sin anomalías", "%1 avisos", "Vista previa de la portada", "No se encontró folder.* ni cover.*"}},
        {"en", {"%1 tracks", "Not analyzed", "No issues", "%1 warnings", "Cover preview", "No folder.* or cover.* found"}},
        {"fr", {"%1 pistes", "Non analysé", "Aucune anomalie", "%1 avertissements", "Aperçu de la pochette", "Aucun folder.* ou cover.* trouvé"}},
        {"de", {"%1 Titel", "Nicht analysiert", "Keine Probleme", "%1 Warnungen", "Covervorschau", "Kein folder.* oder cover.* gefunden"}},
        {"it", {"%1 tracce", "Non analizzato", "Nessuna anomalia", "%1 avvisi", "Anteprima copertina", "Nessun folder.* o cover.* trovato"}},
        {"pt", {"%1 faixas", "Não analisado", "Sem anomalias", "%1 avisos", "Pré-visualização da capa", "Nenhum folder.* ou cover.* encontrado"}},
        {"nl", {"%1 tracks", "Niet geanalyseerd", "Geen problemen", "%1 waarschuwingen", "Hoesvoorbeeld", "Geen folder.* of cover.* gevonden"}},
        {"pl", {"%1 utworów", "Nie przeanalizowano", "Brak problemów", "%1 ostrzeżeń", "Podgląd okładki", "Nie znaleziono folder.* ani cover.*"}},
        {"ru", {"%1 треков", "Не проанализировано", "Нет проблем", "%1 предупреждений", "Предпросмотр обложки", "folder.* или cover.* не найдены"}},
        {"zh_CN", {"%1 首曲目", "未分析", "无问题", "%1 个警告", "封面预览", "未找到 folder.* 或 cover.*"}},
        {"ja", {"%1 トラック", "未解析", "問題なし", "%1 件の警告", "カバーのプレビュー", "folder.* または cover.* が見つかりません"}},
        {"ko", {"%1개 트랙", "분석되지 않음", "문제 없음", "%1개 경고", "표지 미리보기", "folder.* 또는 cover.*를 찾을 수 없음"}},
        {"ca", {"%1 pistes", "Sense analitzar", "Sense anomalies", "%1 avisos", "Previsualització de la caràtula", "No s’ha trobat folder.* ni cover.*"}},
        {"eu", {"%1 pista", "Aztertu gabe", "Arazorik ez", "%1 abisu", "Azalaren aurrebista", "Ez da folder.* edo cover.* aurkitu"}},
        {"gl", {"%1 pistas", "Sen analizar", "Sen anomalías", "%1 avisos", "Vista previa da portada", "Non se atopou folder.* nin cover.*"}}
    };
    static const QHash<QString, int> indexes = {
        {"tracks", 0}, {"notAnalyzed", 1}, {"noIssues", 2}, {"warnings", 3},
        {"coverPreview", 4}, {"noCover", 5}
    };
    const QStringList values = texts.value(languageCode_, texts.value(QStringLiteral("en")));
    const int index = indexes.value(key, 0);
    return number >= 0 ? values[index].arg(number) : values[index];
}

void MainWindow::updateArtistPlaceholder(bool compilation)
{
    static const QHash<QString, QStringList> placeholders = {
        {"es", {"Artista común de las pistas","Se conservan los artistas de cada pista"}},
        {"en", {"Artist shared by all tracks","Each track artist will be preserved"}},
        {"fr", {"Artiste commun aux pistes","Les artistes de chaque piste seront conservés"}},
        {"de", {"Gemeinsamer Interpret der Titel","Die Interpreten der einzelnen Titel bleiben erhalten"}},
        {"it", {"Artista comune delle tracce","Gli artisti delle singole tracce saranno conservati"}},
        {"pt", {"Artista comum das faixas","Os artistas de cada faixa serão mantidos"}},
        {"nl", {"Gemeenschappelijke artiest van de tracks","De artiest per track blijft behouden"}},
        {"pl", {"Wspólny wykonawca utworów","Wykonawcy poszczególnych utworów zostaną zachowani"}},
        {"ru", {"Общий исполнитель треков","Исполнитель каждого трека будет сохранён"}},
        {"zh_CN", {"所有曲目的共同艺人","将保留每首曲目的艺人"}},
        {"ja", {"全トラック共通のアーティスト","各トラックのアーティストを保持します"}},
        {"ko", {"모든 트랙의 공통 아티스트","각 트랙의 아티스트를 유지합니다"}},
        {"ca", {"Artista comú de les pistes","Es conservaran els artistes de cada pista"}},
        {"eu", {"Pisten artista komuna","Pista bakoitzaren artista mantenduko da"}},
        {"gl", {"Artista común das pistas","Conservaranse os artistas de cada pista"}}
    };
    const QStringList text = placeholders.value(languageCode_, placeholders.value(QStringLiteral("en")));
    ui->artistEdit->setPlaceholderText(text[compilation ? 1 : 0]);
}

void MainWindow::showAbout()
{
    // El resumen legal se traduce para que la licencia y el carácter no oficial
    // de las modificaciones sean visibles sin abandonar la aplicación.
    static const QHash<QString, QStringList> about = {
        {"es", {"Acerca de AlbumTagger","Versión","Autor","Editor de metadatos para organizar álbumes musicales.","Copyright © 2026 Tomás Fernández Galera.","El nombre AlbumTagger, su icono y su identidad visual no se conceden bajo esta licencia. Las versiones modificadas deben indicar claramente que no son oficiales.","Código fuente distribuido bajo GNU GPL versión 3.","Cerrar"}},
        {"en", {"About AlbumTagger","Version","Author","Metadata editor for organizing music albums.","Copyright © 2026 Tomás Fernández Galera.","The AlbumTagger name, icon and visual identity are not granted under this license. Modified versions must clearly state that they are unofficial.","Source code distributed under GNU GPL version 3.","Close"}},
        {"fr", {"À propos d’AlbumTagger","Version","Auteur","Éditeur de métadonnées pour organiser les albums musicaux.","Copyright © 2026 Tomás Fernández Galera.","Le nom AlbumTagger, son icône et son identité visuelle ne sont pas concédés sous cette licence. Les versions modifiées doivent indiquer clairement qu’elles ne sont pas officielles.","Code source distribué sous GNU GPL version 3.","Fermer"}},
        {"de", {"Über AlbumTagger","Version","Autor","Metadaten-Editor zum Organisieren von Musikalben.","Copyright © 2026 Tomás Fernández Galera.","Der Name AlbumTagger, das Symbol und die visuelle Identität werden unter dieser Lizenz nicht gewährt. Geänderte Versionen müssen klar als inoffiziell gekennzeichnet sein.","Quellcode unter GNU GPL Version 3.","Schließen"}},
        {"it", {"Informazioni su AlbumTagger","Versione","Autore","Editor di metadati per organizzare album musicali.","Copyright © 2026 Tomás Fernández Galera.","Il nome AlbumTagger, l’icona e l’identità visiva non sono concessi con questa licenza. Le versioni modificate devono indicare chiaramente di non essere ufficiali.","Codice sorgente distribuito con GNU GPL versione 3.","Chiudi"}},
        {"pt", {"Acerca do AlbumTagger","Versão","Autor","Editor de metadados para organizar álbuns de música.","Copyright © 2026 Tomás Fernández Galera.","O nome AlbumTagger, o ícone e a identidade visual não são concedidos por esta licença. As versões modificadas devem indicar claramente que não são oficiais.","Código-fonte distribuído sob a GNU GPL versão 3.","Fechar"}},
        {"nl", {"Over AlbumTagger","Versie","Auteur","Metadata-editor voor het organiseren van muziekalbums.","Copyright © 2026 Tomás Fernández Galera.","De naam AlbumTagger, het pictogram en de visuele identiteit vallen niet onder deze licentie. Gewijzigde versies moeten duidelijk vermelden dat ze niet officieel zijn.","Broncode verspreid onder GNU GPL versie 3.","Sluiten"}},
        {"pl", {"O AlbumTagger","Wersja","Autor","Edytor metadanych do porządkowania albumów muzycznych.","Copyright © 2026 Tomás Fernández Galera.","Nazwa AlbumTagger, ikona i identyfikacja wizualna nie są udostępniane na tej licencji. Zmodyfikowane wersje muszą wyraźnie informować, że nie są oficjalne.","Kod źródłowy rozpowszechniany na licencji GNU GPL w wersji 3.","Zamknij"}},
        {"ru", {"О программе AlbumTagger","Версия","Автор","Редактор метаданных для организации музыкальных альбомов.","Copyright © 2026 Tomás Fernández Galera.","Название AlbumTagger, значок и визуальный стиль не предоставляются по этой лицензии. Изменённые версии должны быть явно обозначены как неофициальные.","Исходный код распространяется по GNU GPL версии 3.","Закрыть"}},
        {"zh_CN", {"关于 AlbumTagger","版本","作者","用于整理音乐专辑的元数据编辑器。","Copyright © 2026 Tomás Fernández Galera。","AlbumTagger 名称、图标和视觉标识不在此许可证的授权范围内。修改版本必须明确声明其为非官方版本。","源代码依据 GNU GPL 第 3 版发布。","关闭"}},
        {"ja", {"AlbumTagger について","バージョン","作者","音楽アルバムを整理するためのメタデータエディターです。","Copyright © 2026 Tomás Fernández Galera.","AlbumTagger の名称、アイコン、ビジュアル・アイデンティティは本ライセンスの対象外です。変更版は非公式であることを明記する必要があります。","ソースコードは GNU GPL バージョン 3 で配布されています。","閉じる"}},
        {"ko", {"AlbumTagger 정보","버전","제작자","음악 앨범 정리를 위한 메타데이터 편집기입니다.","Copyright © 2026 Tomás Fernández Galera.","AlbumTagger 이름, 아이콘 및 시각적 정체성은 이 라이선스에 포함되지 않습니다. 수정 버전은 비공식임을 명확히 표시해야 합니다.","소스 코드는 GNU GPL 버전 3으로 배포됩니다.","닫기"}},
        {"ca", {"Quant a AlbumTagger","Versió","Autor","Editor de metadades per organitzar àlbums de música.","Copyright © 2026 Tomás Fernández Galera.","El nom AlbumTagger, la icona i la identitat visual no es concedeixen sota aquesta llicència. Les versions modificades han d’indicar clarament que no són oficials.","Codi font distribuït sota la GNU GPL versió 3.","Tanca"}},
        {"eu", {"AlbumTagger-i buruz","Bertsioa","Egilea","Musika-albumak antolatzeko metadatu-editorea.","Copyright © 2026 Tomás Fernández Galera.","AlbumTagger izena, ikonoa eta ikusizko nortasuna ez dira lizentzia honen bidez ematen. Aldatutako bertsioek argi adierazi behar dute ez direla ofizialak.","Iturburu-kodea GNU GPL 3. bertsioaren pean banatzen da.","Itxi"}},
        {"gl", {"Acerca de AlbumTagger","Versión","Autor","Editor de metadatos para organizar álbums de música.","Copyright © 2026 Tomás Fernández Galera.","O nome AlbumTagger, a icona e a identidade visual non se conceden baixo esta licenza. As versións modificadas deben indicar claramente que non son oficiais.","Código fonte distribuído baixo GNU GPL versión 3.","Pechar"}}
    };
    const QStringList s = about.value(languageCode_, about.value(QStringLiteral("en")));
    QMessageBox dialog(this);
    dialog.setWindowTitle(s[0]);
    dialog.setIconPixmap(QIcon(QStringLiteral(":/app/icon.svg")).pixmap(96, 96));
    dialog.setTextFormat(Qt::RichText);
    dialog.setText(QStringLiteral(
        "<div style='min-width:360px'>"
        "<h2>AlbumTagger</h2>"
        "<p><b>%1 %2</b></p>"
        "<p>%3: <b>Tomás Fernández Galera</b></p>"
        "<p>%4</p><p>%5</p><p>%6</p><p><i>%7</i></p>"
        "</div>").arg(s[1], QCoreApplication::applicationVersion(), s[2],
                       s[3], s[4], s[5], s[6]));
    dialog.setStandardButtons(QMessageBox::Ok);
    dialog.button(QMessageBox::Ok)->setText(s[7]);
    dialog.exec();
}
