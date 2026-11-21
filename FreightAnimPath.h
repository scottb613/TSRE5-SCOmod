// TSRE GenX - freight animation shape and texture directories.
// Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#pragma once

#include <QDir>
#include <QString>

namespace FreightAnimPath {

struct Paths {
    QString shape;
    QString textures;
};

inline Paths resolve(QString wagonDirectory, QString shapeName) {
    wagonDirectory.replace('\\', '/');
    shapeName.replace('\\', '/');
    return {QDir::cleanPath(wagonDirectory + '/' + shapeName),
            QDir::cleanPath(wagonDirectory)};
}

inline Paths resolveLoad(QString trainsetDirectory, QString shapeName) {
    trainsetDirectory.replace('\\', '/');
    shapeName.replace('\\', '/');
    // OR's Container takes its reference path from the first component of Shape.
    const qsizetype separator = shapeName.indexOf('/');
    if(separator <= 0 || shapeName.contains(':'))
        return {};
    return {QDir::cleanPath(trainsetDirectory + '/' + shapeName),
            QDir::cleanPath(trainsetDirectory + '/' + shapeName.left(separator))};
}

}
