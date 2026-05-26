---
name: Qt6 C++ Desktop Application Development
description: "Workflow for building, compiling, and deploying modern Qt 6 C++ desktop applications on Windows using MinGW-w64 UCRT."
author: Antigravity
version: 1.0.0
tags: [cpp, qt, qt6, cmake, mingw, windows, desktop]
---

# Qt 6 C++ Desktop Application Development

This skill defines the established best practices for structuring, compiling, and deploying modern Qt 6 C++ native applications on Windows, particularly when utilizing a MinGW-w64 UCRT toolchain and CMake.

## 1. Toolchain Setup

When setting up a Windows C++ development environment completely isolated from MSVC constraints, prefer the open-source **MinGW-w64 UCRT** (Universal C Runtime) over older MSVCRT variants.

- **CMake**: Use `winget install -e --id Kitware.CMake` to get the latest CMake.
- **Compiler**: Use a modern POSIX-threaded MinGW distribution (e.g., Brecht Sanders WinLibs GCC 16+).
- **Qt Installation**: Use `aqtinstall` (Python wrapper) to fetch Qt 6 binaries headless without the massive online installer.
  ```powershell
  pip install aqtinstall
  python -m aqt install-qt windows desktop 6.8.3 win64_mingw --outputdir C:\Qt
  ```

## 2. Architecture & Design Patterns

Do **not** use a monolithic `main.cpp`. Modern Qt applications should adhere to strict MVC/Service-oriented architectures.

- **UI Layer (`src/ui/`)**: Contains `QMainWindow`, dialogs, and specific `QWidget` views. Update the UI *only* from the main thread.
- **Service Layer (`src/core/services/`)**: Contains business logic (Search, Export, File Traversal). These should run heavily on background `QThread` instances.
- **Model Layer (`src/core/models/`)**: Defines the data structures (e.g., `FileSystemNode`). Use `std::shared_ptr` combined with `std::enable_shared_from_this` for tree structures.
- **Thread Communication**: Offload heavy IO/CPU work to worker classes deriving from `QRunnable` or using raw `QThread` inheritance. Push results back to the main thread via Qt Signals (`emit progressUpdated()`, `emit resultsReady(batch)`). Use `qRegisterMetaType` for custom structs passed through signals.

## 3. CMake Configuration (MinGW Quirks)

When building Qt 6 on Windows with MinGW, specific flags are required to bypass linker issues (`undefined reference to '__imp___argc'`).

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyQtApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# DO NOT ADD WIN32 HERE on MinGW if avoiding Qt6EntryPoint linker bugs
add_executable(MyQtApp ${SOURCES})

if (MINGW)
    # Hide the terminal window manually instead of relying on WIN32_EXECUTABLE
    target_link_options(MyQtApp PRIVATE "-mwindows")
    # Prevent Qt from auto-linking the faulty Qt6EntryPoint.a on certain UCRT builds
    set_target_properties(MyQtApp PROPERTIES QT_NO_ENTRYPOINT TRUE)
endif()

target_link_libraries(MyQtApp PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
)
```

## 4. Modern Qt 6 Syntax Reminders

- **QButtonGroup**: `buttonClicked(int)` was deprecated. Use `idClicked(int)` in Qt 6.
- **QTextStream**: `setCodec("UTF-8")` was removed. Use `setEncoding(QStringConverter::Utf8)`.
- **QCryptographicHash**: When hashing data, `addData(const char*, int)` is deprecated. Wrap it in a `QByteArrayView` overload or cast appropriately.

## 5. Deployment

Once the executable is built, you cannot distribute it without its dynamic libraries. Use Qt's `windeployqt` tool to traverse the `.exe` and automatically bundle necessary DLLs.

```powershell
C:\Qt\6.8.3\mingw_64\bin\windeployqt.exe build\MyQtApp.exe
```
This extracts modules like `Qt6Core.dll`, `Qt6Gui.dll`, and necessary platform plugins (e.g., `platforms/qwindows.dll`) so the application becomes entirely portable on Windows machines.
