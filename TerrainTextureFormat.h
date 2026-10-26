// TSRE GenX - Licensed under GNU GPL v3 or later. See LICENSE.md.
// Keep ACE and DDS output basenames distinct: Open Rails prefers a DDS over
// an ACE with the same basename even when the terrain names the ACE.
#pragma once
#include <QFileInfo>
#include <QFile>

namespace TerrainTextureFormat {
inline QString outputPath(const QString &source, bool dds) {
    const QFileInfo info(source);
    QString base = info.completeBaseName();
    if(dds) {
        while(base.endsWith("_ace",Qt::CaseInsensitive)) base.chop(4);
        if(!base.endsWith("_dds24",Qt::CaseInsensitive)) base += "_dds24";
        return info.absolutePath() + '/' + base + ".dds";
    }
    if(base.endsWith("_dds24",Qt::CaseInsensitive)) { base.chop(6); base += "_ace"; }
    else if(info.suffix().compare("dds",Qt::CaseInsensitive)==0
            && !base.endsWith("_ace",Qt::CaseInsensitive)) base += "_ace";
    while(QFile::exists(info.absolutePath() + '/' + base + ".dds")) base += "_ace";
    return info.absolutePath() + '/' + base + ".ace";
}
inline bool isPaintedPatchName(const QString &existing, const QString &normalAce) {
    QString base = QFileInfo(existing).completeBaseName();
    if(base.endsWith("_dds24",Qt::CaseInsensitive)) base.chop(6);
    while(base.endsWith("_ace",Qt::CaseInsensitive)) base.chop(4);
    const QString normal = QFileInfo(normalAce).completeBaseName();
    return base.compare(normal,Qt::CaseInsensitive)==0;
}
}
