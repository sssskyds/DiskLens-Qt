#include "core/services/TemplateService.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDirIterator>
#include <QDateTime>

TemplateService::TemplateService(QObject* parent) : QObject(parent) {
    initBuiltInTemplates();
    // Load custom templates from disk
    QDir customDir(customTemplatesDir());
    if (customDir.exists()) {
        QStringList files = customDir.entryList({"*.json"}, QDir::Files);
        for (const QString& f : files) {
            ProjectTemplate t = loadTemplateFromJson(customDir.absoluteFilePath(f));
            if (!t.id.isEmpty()) m_customTemplates.append(t);
        }
    }
}

void TemplateService::initBuiltInTemplates() {
    // ── Node.js Project ──
    {
        ProjectTemplate t;
        t.id = "nodejs"; t.name = "Node.js Project"; t.description = "Standard Node.js project with package.json";
        t.isBuiltIn = true;
        t.entries = {
            {"src", true, "", false},
            {"tests", true, "", false},
            {"docs", true, "", false},
            {"package.json", false, "{\n  \"name\": \"{name}\",\n  \"version\": \"1.0.0\",\n  \"main\": \"src/index.js\",\n  \"scripts\": {\n    \"start\": \"node src/index.js\",\n    \"test\": \"jest\"\n  }\n}", true},
            {"src/index.js", false, "// {name} entry point\nconsole.log('Hello from {name}!');\n", true},
            {".gitignore", false, "node_modules/\ndist/\n.env\n", false},
            {"README.md", false, "# {name}\n\nA Node.js project.\n", true},
        };
        m_builtInTemplates.append(t);
    }
    // ── Python Package ──
    {
        ProjectTemplate t;
        t.id = "python"; t.name = "Python Package"; t.description = "Python package with setup.py and tests";
        t.isBuiltIn = true;
        t.entries = {
            {"{name}", true, "", true},
            {"{name}/__init__.py", false, "\"\"\"{ name } package.\"\"\"\n__version__ = '1.0.0'\n", true},
            {"{name}/main.py", false, "def main():\n    print('Hello from {name}!')\n\nif __name__ == '__main__':\n    main()\n", true},
            {"tests", true, "", false},
            {"tests/__init__.py", false, "", false},
            {"tests/test_main.py", false, "import unittest\n\nclass TestMain(unittest.TestCase):\n    def test_placeholder(self):\n        self.assertTrue(True)\n", false},
            {"setup.py", false, "from setuptools import setup, find_packages\n\nsetup(\n    name='{name}',\n    version='1.0.0',\n    packages=find_packages(),\n)\n", true},
            {"requirements.txt", false, "# Add dependencies here\n", false},
            {".gitignore", false, "__pycache__/\n*.pyc\n*.egg-info/\ndist/\nbuild/\n.venv/\n", false},
            {"README.md", false, "# {name}\n\nA Python package.\n", true},
        };
        m_builtInTemplates.append(t);
    }
    // ── Web Frontend ──
    {
        ProjectTemplate t;
        t.id = "web-frontend"; t.name = "Web Frontend"; t.description = "Basic HTML/CSS/JS frontend project";
        t.isBuiltIn = true;
        t.entries = {
            {"css", true, "", false},
            {"js", true, "", false},
            {"images", true, "", false},
            {"index.html", false, "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"UTF-8\">\n  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n  <title>{name}</title>\n  <link rel=\"stylesheet\" href=\"css/style.css\">\n</head>\n<body>\n  <h1>{name}</h1>\n  <script src=\"js/app.js\"></script>\n</body>\n</html>\n", true},
            {"css/style.css", false, "* { margin: 0; padding: 0; box-sizing: border-box; }\nbody { font-family: sans-serif; }\n", false},
            {"js/app.js", false, "// {name} application\nconsole.log('{name} loaded');\n", true},
            {"README.md", false, "# {name}\n\nA web frontend project.\n", true},
        };
        m_builtInTemplates.append(t);
    }
    // ── Data Science ──
    {
        ProjectTemplate t;
        t.id = "data-science"; t.name = "Data Science"; t.description = "Data science project with notebooks and data folders";
        t.isBuiltIn = true;
        t.entries = {
            {"data/raw", true, "", false},
            {"data/processed", true, "", false},
            {"notebooks", true, "", false},
            {"src", true, "", false},
            {"models", true, "", false},
            {"reports", true, "", false},
            {"src/__init__.py", false, "", false},
            {"src/data_loader.py", false, "# Data loading utilities for {name}\n", true},
            {"requirements.txt", false, "numpy\npandas\nmatplotlib\nscikit-learn\njupyter\n", false},
            {".gitignore", false, "data/raw/\n*.pyc\n__pycache__/\n.ipynb_checkpoints/\n.venv/\n", false},
            {"README.md", false, "# {name}\n\nA data science project.\n\n## Structure\n- `data/` — Raw and processed datasets\n- `notebooks/` — Jupyter notebooks\n- `src/` — Source code\n- `models/` — Trained models\n- `reports/` — Generated reports\n", true},
        };
        m_builtInTemplates.append(t);
    }
    // ── C++ Project ──
    {
        ProjectTemplate t;
        t.id = "cpp-project"; t.name = "C++ Project"; t.description = "CMake-based C++ project";
        t.isBuiltIn = true;
        t.entries = {
            {"src", true, "", false},
            {"include", true, "", false},
            {"tests", true, "", false},
            {"build", true, "", false},
            {"CMakeLists.txt", false, "cmake_minimum_required(VERSION 3.16)\nproject({name} VERSION 1.0.0 LANGUAGES CXX)\nset(CMAKE_CXX_STANDARD 17)\nadd_executable({name} src/main.cpp)\n", true},
            {"src/main.cpp", false, "#include <iostream>\n\nint main() {\n    std::cout << \"Hello from {name}!\" << std::endl;\n    return 0;\n}\n", true},
            {".gitignore", false, "build/\n*.o\n*.exe\nCMakeCache.txt\n", false},
            {"README.md", false, "# {name}\n\nA C++ project built with CMake.\n", true},
        };
        m_builtInTemplates.append(t);
    }
    // ── General Workspace ──
    {
        ProjectTemplate t;
        t.id = "workspace"; t.name = "General Workspace"; t.description = "Clean workspace with common folders";
        t.isBuiltIn = true;
        t.entries = {
            {"docs", true, "", false},
            {"assets", true, "", false},
            {"src", true, "", false},
            {"output", true, "", false},
            {"archive", true, "", false},
            {"README.md", false, "# {name}\n\nWorkspace created on {date}.\n", true},
        };
        m_builtInTemplates.append(t);
    }
}

