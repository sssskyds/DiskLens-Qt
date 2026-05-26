#include "ui/MainWindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <iostream>

int main(int argc, char *argv[]) {
    std::cerr << "DEBUG: Entered main()" << std::endl;

    // Enable High DPI scaling for modern premium screens (4k etc)
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    std::cerr << "DEBUG: Setting up dummy argc/argv..." << std::endl;
    int dummyArgc = 1;
    char *dummyArgv[] = { (char*)"DiskLens", nullptr };
    
    std::cerr << "DEBUG: Creating QApplication..." << std::endl;
    QApplication app(dummyArgc, dummyArgv);
    
    std::cerr << "DEBUG: Setting application parameters..." << std::endl;
    app.setApplicationName("DiskLens");
    std::cerr << "DEBUG: Application name set." << std::endl;
    app.setApplicationVersion("1.0.0");
    std::cerr << "DEBUG: Application version set." << std::endl;
    app.setOrganizationName("DiskLens");
    std::cerr << "DEBUG: Organization name set." << std::endl;

    std::cerr << "DEBUG: Creating Fusion style..." << std::endl;
    QStyle* fusionStyle = QStyleFactory::create("Fusion");
    std::cerr << "DEBUG: Fusion style pointer: " << fusionStyle << std::endl;
    
    std::cerr << "DEBUG: Setting style on app..." << std::endl;
    if (fusionStyle) {
        app.setStyle(fusionStyle);
        std::cerr << "DEBUG: Style set successfully." << std::endl;
    } else {
        std::cerr << "DEBUG: Fusion style was null!" << std::endl;
    }

    std::cerr << "DEBUG: Creating MainWindow..." << std::endl;
    MainWindow window;
    
    std::cerr << "DEBUG: Showing MainWindow..." << std::endl;
    window.show();

    std::cerr << "DEBUG: Executing QApplication event loop..." << std::endl;
    int ret = app.exec();
    std::cerr << "DEBUG: Exiting main() with code " << ret << std::endl;
    return ret;
}
