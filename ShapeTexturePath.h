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

}
