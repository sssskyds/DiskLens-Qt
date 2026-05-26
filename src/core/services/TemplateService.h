#ifndef TEMPLATESERVICE_H
#define TEMPLATESERVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QVector>

/**
 * @brief A single entry in a project template (file or folder).
 */
struct TemplateEntry {
    QString relativePath;    // e.g., "src/main.cpp"
    bool isDirectory = false;
    QString content;         // File content (empty for directories)
    bool isPlaceholder = false; // Contains {name}, {date} etc.
};

/**
 * @brief A project folder template definition.
 */
struct ProjectTemplate {
    QString id;              // Unique identifier
    QString name;            // Display name
    QString description;     // Short description
    QString icon;            // Icon name/path
    bool isBuiltIn = false;
    QVector<TemplateEntry> entries;
};

/**
 * @brief Service for managing and creating project folder templates.
 * Supports built-in templates, custom user templates, and template capture from existing folders.
 */
class TemplateService : public QObject {
    Q_OBJECT
public:
    explicit TemplateService(QObject* parent = nullptr);

    // Get all available templates (built-in + custom)
    QVector<ProjectTemplate> allTemplates() const;
    QVector<ProjectTemplate> builtInTemplates() const;
    QVector<ProjectTemplate> customTemplates() const;

    // Get template by ID
    ProjectTemplate getTemplate(const QString& id) const;

    // Create folders from template
    bool createFromTemplate(const QString& templateId,
                            const QString& destinationDir,
                            const QString& projectName);

    // Preview what will be created
    QStringList previewTemplate(const QString& templateId,
                                const QString& projectName);

    // Custom template management
    bool saveCustomTemplate(const ProjectTemplate& tmpl);
    bool deleteCustomTemplate(const QString& id);

    // Capture template from existing folder
    ProjectTemplate captureFromFolder(const QString& folderPath,
                                       const QString& templateName,
                                       bool includeFileContent = false);

signals:
    void templateCreated(const QString& destinationDir);
    void error(const QString& message);

private:
    void initBuiltInTemplates();
    QString expandPlaceholders(const QString& text, const QString& projectName);
    QString customTemplatesDir() const;
    bool saveTemplateToJson(const ProjectTemplate& tmpl, const QString& filePath);
    ProjectTemplate loadTemplateFromJson(const QString& filePath);

    QVector<ProjectTemplate> m_builtInTemplates;
    QVector<ProjectTemplate> m_customTemplates;
};

#endif // TEMPLATESERVICE_H
