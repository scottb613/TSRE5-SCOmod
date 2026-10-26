// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#ifndef ACTIVITYPATHID_H
#define ACTIVITYPATHID_H

#include <QRegularExpression>
#include <QString>

inline QString cleanActivityPathId(QString base)
{
    // Preserve Unicode spelling, including combining accents, so the PAT
    // filename and the IDs referenced by services and activities stay equal.
    static const QRegularExpression unsupported(
        QStringLiteral("[^\\p{L}\\p{M}\\p{N}_-]+"));
    return base.replace(unsupported, QStringLiteral("_"));
}

#endif
