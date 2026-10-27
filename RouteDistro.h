// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#pragma once
#include <QByteArray>
#include <QStringList>
#include <QVector>
#include <QMap>
#include <functional>

namespace RouteDistro {
using Progress = std::function<bool(double, const QString &)>;
struct Options {
    bool keepEditorAssets = true;
    bool consolidateTerrtex = true;
    bool includeEditorCaches = true;
};
struct Removal {
    QString path, reason, category;
    qint64 bytes = 0;
    QByteArray hash;
    // Consolidation rows represent a complete matching texture family.
    QStringList related, keepers;
    QVector<QByteArray> memberHashes;
};
struct Report {
    QString root, installationRoot;
    QString scannedAtUtc;
    Options options;
    QVector<Removal> removals;
    QStringList problems, warnings, notices;
    QByteArray signature;
    QMap<QString,QByteArray> referenceHashes;
    int files = 0, required = 0, duplicateGroups = 0;
    qint64 bytes = 0, removableBytes = 0;
    bool complete = false, cancelled = false;
};
struct Result {
    QStringList removed, rewritten, errors;
    qint64 bytes = 0;
    qint64 beforeBytes = 0, afterBytes = 0, netBytes = 0;
    QString measurementError, completedAtUtc;
    bool beforeMeasured = false, sizeMeasured = false;
    bool cancelled = false;
};
Report scan(const QString &root, const QString &installationRoot,
            Options options = {}, const Progress &progress = {});
Result clean(const Report &reviewed, const QStringList &selected,
             const Progress &progress = {});
QString summary(const Report &report);
// Shared by route cleanup and Copy To publication; aliases differ only in case
// must not allow two writers to operate on the same destination simultaneously.
QString operationLockPath(const QString &root);
bool inspectAssetReferences(const QString &path, QStringList &references, QString &error,
                            const Progress &progress = {});
QMap<QString,QStringList> warningGroups(const QStringList &warnings);
QString sizeSummary(const Result &result);
QString auditReport(const Report &report, const QStringList &selected,
                    const Result *result = nullptr);
}
