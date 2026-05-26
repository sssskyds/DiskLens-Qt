cmake_minimum_required(VERSION 3.16)
project(DiskLens VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automatic compilation of Qt files
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 or Qt5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Gui Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Gui Concurrent)

message(STATUS "Found Qt: Version ${Qt${QT_VERSION_MAJOR}_VERSION} (Major: ${QT_VERSION_MAJOR})")

# Header directories
include_directories(src)

# Define source files
set(SOURCES
    # Entry point
    src/main.cpp

    # App layer
    src/app/SettingsManager.h
    src/app/SettingsManager.cpp

    # Core models
    src/core/models/VolumeInfo.h
    src/core/models/FileSystemNode.h
    src/core/models/FileSystemNode.cpp

    # Core services
    src/core/services/VolumeService.h
    src/core/services/VolumeService.cpp
    src/core/services/ScanService.h
    src/core/services/ScanService.cpp
    src/core/services/LargeFileService.h
    src/core/services/LargeFileService.cpp
    src/core/services/FileTypeService.h
    src/core/services/FileTypeService.cpp
    src/core/services/DuplicateService.h
    src/core/services/DuplicateService.cpp
    src/core/services/FileOperationService.h
    src/core/services/FileOperationService.cpp
    src/core/services/SearchService.h
    src/core/services/SearchService.cpp
    src/core/services/ExportService.h
    src/core/services/ExportService.cpp
    src/core/services/OrganizerService.h
    src/core/services/OrganizerService.cpp
    src/core/services/TemplateService.h
    src/core/services/TemplateService.cpp

    # Platform services
    src/platform/ShellIntegrationService.h
    src/platform/ShellIntegrationService.cpp

    # UI — Main Window
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp

    # UI — Custom Widgets
    src/ui/widgets/RadialGauge.h
    src/ui/widgets/RadialGauge.cpp
    src/ui/widgets/TreemapWidget.h
    src/ui/widgets/TreemapWidget.cpp
    src/ui/widgets/DonutChartWidget.h
    src/ui/widgets/DonutChartWidget.cpp

    # UI — Views
    src/ui/views/DashboardView.h
    src/ui/views/DashboardView.cpp
    src/ui/views/TreemapView.h
    src/ui/views/TreemapView.cpp
    src/ui/views/LargeFilesView.h
    src/ui/views/LargeFilesView.cpp
    src/ui/views/FileTypesView.h
    src/ui/views/FileTypesView.cpp
    src/ui/views/DuplicatesView.h
    src/ui/views/DuplicatesView.cpp
    src/ui/views/SearchOrganizerView.h
    src/ui/views/SearchOrganizerView.cpp
    src/ui/views/AdvancedSearchView.h
    src/ui/views/AdvancedSearchView.cpp
    src/ui/views/OrganizerView.h
    src/ui/views/OrganizerView.cpp
    src/ui/views/TemplatesView.h
    src/ui/views/TemplatesView.cpp

    # UI — Dialogs
    src/ui/dialogs/ExportDialog.h
    src/ui/dialogs/ExportDialog.cpp

    # Resources
    resources.qrc
)

add_executable(DiskLens
    ${SOURCES}
    ${UI_SOURCES}
    ${CORE_SOURCES}
    ${SERVICE_SOURCES}
    ${VIEW_SOURCES}
    ${DIALOG_SOURCES}
)

# if (MINGW)
#     target_link_options(DiskLens PRIVATE "-mwindows")
#     set_target_properties(DiskLens PROPERTIES QT_NO_ENTRYPOINT TRUE)
# endif()

target_link_libraries(DiskLens PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Concurrent
)

if (WIN32)
    # Link additional libraries for shell integration, Recycle Bin, drive classification
    target_link_libraries(DiskLens PRIVATE Shell32 Ole32)
endif()

# Install target
install(TARGETS DiskLens DESTINATION bin)
