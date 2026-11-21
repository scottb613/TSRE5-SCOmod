// TSRE GenX - shape texture lookup and regression support.
// Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#pragma once

#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace ShapeTexturePath {

inline QString resolve(const QStringList &basePaths, QString fileName,
                       bool tryAceDdsAlternative = true) {
    fileName.replace('\\', '/');
    QStringList candidateNames;
    const QFileInfo requested(fileName);
    const QString suffix = requested.suffix().toLower();
    if (tryAceDdsAlternative && (suffix == "ace" || suffix == "dds")) {
        const QString stem = fileName.left(fileName.size() - 3);
        candidateNames << stem + "dds" << stem + "ace";
    } else {
        candidateNames << fileName;
    }

    // Search each seasonal/route directory before considering its fallback.
    // Within that directory DDS always wins over ACE, regardless of settings.
    for (const QString &basePath : basePaths) {
        for (const QString &candidateName : candidateNames) {
            const QString candidate = QDir::cleanPath(
                QDir(basePath).filePath(candidateName)).replace('\\', '/');
            if (QFileInfo::exists(candidate))
                return candidate;
        }
    }
    if (basePaths.isEmpty())
        return fileName;
    return QDir::cleanPath(QDir(basePaths.first()).filePath(fileName))
        .replace('\\', '/');
}

inline QString rollingStockSeason(int flags, QString season) {
    season = season.trimmed().toLower();
    if(season == "snow")
        season = "wintersnow";
    if(flags & 0x03) // Snow or SnowTrack takes precedence over other selectors.
        return season == "winter" || season.endsWith("snow") ? QString("snow") : QString();

    const QStringList selectors{"spring", "autumn", "winter", "springsnow",
                                "autumnsnow", "wintersnow"};
    const qsizetype index = selectors.indexOf(season);
    return index >= 0 && (flags & (0x04 << index)) != 0 ? season : QString();
}

inline QString resolveRollingStock(QString basePath, const QString &seasonSubdir,
                                   QString fileName) {
    basePath.replace('\\', '/');
    fileName.replace('\\', '/');
    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if((suffix != "ace" && suffix != "dds") || fileName.contains(':'))
        return {};

    const QString selectedRoot = seasonSubdir.isEmpty() ? basePath : basePath + '/' + seasonSubdir;
    const QString requested = QDir::cleanPath(selectedRoot + '/' + fileName);
    QString candidate = requested;
    const QStringList selectors{"autumn", "autumnsnow", "snow", "spring",
                                "springsnow", "winter", "wintersnow"};
    // Open Rails climbs only through named season directories, up to five attempts.
    for(int depth = 0; depth < 5; ++depth) {
        const QString stem = candidate.chopped(3);
        if(QFileInfo(stem + "dds").isFile())
            return stem + "dds";
        if(QFileInfo(stem + "ace").isFile())
            return stem + "ace";
        const QFileInfo file(candidate);
        const QFileInfo directory(file.path());
        if(!selectors.contains(directory.fileName(), Qt::CaseInsensitive))
            break;
        candidate = QDir(directory.path()).filePath(file.fileName());
    }
    return requested;
}

}
