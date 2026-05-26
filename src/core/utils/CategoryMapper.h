#pragma once
#ifndef CATEGORYMAPPER_H
#define CATEGORYMAPPER_H

#include <QString>
#include <QHash>

/**
 * @brief Single source of truth for extension → category mapping.
 *
 * Used by ScanWorker, SearchWorker, LargeFileWorker, FileTypeService, and
 * any future module that needs category classification.  Never duplicate
 * this logic elsewhere.
 */
class CategoryMapper {
public:
    static QString classify(const QString& extension) {
        const QString ext = extension.startsWith('.')
                                ? extension.mid(1).toLower()
                                : extension.toLower();

        static const QHash<QString, QString> map = {
            // Images
            {"jpg","Images"},{"jpeg","Images"},{"png","Images"},{"gif","Images"},
            {"bmp","Images"},{"svg","Images"},{"webp","Images"},{"ico","Images"},
            {"tiff","Images"},{"tif","Images"},{"raw","Images"},{"cr2","Images"},
            {"nef","Images"},{"heic","Images"},{"avif","Images"},
            // Videos
            {"mp4","Videos"},{"avi","Videos"},{"mkv","Videos"},{"mov","Videos"},
            {"wmv","Videos"},{"flv","Videos"},{"webm","Videos"},{"m4v","Videos"},
            {"mpg","Videos"},{"mpeg","Videos"},{"3gp","Videos"},{"ts","Videos"},
            // Audio
            {"mp3","Audio"},{"wav","Audio"},{"flac","Audio"},{"aac","Audio"},
            {"ogg","Audio"},{"wma","Audio"},{"m4a","Audio"},{"opus","Audio"},
            {"aiff","Audio"},{"aif","Audio"},
            // Documents
            {"pdf","Documents"},{"doc","Documents"},{"docx","Documents"},
            {"xls","Documents"},{"xlsx","Documents"},{"ppt","Documents"},
            {"pptx","Documents"},{"txt","Documents"},{"rtf","Documents"},
            {"odt","Documents"},{"ods","Documents"},{"odp","Documents"},
            // Code
            {"cpp","Code"},{"c","Code"},{"h","Code"},{"hpp","Code"},
            {"py","Code"},{"js","Code"},{"jsx","Code"},{"ts","Code"},
            {"tsx","Code"},{"java","Code"},{"cs","Code"},{"go","Code"},
            {"rs","Code"},{"rb","Code"},{"php","Code"},{"html","Code"},
            {"htm","Code"},{"css","Code"},{"scss","Code"},{"sql","Code"},
            {"sh","Code"},{"bat","Code"},{"ps1","Code"},{"json","Code"},
            {"yaml","Code"},{"yml","Code"},{"toml","Code"},{"xml","Code"},
            {"md","Code"},{"qss","Code"},{"cmake","Code"},{"makefile","Code"},
            // Archives
            {"zip","Archives"},{"rar","Archives"},{"7z","Archives"},{"tar","Archives"},
            {"gz","Archives"},{"bz2","Archives"},{"xz","Archives"},{"iso","Archives"},
            {"dmg","Archives"},{"pkg","Archives"},
            // Executables
            {"exe","Executables"},{"msi","Executables"},{"dll","Executables"},
            {"sys","Executables"},{"so","Executables"},{"dylib","Executables"},
            {"app","Executables"},{"deb","Executables"},{"rpm","Executables"},
            {"apk","Executables"},{"elf","Executables"},
            // Fonts
            {"ttf","Fonts"},{"otf","Fonts"},{"woff","Fonts"},{"woff2","Fonts"},{"eot","Fonts"},
            // Data
            {"db","Data"},{"sqlite","Data"},{"sqlite3","Data"},{"parquet","Data"},
            {"csv","Data"},{"tsv","Data"},{"hdf5","Data"},{"h5","Data"},
            {"ini","Data"},{"cfg","Data"},{"conf","Data"},{"log","Data"}
        };

        return map.value(ext, "Other");
    }

private:
    CategoryMapper() = delete;
};

#endif // CATEGORYMAPPER_H
