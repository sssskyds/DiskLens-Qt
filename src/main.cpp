#include "ui/MainWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[]) {
    // Enable High DPI scaling for modern premium screens (4k etc)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    int dummyArgc = 1;
    char *dummyArgv[] = { (char*)"DiskLens", nullptr };
    QApplication app(dummyArgc, dummyArgv);
    app.setApplicationName("DiskLens");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("DiskLens");

    // Standard Fusion style as base for custom styling
    app.setStyle(QStyleFactory::create("Fusion"));

    MainWindow window;
    window.show();

    return app.exec();
}