QVector<ProjectTemplate> TemplateService::allTemplates() const {
    QVector<ProjectTemplate> all = m_builtInTemplates;
    all.append(m_customTemplates);
    return all;
}

QVector<ProjectTemplate> TemplateService::builtInTemplates() const { return m_builtInTemplates; }
QVector<ProjectTemplate> TemplateService::customTemplates() const { return m_customTemplates; }

ProjectTemplate TemplateService::getTemplate(const QString& id) const {
    for (const auto& t : m_builtInTemplates) if (t.id == id) return t;
    for (const auto& t : m_customTemplates)  if (t.id == id) return t;
    return {};
}

QStringList TemplateService::previewTemplate(const QString& templateId, const QString& projectName) {
    QStringList paths;
    ProjectTemplate t = getTemplate(templateId);
    for (const auto& entry : t.entries) {
        QString path = expandPlaceholders(entry.relativePath, projectName);
        if (entry.isDirectory) path += "/";
        paths.append(path);
    }
    return paths;
}

bool TemplateService::createFromTemplate(const QString& templateId,
                                          const QString& destinationDir,
                                          const QString& projectName) {
    ProjectTemplate t = getTemplate(templateId);
    if (t.id.isEmpty()) {
        emit error("Template not found: " + templateId);
        return false;
    }

    QString rootDir = destinationDir + "/" + projectName;
    QDir().mkpath(rootDir);

    for (const auto& entry : t.entries) {
        QString relPath = expandPlaceholders(entry.relativePath, projectName);
        QString fullPath = rootDir + "/" + relPath;

        if (entry.isDirectory) {
            QDir().mkpath(fullPath);
        } else {
            // Ensure parent directory exists
            QFileInfo fi(fullPath);
            QDir().mkpath(fi.absolutePath());

            QFile file(fullPath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QString content = entry.isPlaceholder
                    ? expandPlaceholders(entry.content, projectName)
                    : entry.content;
                file.write(content.toUtf8());
                file.close();
            }
        }
    }

    emit templateCreated(rootDir);
    return true;
}

