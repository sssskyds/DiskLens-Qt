#include "app/SettingsManager.h"

SettingsManager& SettingsManager::instance() {
    static SettingsManager mgr;
    return mgr;
}

SettingsManager::SettingsManager(QObject* parent)
    : QObject(parent)
    , m_settings("DiskLens", "DiskLens")
{
}

// --- Scan Settings ---
int SettingsManager::defaultScanDepth() const {
    return m_settings.value("scan/defaultDepth", DEFAULT_SCAN_DEPTH).toInt();
}
void SettingsManager::setDefaultScanDepth(int depth) {
    m_settings.setValue("scan/defaultDepth", qBound(1, depth, 50));
    emit settingsChanged("scan/defaultDepth");
}

int SettingsManager::largeFileTopN() const {
    return m_settings.value("scan/largeFileTopN", DEFAULT_LARGE_FILE_TOP_N).toInt();
}
void SettingsManager::setLargeFileTopN(int n) {
    m_settings.setValue("scan/largeFileTopN", qBound(10, n, 1000));
    emit settingsChanged("scan/largeFileTopN");
}

bool SettingsManager::scanIncludeHidden() const {
    return m_settings.value("scan/includeHidden", false).toBool();
}
void SettingsManager::setScanIncludeHidden(bool include) {
    m_settings.setValue("scan/includeHidden", include);
    emit settingsChanged("scan/includeHidden");
}

// --- UI Settings ---
int SettingsManager::sidebarWidth() const {
    return m_settings.value("ui/sidebarWidth", DEFAULT_SIDEBAR_WIDTH).toInt();
}
void SettingsManager::setSidebarWidth(int width) {
    m_settings.setValue("ui/sidebarWidth", width);
}

QString SettingsManager::lastScanPath() const {
    return m_settings.value("ui/lastScanPath", "").toString();
}
void SettingsManager::setLastScanPath(const QString& path) {
    m_settings.setValue("ui/lastScanPath", path);
}

QStringList SettingsManager::recentPaths() const {
    return m_settings.value("ui/recentPaths").toStringList();
}
void SettingsManager::addRecentPath(const QString& path) {
    QStringList list = recentPaths();
    list.removeAll(path);
    list.prepend(path);
    while (list.size() > MAX_RECENT_PATHS) list.removeLast();
    m_settings.setValue("ui/recentPaths", list);
}
void SettingsManager::clearRecentPaths() {
    m_settings.setValue("ui/recentPaths", QStringList());
}

QStringList SettingsManager::favoritePaths() const {
    return m_settings.value("ui/favoritePaths").toStringList();
}
void SettingsManager::addFavoritePath(const QString& path) {
    QStringList list = favoritePaths();
    if (!list.contains(path)) list.append(path);
    m_settings.setValue("ui/favoritePaths", list);
}
void SettingsManager::removeFavoritePath(const QString& path) {
    QStringList list = favoritePaths();
    list.removeAll(path);
    m_settings.setValue("ui/favoritePaths", list);
}

QStringList SettingsManager::ignoredPaths() const {
    return m_settings.value("scan/ignoredPaths").toStringList();
}
void SettingsManager::addIgnoredPath(const QString& path) {
    QStringList list = ignoredPaths();
    if (!list.contains(path)) list.append(path);
    m_settings.setValue("scan/ignoredPaths", list);
}
void SettingsManager::removeIgnoredPath(const QString& path) {
    QStringList list = ignoredPaths();
    list.removeAll(path);
    m_settings.setValue("scan/ignoredPaths", list);
}

// --- Export Settings ---
int SettingsManager::exportMaxDepth() const {
    return m_settings.value("export/maxDepth", DEFAULT_EXPORT_DEPTH).toInt();
}
void SettingsManager::setExportMaxDepth(int depth) {
    m_settings.setValue("export/maxDepth", qBound(1, depth, 50));
}

bool SettingsManager::exportIncludeHidden() const {
    return m_settings.value("export/includeHidden", false).toBool();
}
void SettingsManager::setExportIncludeHidden(bool include) {
    m_settings.setValue("export/includeHidden", include);
}

// --- Network Drive Settings ---
int SettingsManager::networkScanDepth() const {
    return m_settings.value("network/scanDepth", DEFAULT_NETWORK_DEPTH).toInt();
}
void SettingsManager::setNetworkScanDepth(int depth) {
    m_settings.setValue("network/scanDepth", qBound(1, depth, 20));
}

int SettingsManager::networkConcurrency() const {
    return m_settings.value("network/concurrency", DEFAULT_NETWORK_CONCURRENCY).toInt();
}
void SettingsManager::setNetworkConcurrency(int c) {
    m_settings.setValue("network/concurrency", qBound(1, c, 8));
}

// --- Window Geometry ---
QByteArray SettingsManager::windowGeometry() const {
    return m_settings.value("window/geometry").toByteArray();
}
void SettingsManager::setWindowGeometry(const QByteArray& geo) {
    m_settings.setValue("window/geometry", geo);
}

QByteArray SettingsManager::windowState() const {
    return m_settings.value("window/state").toByteArray();
}
void SettingsManager::setWindowState(const QByteArray& state) {
    m_settings.setValue("window/state", state);
}
