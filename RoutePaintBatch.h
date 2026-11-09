// TSRE GenX route paint runner. GNU GPL v3 or later.
#pragma once
#include "RoutePaintPlan.h"
#include <QImage>
#include <functional>
class Route;
namespace RoutePaintBatch {
struct Result { int scanned = 0, saved = 0, skipped = 0; bool cancelled = false, refreshFailed = false; QString tile; };
using Progress = std::function<bool(int, int, const QString &)>;
int textureResolution(const QString &textureName,QString &error);
bool validateMirrorPreset(const QJsonObject &preset,QString &error);
bool run(Route *route, const RoutePaintPlan::Plan &plan, const QJsonArray &presets,
         const QVector<QImage> &shapes, const Progress &progress, Result &result, QString &error);
}
