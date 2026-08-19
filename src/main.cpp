#include "mainwindow.h"

#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QScreen>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    // QApplication debe existir antes de crear iconos, estilos o widgets.
    QApplication app(argc, argv);

    // Estos metadatos alimentan QSettings y el diálogo Acerca de.
    app.setApplicationName(QStringLiteral("AlbumTagger"));
    app.setOrganizationName(QStringLiteral("AlbumTagger"));
    app.setApplicationVersion(QStringLiteral(ALBUMTAGGER_VERSION));
    app.setWindowIcon(QIcon(QStringLiteral(":/app/icon.svg")));
    // Fusion ofrece un aspecto consistente entre distintas versiones de Windows.
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    // Tema global. Los nombres #... corresponden a objetos definidos en el .ui.
    // Mantener aquí los colores facilita un futuro tema oscuro.
    app.setStyleSheet(QStringLiteral(R"(
        QMainWindow { background: #f3f5f8; }
        QWidget { color: #253047; font-family: "Segoe UI"; font-size: 10pt; }
        QFrame#headerFrame, QFrame#editorCard, QFrame#coverCard {
            background: white; border: 1px solid #dde3ec; border-radius: 12px;
        }
        QLabel#titleLabel { font-size: 22pt; font-weight: 700; color: #17213a; }
        QLabel#subtitleLabel, QLabel#coverInfoLabel { color: #6d7890; }
        QPushButton { background: #e9edf5; border: 0; border-radius: 7px; padding: 8px 14px; }
        QPushButton:hover { background: #dce3ef; }
        QPushButton#openFolderButton, QPushButton#applyButton {
            background: #5965dc; color: white; font-weight: 600;
        }
        QPushButton#openFolderButton:hover, QPushButton#applyButton:hover { background: #4854c9; }
        QPushButton:disabled { background: #cdd2dd; color: #7f8798; }
        QLineEdit, QSpinBox, QComboBox {
            background: #f8f9fc; border: 1px solid #d8deea; border-radius: 6px; padding: 7px;
        }
        QTableWidget { background: white; alternate-background-color: #f7f8fb; border: 1px solid #dde3ec; border-radius: 8px; gridline-color: #edf0f5; }
        QHeaderView::section { background: #e9edf5; border: 0; border-right: 2px solid #bac4d6; padding: 8px; font-weight: 600; }
        QStatusBar { background: white; color: #657089; }
    )"));
    MainWindow window;
    // Ajustar al área útil evita que la barra de tareas o el escalado DPI dejen
    // controles fuera de pantalla. Los límites mantienen la aplicación usable.
    const QRect available = QGuiApplication::primaryScreen()->availableGeometry();
    const int width = qMin(1180, qMax(760, static_cast<int>(available.width() * 0.88)));
    const int height = qMin(760, qMax(520, static_cast<int>(available.height() * 0.86)));
    window.setMinimumSize(760, 520);
    window.resize(width, height);
    window.move(available.center() - window.rect().center());
    window.show();
    return app.exec();
}