ProjectTemplate TemplateService::captureFromFolder(const QString& folderPath,
                                                     const QString& templateName,
                                                     bool includeFileContent) {
    ProjectTemplate t;
    t.id = "custom-" + QString::number(QDateTime::currentMSecsSinceEpoch());
    t.name = templateName;
    t.description = "Captured from " + folderPath;
    t.isBuiltIn = false;

    QDir rootDir(folderPath);
    QDirIterator it(folderPath, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo fi = it.fileInfo();
        QString relPath = rootDir.relativeFilePath(fi.absoluteFilePath());

        // Skip binary files and very large files for content capture
        if (!fi.isDir() && fi.size() > 1024 * 1024) continue; // Skip > 1MB

        TemplateEntry entry;
        entry.relativePath = relPath;
        entry.isDirectory = fi.isDir();

        if (!fi.isDir() && includeFileContent) {
            QFile file(fi.absoluteFilePath());
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                entry.content = file.readAll();
                file.close();
            }
        }

        t.entries.append(entry);
    }

    return t;
}

bool TemplateService::saveCustomTemplate(const ProjectTemplate& tmpl) {
    QDir dir(customTemplatesDir());
    dir.mkpath(".");
    QString filePath = dir.absoluteFilePath(tmpl.id + ".json");
    if (saveTemplateToJson(tmpl, filePath)) {
        m_customTemplates.append(tmpl);
        return true;
    }
    return false;
}

bool TemplateService::deleteCustomTemplate(const QString& id) {
    QDir dir(customTemplatesDir());
    QString filePath = dir.absoluteFilePath(id + ".json");
    QFile::remove(filePath);
    for (int i = 0; i < m_customTemplates.size(); i++) {
        if (m_customTemplates[i].id == id) {
            m_customTemplates.removeAt(i);
            return true;
        }
    }
    return false;
}

// ─── Private Helpers ─────────────────────────────────────────────

QString TemplateService::expandPlaceholders(const QString& text, const QString& projectName) {
    QString result = text;
    result.replace("{name}", projectName);
    result.replace("{date}", QDateTime::currentDateTime().toString("yyyy-MM-dd"));
    result.replace("{year}", QDateTime::currentDateTime().toString("yyyy"));
    return result;
}

QString TemplateService::customTemplatesDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/templates";
}

bool TemplateService::saveTemplateToJson(const ProjectTemplate& tmpl, const QString& filePath) {
    QJsonObject root;
    root["id"] = tmpl.id;
    root["name"] = tmpl.name;
    root["description"] = tmpl.description;

    QJsonArray entries;
    for (const auto& e : tmpl.entries) {
        QJsonObject obj;
        obj["path"] = e.relativePath;
        obj["isDir"] = e.isDirectory;
        if (!e.content.isEmpty()) obj["content"] = e.content;
        obj["placeholder"] = e.isPlaceholder;
        entries.append(obj);
    }
    root["entries"] = entries;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    return true;
}

ProjectTemplate TemplateService::loadTemplateFromJson(const QString& filePath) {
    ProjectTemplate t;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return t;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject root = doc.object();

    t.id = root["id"].toString();
    t.name = root["name"].toString();
    t.description = root["description"].toString();
    t.isBuiltIn = false;

    QJsonArray entries = root["entries"].toArray();
    for (const auto& val : entries) {
        QJsonObject obj = val.toObject();
        TemplateEntry e;
        e.relativePath = obj["path"].toString();
        e.isDirectory = obj["isDir"].toBool();
        e.content = obj["content"].toString();
        e.isPlaceholder = obj["placeholder"].toBool();
        t.entries.append(e);
    }
    return t;
}
