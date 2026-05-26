#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <QVariant>

/**
 * @brief Centralized settings manager using QSettings for persistence.
 * Provides typed getters/setters for all application configuration.
 */
class SettingsManager : public QObject {
    Q_OBJECT
public:
    static SettingsManager& instance();

    // --- Scan Settings ---
    int defaultScanDepth() const;
    void setDefaultScanDepth(int depth);

    int largeFileTopN() const;
    void setLargeFileTopN(int n);

    bool scanIncludeHidden() const;
    void setScanIncludeHidden(bool include);

    // --- UI Settings ---
    int sidebarWidth() const;
    void setSidebarWidth(int width);

    QString lastScanPath() const;
    void setLastScanPath(const QString& path);

    QStringList recentPaths() const;
    void addRecentPath(const QString& path);
    void clearRecentPaths();

    QStringList favoritePaths() const;
    void addFavoritePath(const QString& path);
    void removeFavoritePath(const QString& path);

    QStringList ignoredPaths() const;
    void addIgnoredPath(const QString& path);
    void removeIgnoredPath(const QString& path);

    // --- Export Settings ---
    int exportMaxDepth() const;
    void setExportMaxDepth(int depth);

    bool exportIncludeHidden() const;
    void setExportIncludeHidden(bool include);

    // --- Network Drive Settings ---
    int networkScanDepth() const;
    void setNetworkScanDepth(int depth);

    int networkConcurrency() const;
    void setNetworkConcurrency(int c);

    // --- Window Geometry ---
    QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geo);

    QByteArray windowState() const;
    void setWindowState(const QByteArray& state);

signals:
    void settingsChanged(const QString& key);

private:
    explicit SettingsManager(QObject* parent = nullptr);
    ~SettingsManager() = default;
    Q_DISABLE_COPY(SettingsManager)

    QSettings m_settings;

    static const int DEFAULT_SCAN_DEPTH = 4;
    static const int DEFAULT_LARGE_FILE_TOP_N = 100;
    static const int DEFAULT_EXPORT_DEPTH = 6;
    static const int DEFAULT_NETWORK_DEPTH = 3;
    static const int DEFAULT_NETWORK_CONCURRENCY = 2;
    static const int DEFAULT_SIDEBAR_WIDTH = 220;
    static const int MAX_RECENT_PATHS = 10;
};

#endif // SETTINGSMANAGER_H
