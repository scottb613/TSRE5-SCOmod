/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "Flex.h"
#include <QtWidgets>
#include "Game.h"
#include <QDebug>
#include "TDB.h"
#include "Intersections.h"
#include "GLMatrix.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <vector>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

QWidget* Flex::window;
int Flex::windowInit = -1;
int Flex::offx = 0;
int Flex::offy = 0;
QPainter* Flex::painter;
QImage* Flex::img;
QLabel* Flex::myLabel;

namespace {

struct FlexVec2 {
    float x = 0.0f;
    float y = 0.0f;

    FlexVec2() {}
    FlexVec2(float px, float py) : x(px), y(py) {}
};

inline FlexVec2 add(FlexVec2 a, FlexVec2 b) {
    return {a.x + b.x, a.y + b.y};
}

inline FlexVec2 sub(FlexVec2 a, FlexVec2 b) {
    return {a.x - b.x, a.y - b.y};
}

inline FlexVec2 scale(FlexVec2 a, float s) {
    return {a.x * s, a.y * s};
}

inline float dot(FlexVec2 a, FlexVec2 b) {
    return a.x * b.x + a.y * b.y;
}

inline float length(FlexVec2 a) {
    return std::sqrt(dot(a, a));
}

inline float wrapPi(float a) {
    a = std::fmod(a + (float)M_PI, 2.0f * (float)M_PI);
    if (a < 0.0f)
        a += 2.0f * (float)M_PI;
    return a - (float)M_PI;
}

inline int signf(float v) {
    if (v > 0.0f) return 1;
    if (v < 0.0f) return -1;
    return 0;
}

inline FlexVec2 forward(float theta) {
    return {-std::sin(theta), std::cos(theta)};
}

inline FlexVec2 right(float theta) {
    return {std::cos(theta), std::sin(theta)};
}

inline FlexVec2 curveDisp(float angle, float radius) {
    float a = std::fabs(angle);
    if (a < 1e-6f || radius <= 0.0f)
        return {0.0f, 0.0f};
    int s = (angle >= 0.0f) ? 1 : -1;
    float dx = -((float)s) * radius * (1.0f - std::cos(a));
    float dy = radius * std::sin(a);
    return {dx, dy};
}

struct FlexPose2 {
    FlexVec2 pos;
    float heading = 0.0f; // dyntrack convention: + = left
};

inline void applyStraight(FlexPose2 &pose, float length) {
    if (length <= 0.0f)
        return;
    pose.pos = add(pose.pos, scale(forward(pose.heading), length));
}

inline void applyCurve(FlexPose2 &pose, float angle, float radius) {
    if (std::fabs(angle) < 1e-6f || radius <= 0.0f)
        return;
    FlexVec2 local = curveDisp(angle, radius);
    pose.pos = add(pose.pos, add(scale(right(pose.heading), local.x), scale(forward(pose.heading), local.y)));
    pose.heading = wrapPi(pose.heading + angle);
}

inline float sectionLength(int index, const float *sections10) {
    float a = sections10[index * 2 + 0];
    float r = sections10[index * 2 + 1];
    if ((index % 2) == 0)
        return std::max(0.0f, a);
    return (r > 0.0f) ? (std::fabs(a) * r) : 0.0f;
}

inline float totalCenterlineLength(const float *sections10) {
    float sum = 0.0f;
    for (int i = 0; i < 5; i++)
        sum += sectionLength(i, sections10);
    return sum;
}

inline void zeroFromSection(int startIndex, float *sections10) {
    for (int i = startIndex; i < 5; i++) {
        sections10[i * 2 + 0] = 0.0f;
        sections10[i * 2 + 1] = 0.0f;
    }
}

inline void trimToLength(float maxLen, float *sections10) {
    if (maxLen <= 0.0f) {
        for (int i = 0; i < 10; i++)
            sections10[i] = 0.0f;
        return;
    }
    float remaining = maxLen;
    for (int i = 0; i < 5; i++) {
        float a = sections10[i * 2 + 0];
        float r = sections10[i * 2 + 1];
        float len = 0.0f;
        if ((i % 2) == 0) {
            len = std::max(0.0f, a);
            if (len <= remaining + 1e-4f) {
                remaining -= len;
                continue;
            }
            sections10[i * 2 + 0] = std::max(0.0f, remaining);
            sections10[i * 2 + 1] = 0.0f;
            zeroFromSection(i + 1, sections10);
            return;
        }

        if (r <= 0.0f || std::fabs(a) < 1e-6f) {
            sections10[i * 2 + 0] = 0.0f;
            sections10[i * 2 + 1] = 0.0f;
            continue;
        }
        len = std::fabs(a) * r;
        if (len <= remaining + 1e-4f) {
            remaining -= len;
            continue;
        }
        float newAngle = (remaining <= 0.0f) ? 0.0f : ((a >= 0.0f ? 1.0f : -1.0f) * (remaining / r));
        sections10[i * 2 + 0] = newAngle;
        sections10[i * 2 + 1] = r;
        zeroFromSection(i + 1, sections10);
        return;
    }
}

inline int enabledSectionCount(const float *sections10) {
    int count = 0;
    for (int i = 0; i < 5; i++) {
        float a = sections10[i * 2 + 0];
        float r = sections10[i * 2 + 1];
        if ((i % 2) == 0) {
            if (a > 0.01f)
                count++;
            continue;
        }
        if (std::fabs(a) > 0.01f && r > 0.1f)
            count++;
    }
    return count;
}

inline void canonicalize(float *sections10) {
    auto disableCurve = [&](int idx) {
        sections10[idx * 2 + 0] = 0.0f;
        sections10[idx * 2 + 1] = 0.0f;
    };

    if (std::fabs(sections10[2]) < 1e-4f || sections10[3] <= 0.1f)
        disableCurve(1);
    if (std::fabs(sections10[6]) < 1e-4f || sections10[7] <= 0.1f)
        disableCurve(3);

    if (sections10[0] < 0.0f) sections10[0] = 0.0f;
    if (sections10[4] < 0.0f) sections10[4] = 0.0f;
    if (sections10[8] < 0.0f) sections10[8] = 0.0f;

    // Merge straights if an intermediate curve is disabled.
    bool curve1Disabled = (std::fabs(sections10[2]) < 1e-6f || sections10[3] <= 0.1f);
    bool curve2Disabled = (std::fabs(sections10[6]) < 1e-6f || sections10[7] <= 0.1f);

    if (curve1Disabled) {
        sections10[0] += sections10[4];
        sections10[4] = 0.0f;
    }
    if (curve2Disabled) {
        sections10[4] += sections10[8];
        sections10[8] = 0.0f;
    }
    if (curve1Disabled && curve2Disabled) {
        sections10[0] += sections10[4];
        sections10[4] = 0.0f;
    }

    if (sections10[0] < 0.01f) sections10[0] = 0.0f;
    if (sections10[4] < 0.01f) sections10[4] = 0.0f;
    if (sections10[8] < 0.01f) sections10[8] = 0.0f;

    if (std::fabs(sections10[2]) < 0.01f) disableCurve(1);
    if (std::fabs(sections10[6]) < 0.01f) disableCurve(3);
}

inline FlexPose2 simulate(const float *sections10) {
    FlexPose2 pose;
    pose.pos = {0.0f, 0.0f};
    pose.heading = 0.0f;
    applyStraight(pose, sections10[0]);
    applyCurve(pose, sections10[2], sections10[3]);
    applyStraight(pose, sections10[4]);
    applyCurve(pose, sections10[6], sections10[7]);
    applyStraight(pose, sections10[8]);
    return pose;
}

inline void samplePath(const float *sections10, std::vector<FlexVec2> &outPoints) {
    outPoints.clear();
    FlexPose2 pose;
    pose.pos = {0.0f, 0.0f};
    pose.heading = 0.0f;
    outPoints.push_back(pose.pos);

    auto pushIfFar = [&](FlexVec2 p) {
        if (outPoints.empty()) {
            outPoints.push_back(p);
            return;
        }
        FlexVec2 last = outPoints.back();
        if (std::fabs(p.x - last.x) > 1e-4f || std::fabs(p.y - last.y) > 1e-4f)
            outPoints.push_back(p);
    };

    auto sampleStraight = [&](float len) {
        if (len <= 0.0f)
            return;
        applyStraight(pose, len);
        pushIfFar(pose.pos);
    };

    auto sampleCurve = [&](float angle, float radius) {
        float a = std::fabs(angle);
        if (a < 1e-6f || radius <= 0.0f)
            return;
        float maxStep = 0.05f; // rad
        int steps = std::max(1, (int)std::ceil(a / maxStep));
        float da = angle / (float)steps;
        for (int i = 0; i < steps; i++) {
            applyCurve(pose, da, radius);
            pushIfFar(pose.pos);
        }
    };

    sampleStraight(sections10[0]);
    sampleCurve(sections10[2], sections10[3]);
    sampleStraight(sections10[4]);
    sampleCurve(sections10[6], sections10[7]);
    sampleStraight(sections10[8]);
}

inline bool hasSelfIntersection(const float *sections10) {
    std::vector<FlexVec2> pts;
    samplePath(sections10, pts);
    if (pts.size() < 4)
        return false;
    for (size_t i = 0; i + 1 < pts.size(); i++) {
        float x0 = pts[i].x;
        float y0 = pts[i].y;
        float x1 = pts[i + 1].x;
        float y1 = pts[i + 1].y;
        for (size_t j = i + 2; j + 1 < pts.size(); j++) {
            if (j == i + 1)
                continue;
            float x2 = pts[j].x;
            float y2 = pts[j].y;
            float x3 = pts[j + 1].x;
            float y3 = pts[j + 1].y;
            float ix = 0.0f;
            float iy = 0.0f;
            if (Intersections::segmentIntersection(x0, y0, x1, y1, x2, y2, x3, y3, ix, iy))
                return true;
        }
    }
    return false;
}

inline bool makesOnlyForwardProgress(const float *sections10,
        FlexVec2 targetPos) {
    const float chordLength = length(targetPos);
    if(chordLength <= 0.001f)
        return false;

    const FlexVec2 chordDirection = scale(targetPos, 1.0f / chordLength);
    std::vector<FlexVec2> points;
    samplePath(sections10, points);
    for(size_t i = 0; i + 1 < points.size(); ++i){
        const FlexVec2 step = sub(points[i + 1], points[i]);
        // A direct flex may run across the chord, but it must never turn back
        // toward its source.  This rejects U-shaped solutions which happen
        // to land on the requested endpoint with the right final tangent.
        if(dot(step, chordDirection) < -0.001f)
            return false;
    }
    return true;
}

struct FlexCandidate {
    float sections[10] = {0};
    float minRadius = 0.0f;
    float trimmedLen = 0.0f;
    float rawLen = 0.0f;
    int enabledCount = 0;
    int curveCount = 0;
    float endStraightSum = 0.0f;
    bool selfIntersect = false;
    bool initialWrongWay = false;
    bool wasTrimmed = false;
    bool meetsPreferredMin = false;
};

inline float firstEnabledCurveAngle(const float *sections10) {
    if (std::fabs(sections10[2]) > 1e-6f && sections10[3] > 0.1f)
        return sections10[2];
    if (std::fabs(sections10[6]) > 1e-6f && sections10[7] > 0.1f)
        return sections10[6];
    return 0.0f;
}

inline float candidateMinRadius(const float *sections10) {
    float r1 = (std::fabs(sections10[2]) > 0.01f) ? sections10[3] : std::numeric_limits<float>::infinity();
    float r2 = (std::fabs(sections10[6]) > 0.01f) ? sections10[7] : std::numeric_limits<float>::infinity();
    float r = std::min(r1, r2);
    if (!std::isfinite(r))
        return std::numeric_limits<float>::infinity();
    return r;
}

inline int enabledCurveCount(const float *sections10) {
    int count = 0;
    if (std::fabs(sections10[2]) > 0.01f && sections10[3] > 0.1f)
        count++;
    if (std::fabs(sections10[6]) > 0.01f && sections10[7] > 0.1f)
        count++;
    return count;
}

inline bool betterCandidate(const FlexCandidate &a, const FlexCandidate &b) {
    if (!b.rawLen && !b.trimmedLen && b.enabledCount == 0)
        return true;
    if (a.selfIntersect != b.selfIntersect)
        return b.selfIntersect; // prefer non-intersecting
    if (a.wasTrimmed != b.wasTrimmed)
        return b.wasTrimmed; // prefer solutions that don't exceed the 2048m limit

    if (a.meetsPreferredMin != b.meetsPreferredMin)
        return a.meetsPreferredMin;

    // A large radius is useful only when it does not send the track on a
    // substantial detour.  Previously radius was the first objective, so a
    // near-180 degree, 150m-radius loop could beat a much more direct flex.
    if (a.initialWrongWay != b.initialWrongWay)
        return b.initialWrongWay;

    float shorterLen = std::min(a.trimmedLen, b.trimmedLen);
    float materialDetour = std::max(25.0f, 0.10f * shorterLen);
    if (std::fabs(a.trimmedLen - b.trimmedLen) > materialDetour)
        return a.trimmedLen < b.trimmedLen;

    // Within the same general route length, maximize the minimum curve radius.
    auto radiusValue = [](float r) -> float {
        if (std::isfinite(r))
            return r;
        if (r > 0.0f)
            return 1e30f; // treat +inf as "very large"
        return -1e30f;    // treat NaN/-inf as "very small"
    };
    float ra = radiusValue(a.minRadius);
    float rb = radiusValue(b.minRadius);
    float rmax = std::max(ra, rb);
    float rTol = std::max(0.5f, 0.02f * rmax); // tie-break window: 0.5m or 2%
    if (std::fabs(ra - rb) > rTol)
        return ra > rb;

    // Prefer simpler geometry if still tied.
    if (a.curveCount != b.curveCount)
        return a.curveCount < b.curveCount;
    if (a.enabledCount != b.enabledCount)
        return a.enabledCount < b.enabledCount;

    if (std::fabs(a.trimmedLen - b.trimmedLen) > 0.05f)
        return a.trimmedLen < b.trimmedLen;
    if (std::fabs(a.endStraightSum - b.endStraightSum) > 0.05f)
        return a.endStraightSum < b.endStraightSum;
    return false;
}

inline bool validatePose(const float *sections10, FlexVec2 targetPos, float targetHeading) {
    FlexPose2 end = simulate(sections10);
    float posErr = length(sub(end.pos, targetPos));
    float angErr = std::fabs(wrapPi(end.heading - targetHeading));
    // A visibly plausible candidate is not sufficient for rail geometry.
    // Reject anything that cannot form a genuinely solid positional and
    // tangent connection at both ends.
    return posErr < 0.002f && angErr < 0.0001f;
}

inline void flipCurveAngleSignsForDyntrack(float *sections10) {
    // Convention boundary:
    // `Flex::NewFlex(...)` solves in Flex's legacy 2D mapping (XZ -> XY with Z flipped) to stay consistent with TDB yaw.
    // Dyntrack sections are later consumed in the engine's OpenGL/XZ frame, where the Z-mirror reverses turn direction.
    // Convert by negating curve angles (lengths/radii stay the same).
    sections10[2] = -sections10[2];
    sections10[6] = -sections10[6];
}

inline QJsonValue jsonNumberOrNull(double v) {
    if (std::isfinite(v))
        return v;
    return QJsonValue();
}

inline QJsonArray jsonFloatArray(const float *v, int count) {
    QJsonArray arr;
    for (int i = 0; i < count; i++)
        arr.append(jsonNumberOrNull(v[i]));
    return arr;
}

inline QJsonArray jsonVec2(FlexVec2 v) {
    QJsonArray arr;
    arr.append(jsonNumberOrNull(v.x));
    arr.append(jsonNumberOrNull(v.y));
    return arr;
}

class FlexJsonlLogger {
public:
    static FlexJsonlLogger &instance() {
        static FlexJsonlLogger inst;
        return inst;
    }

    bool enabled() const {
        return Game::flexLogEnabled;
    }

    bool includeCandidates() const {
        return Game::flexLogEnabled && Game::flexLogCandidates;
    }

    int nextCaseId() {
        return ++caseId_;
    }

    void logObject(const QJsonObject &obj) {
        if (!enabled())
            return;
        if (!ensureOpen())
            return;
        const QByteArray line = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        out_ << line << "\n";
        out_.flush();
        file_.flush();
    }

private:
    bool ensureOpen() {
        if (file_.isOpen())
            return true;

        QString path = Game::flexLogFile;
        if (path.isEmpty()) {
            const QString dir = "features/tests/captures";
            QDir().mkpath(dir);
            const QString stamp = QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
            path = dir + QString("/flex-capture-%1.jsonl").arg(stamp);
            Game::flexLogFile = path;
            qDebug() << "Flex JSONL capture:" << path;
        } else {
            const QFileInfo fi(path);
            fi.absoluteDir().mkpath(".");
        }

        file_.setFileName(path);
        if (!file_.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            qWarning() << "Flex JSONL capture: failed to open" << path;
            Game::flexLogEnabled = false;
            return false;
        }
        out_.setDevice(&file_);
        return true;
    }

    std::atomic<int> caseId_{0};
    QFile file_;
    QTextStream out_;
};

} // namespace

bool Flex::AutoFlex(int x1, int z1, float* p1, int x2, int z2, float* p2,
        float* dyntrackSections, float &visualElev, float &averageElev,
        float preferredMinCurveRadius, const float *sourceQ,
        const float *destinationQ, float *resolvedSourceYaw){
    TDB* tdb = Game::trackDB;
    visualElev = 0.0f;
    averageElev = 0.0f;
    if(dyntrackSections == NULL || p1 == NULL || p2 == NULL){
        qWarning() << "Auto-Flex rejected: missing connection geometry";
        return false;
    }
    std::fill(dyntrackSections, dyntrackSections + 10, 0.0f);

    auto finiteValues = [](const float *values, int count) {
        if(values == NULL)
            return false;
        for(int i = 0; i < count; ++i){
            if(!std::isfinite(values[i]))
                return false;
        }
        return true;
    };
    if(!finiteValues(p1, 3) || !finiteValues(p2, 3)
            || (sourceQ != NULL && !finiteValues(sourceQ, 4))
            || (destinationQ != NULL && !finiteValues(destinationQ, 4))){
        qWarning() << "Auto-Flex rejected: non-finite connection geometry";
        return false;
    }
    if(tdb == NULL && (sourceQ == NULL || destinationQ == NULL)){
        qWarning() << "Auto-Flex rejected: track database is unavailable";
        return false;
    }

    const double tileDeltaX = static_cast<double>(x2)
            - static_cast<double>(x1);
    const double tileDeltaZ = static_cast<double>(z2)
            - static_cast<double>(z1);
    const double worldDx = tileDeltaX * 2048.0
            + static_cast<double>(p2[0]) - p1[0];
    const double worldDy = static_cast<double>(p2[1]) - p1[1];
    const double worldDz = tileDeltaZ * 2048.0
            + static_cast<double>(p2[2]) - p1[2];
    const double chordLength = std::sqrt(worldDx * worldDx
            + worldDy * worldDy + worldDz * worldDz);
    const double minimumConnectionLength = std::max(
            0.001, static_cast<double>(Game::trackGap));
    constexpr double maximumConnectionLength = 2048.0;
    if(!std::isfinite(chordLength)
            || chordLength < minimumConnectionLength){
        qWarning() << "Auto-Flex rejected: source and destination"
                << "collapse into the same track connection";
        return false;
    }
    if(chordLength > maximumConnectionLength + 0.001){
        qWarning() << "Auto-Flex rejected: connection exceeds the"
                << "2048 metre dynamic-track limit";
        return false;
    }

    qDebug() <<"flex "<< x1 << " " << z1 << " " << p1[0] << " " << p1[1] << " " << p1[2];
    qDebug() <<"flex "<< x2 << " " << z2 << " " << p2[0] << " " << p2[1] << " " << p2[2];

    float sourceAxis[4] = {0,0,0,1};
    float destinationAxis[4] = {0,0,0,1};

    if(sourceQ != NULL)
        Quat::copy(sourceAxis, const_cast<float*>(sourceQ));
    else {
        const int sourceNode = tdb->findNearestNode(x1, z1, p1, sourceAxis);
        if(sourceNode < 0)
            return false;
    }

    if(destinationQ != NULL)
        Quat::copy(destinationAxis, const_cast<float*>(destinationQ));
    else {
        const int destinationNode = tdb->findNearestNode(
                x2, z2, p2, destinationAxis);
        if(destinationNode < 0){
            qWarning() << "Auto-Flex rejected: no destination track endpoint";
            return false;
        }
    }

    float destinationWorld[3] = {
        static_cast<float>(p2[0] + 2048.0 * tileDeltaX),
        p2[1],
        static_cast<float>(p2[2] + 2048.0 * tileDeltaZ)
    };
    const float rise = destinationWorld[1] - p1[1];
    const float deltaX = destinationWorld[0] - p1[0];
    const float deltaZ = -(destinationWorld[2] - p1[2]);

    struct SolvedCandidate {
        bool valid = false;
        float sections[10] = {0};
        float sourceYaw = 0.0f;
        float visualGrade = 0.0f;
        float averageGrade = 0.0f;
        float length = std::numeric_limits<float>::max();
    } best;

    // Track tangents are axes, not arrows.  Try both directions at both
    // connections and choose the shortest fully validated centerline.  This
    // makes A->B and B->A the same geometric problem instead of relying on
    // TDB traversal order.
    for(int sourceDirection = 0; sourceDirection < 2; ++sourceDirection){
        const float sourceYaw = wrapPi(sourceAxis[1]
                + sourceDirection * (float)M_PI);
        const float forwardX = std::sin(sourceYaw);
        const float forwardZ = std::cos(sourceYaw);
        const float rightX = std::cos(sourceYaw);
        const float rightZ = -std::sin(sourceYaw);
        const float forwardDistance = deltaX * forwardX + deltaZ * forwardZ;
        const float lateralDistance = deltaX * rightX + deltaZ * rightZ;
        if(forwardDistance <= 0.001f)
            continue;
        const float unpitchedForward = std::copysign(
                std::sqrt(forwardDistance * forwardDistance + rise * rise),
                std::fabs(forwardDistance) > 0.001f ? forwardDistance : 1.0f);

        const float adjustedDeltaX = rightX * lateralDistance
                + forwardX * unpitchedForward;
        const float adjustedDeltaZ = rightZ * lateralDistance
                + forwardZ * unpitchedForward;
        float adjustedP2[3] = {
            static_cast<float>(p1[0] + adjustedDeltaX
                    - 2048.0 * tileDeltaX),
            p2[1],
            static_cast<float>(-(-p1[2] + adjustedDeltaZ)
                    - 2048.0 * tileDeltaZ)
        };

        for(int destinationDirection = 0;
                destinationDirection < 2; ++destinationDirection){
            float q1[4] = {0, sourceYaw, 0, 1};
            const float destinationYaw = wrapPi(destinationAxis[1]
                    + destinationDirection * (float)M_PI);
            const float destinationForward = deltaX
                    * std::sin(destinationYaw)
                    + deltaZ * std::cos(destinationYaw);
            if(destinationForward <= 0.001f)
                continue;
            float q2[4] = {0, destinationYaw, 0, 1};
            float candidateSections[10] = {0};
            if(!Flex::NewFlex(x1, z1, p1, q1, x2, z2, adjustedP2, q2,
                    candidateSections, preferredMinCurveRadius))
                continue;

            // NewFlex returns angles in dyntrack convention.  Convert a copy
            // back to solver convention so the exact endpoint can be checked
            // after applying the proposed source-plane pitch.
            float solverSections[10];
            std::copy(candidateSections, candidateSections + 10,
                    solverSections);
            flipCurveAngleSignsForDyntrack(solverSections);
            const FlexPose2 flatEnd = simulate(solverSections);
            if(std::fabs(flatEnd.pos.y) <= 0.001f)
                continue;

            const float pitchSin = rise / flatEnd.pos.y;
            if(!std::isfinite(pitchSin) || std::fabs(pitchSin) > 1.0f)
                continue;
            const float pitchCos = std::sqrt(qMax(0.0f,
                    1.0f - pitchSin * pitchSin));
            const float lateralError = flatEnd.pos.x - lateralDistance;
            const float forwardError =
                    flatEnd.pos.y * pitchCos - forwardDistance;
            const float verticalError = flatEnd.pos.y * pitchSin - rise;
            const float endpointError = std::sqrt(
                    lateralError * lateralError
                    + forwardError * forwardError
                    + verticalError * verticalError);
            if(endpointError >= 0.002f)
                continue;

            const float railLength =
                    totalCenterlineLength(candidateSections);
            if(railLength <= 0.001f || railLength >= best.length)
                continue;

            best.valid = true;
            std::copy(candidateSections, candidateSections + 10,
                    best.sections);
            best.sourceYaw = sourceYaw;
            best.visualGrade = pitchSin * 1000.0f;
            best.averageGrade = rise * (1000.0f / railLength);
            best.length = railLength;
        }
    }

    if(!best.valid){
        qWarning() << "Auto-Flex rejected: no direction produced solid"
                << "position, tangent, and grade connections";
        return false;
    }

    std::copy(best.sections, best.sections + 10, dyntrackSections);
    visualElev = best.visualGrade;
    averageElev = best.averageGrade;
    if(resolvedSourceYaw != NULL)
        *resolvedSourceYaw = best.sourceYaw;
    qDebug() << "elev" << best.length << p2[1] << p1[1]
            << "source yaw" << best.sourceYaw << "success" << true;
    return true;
}

bool Flex::NewFlex(int x1, int z1, float *p1, float *q1, int x2, int z2, float *p2, float *q2, float * dyntrackSections, float preferredMinCurveRadius){
    for (int i = 0; i < 10; i++)
        dyntrackSections[i] = 0.0f;

    const bool logEnabled = FlexJsonlLogger::instance().enabled();
    const bool logCandidates = FlexJsonlLogger::instance().includeCandidates();
    const int flexCaseId = (logEnabled || logCandidates) ? FlexJsonlLogger::instance().nextCaseId() : -1;

    if (preferredMinCurveRadius < 0.0f)
        preferredMinCurveRadius = 0.0f;

    if (Game::gui && Game::flexDebugWindow) {
        if (windowInit == 0) {
            window = new QWidget();
            windowInit = 1;
            window->setFixedSize(800, 800);
            window->show();
            img = new QImage(800, 800, QImage::Format_RGBA8888);
            painter = new QPainter();
            myLabel = new QLabel("");
            myLabel->setPixmap(QPixmap::fromImage(*img));
            QVBoxLayout *mainLayout = new QVBoxLayout;
            mainLayout->addWidget(myLabel);
            window->setLayout(mainLayout);
            window->show();
        }
        if (img != nullptr)
            img->fill(0);
    }

    QPen niebieski(QColor(50,50,255));
    QPen czerwony(QColor(255,50,50));

    // Build world-space 2D positions in Flex's legacy convention (XZ -> XY with Z flipped).
    // This matches how the old (working) Flex solver visualized nodes and is consistent with TDB yaw.
    FlexVec2 P0 = {p1[0], -p1[2]};
    FlexVec2 P1 = {p2[0] + 2048.0f * (x2 - x1), -p2[2] - 2048.0f * (z2 - z1)};

    float yaw0 = q1[1];
    float yaw1 = q2[1];

    // Visualize the raw tangent directions (from TDB) first.
    FlexVec2 v0Draw = {std::sin(yaw0), std::cos(yaw0)};
    FlexVec2 v1Draw = {std::sin(yaw1), std::cos(yaw1)};

    // Empirical fix: dyntrack local frame appears to be 180° rotated at the start.
    // Flip the *start* direction for calculations only (the end direction stays as provided).
    float yaw0Calc = yaw0;//wrapPi(yaw0 + (float)M_PI);
    float yaw1Calc = yaw1;

    FlexVec2 v0 = {std::sin(yaw0Calc), std::cos(yaw0Calc)};

    offx = (int)((P0.x + P1.x) * 0.5f);
    offy = (int)((P0.y + P1.y) * 0.5f);

    drawLine(niebieski, (int)P0.x, (int)P0.y, (int)(P0.x + v0Draw.x * 1000.0f), (int)(P0.y + v0Draw.y * 1000.0f));
    drawLine(niebieski, (int)P1.x, (int)P1.y, (int)(P1.x + v1Draw.x * 1000.0f), (int)(P1.y + v1Draw.y * 1000.0f));

    // Transform into start-local frame (start at origin, start heading = +Y).
    FlexVec2 deltaW = sub(P1, P0);
    FlexVec2 f0 = v0;
    FlexVec2 r0 = {std::cos(yaw0), -std::sin(yaw0)};
    FlexVec2 targetPos = {dot(deltaW, r0), dot(deltaW, f0)};

    float yawRel = wrapPi(yaw1Calc - yaw0Calc);   // + = right
    float phi = wrapPi(-yawRel);                  // + = left (dyntrack angle convention)

    auto logFlexCase = [&](bool success) {
        if (!logEnabled)
            return;
        QJsonObject obj;
        obj["type"] = "flex_case";
        obj["id"] = flexCaseId;
        obj["x1"] = x1;
        obj["z1"] = z1;
        obj["p1"] = jsonFloatArray(p1, 3);
        obj["q1"] = jsonFloatArray(q1, 4);
        obj["x2"] = x2;
        obj["z2"] = z2;
        obj["p2"] = jsonFloatArray(p2, 3);
        obj["q2"] = jsonFloatArray(q2, 4);
        obj["preferredMinCurveRadius"] = jsonNumberOrNull(preferredMinCurveRadius);
        obj["P0"] = jsonVec2(P0);
        obj["P1"] = jsonVec2(P1);
        obj["yaw0"] = jsonNumberOrNull(yaw0);
        obj["yaw1"] = jsonNumberOrNull(yaw1);
        obj["targetPos"] = jsonVec2(targetPos);
        obj["phi"] = jsonNumberOrNull(phi);
        obj["success"] = success;
        if (success)
            obj["sections"] = jsonFloatArray(dyntrackSections, 10);
        FlexJsonlLogger::instance().logObject(obj);
    };

    auto logFlexCandidate = [&](const QString &kind, const float *sectionsSolver10, const FlexCandidate &cand, bool bestSoFar) {
        if (!logCandidates)
            return;
        float sectionsDyntrack10[10];
        std::copy(sectionsSolver10, sectionsSolver10 + 10, sectionsDyntrack10);
        flipCurveAngleSignsForDyntrack(sectionsDyntrack10);

        QJsonObject obj;
        obj["type"] = "flex_candidate";
        obj["caseId"] = flexCaseId;
        obj["kind"] = kind;
        obj["sectionsSolver"] = jsonFloatArray(sectionsSolver10, 10);
        obj["sectionsDyntrack"] = jsonFloatArray(sectionsDyntrack10, 10);
        obj["rawLen"] = jsonNumberOrNull(cand.rawLen);
        obj["trimmedLen"] = jsonNumberOrNull(cand.trimmedLen);
        obj["minRadius"] = jsonNumberOrNull(cand.minRadius);
        obj["enabledCount"] = cand.enabledCount;
        obj["curveCount"] = cand.curveCount;
        obj["endStraightSum"] = jsonNumberOrNull(cand.endStraightSum);
        obj["selfIntersect"] = cand.selfIntersect;
        obj["initialWrongWay"] = cand.initialWrongWay;
        obj["wasTrimmed"] = cand.wasTrimmed;
        obj["meetsPreferredMin"] = cand.meetsPreferredMin;
        obj["bestSoFar"] = bestSoFar;
        FlexJsonlLogger::instance().logObject(obj);
    };

    // Fast path: perfectly straight if end is on the start ray and headings match.
    if (std::fabs(phi) < 1e-4f && std::fabs(targetPos.x) < 0.05f && targetPos.y > 0.0f) {
        dyntrackSections[0] = targetPos.y;
        canonicalize(dyntrackSections);
        trimToLength(2048.0f, dyntrackSections);
        canonicalize(dyntrackSections);
        if (!validatePose(dyntrackSections, targetPos, phi)) {
            logFlexCase(false);
            return false;
        }
        if (Game::gui && myLabel != nullptr && img != nullptr)
            myLabel->setPixmap(QPixmap::fromImage(*img));

        if (logCandidates) {
            FlexCandidate cand;
            std::copy(dyntrackSections, dyntrackSections + 10, cand.sections);
            cand.rawLen = totalCenterlineLength(dyntrackSections);
            cand.trimmedLen = cand.rawLen;
            cand.minRadius = candidateMinRadius(dyntrackSections);
            cand.enabledCount = enabledSectionCount(dyntrackSections);
            cand.curveCount = enabledCurveCount(dyntrackSections);
            cand.endStraightSum = dyntrackSections[0] + dyntrackSections[8];
            cand.selfIntersect = false;
            cand.initialWrongWay = false;
            cand.wasTrimmed = false;
            cand.meetsPreferredMin = (preferredMinCurveRadius > 0.0f) ? (cand.minRadius >= preferredMinCurveRadius - 1e-3f) : false;
            logFlexCandidate(QStringLiteral("straight"), dyntrackSections, cand, true);
        }

        flipCurveAngleSignsForDyntrack(dyntrackSections);
        logFlexCase(true);
        return true;
    }

    // Candidate radii (meters), descending preference.
    std::vector<float> radii = {10000.0f, 8000.0f, 5000.0f, 2000.0f, 1200.0f, 800.0f, 500.0f, 300.0f, 200.0f, 150.0f, 100.0f, 75.0f, 50.0f, 30.0f, 20.0f, 15.0f, 10.0f};
    float minAllowedRadius = 5.0f;

    // Best candidate across all search phases.
    FlexCandidate best;
    bool found = false;

    std::vector<float> endStraightOptions = {0.0f, 1.0f, 2.0f, 5.0f, 10.0f, 20.0f, 50.0f, 100.0f, 200.0f};

    auto tryCandidate = [&](const float *rawSections10, const QString &kind) {
        float tmp[10];
        std::copy(rawSections10, rawSections10 + 10, tmp);
        canonicalize(tmp);
        if (!validatePose(tmp, targetPos, phi))
            return;
        if (!makesOnlyForwardProgress(tmp, targetPos))
            return;

        float trimmed[10];
        std::copy(tmp, tmp + 10, trimmed);
        trimToLength(2048.0f, trimmed);
        canonicalize(trimmed);
        if (!validatePose(trimmed, targetPos, phi))
            return;

        FlexCandidate cand;
        std::copy(trimmed, trimmed + 10, cand.sections);
        cand.rawLen = totalCenterlineLength(tmp);
        cand.trimmedLen = totalCenterlineLength(trimmed);
        cand.minRadius = candidateMinRadius(trimmed);
        cand.enabledCount = enabledSectionCount(trimmed);
        cand.curveCount = enabledCurveCount(trimmed);
        cand.endStraightSum = trimmed[0] + trimmed[8];
        cand.selfIntersect = hasSelfIntersection(trimmed);
        float firstCurveAngle = firstEnabledCurveAngle(trimmed);
        cand.initialWrongWay = (targetPos.y > 0.0f && std::fabs(targetPos.x) > 0.2f && (firstCurveAngle * targetPos.x) > 0.0f);
        cand.wasTrimmed = (cand.rawLen > cand.trimmedLen + 0.05f);
        cand.meetsPreferredMin = (preferredMinCurveRadius > 0.0f) ? (cand.minRadius >= preferredMinCurveRadius - 1e-3f) : false;

        bool bestSoFar = (!found || betterCandidate(cand, best));
        logFlexCandidate(kind, trimmed, cand, bestSoFar);

        if (bestSoFar) {
            best = cand;
            found = true;
        }
    };

    // 1) Single curve (L + C + L) candidates over our discrete radius set.
    if (std::fabs(std::sin(phi)) > 1e-4f) {
        for (float R : radii) {
            if (R < minAllowedRadius)
                continue;

            FlexVec2 cd = curveDisp(phi, R);
            float denom = -std::sin(phi);
            if (std::fabs(denom) < 1e-6f)
                continue;

            float L2 = (targetPos.x - cd.x) / denom;
            float L0 = targetPos.y - cd.y - std::cos(phi) * L2;
            if (L0 < -0.05f || L2 < -0.05f)
                continue;
            if (L0 < 0.0f) L0 = 0.0f;
            if (L2 < 0.0f) L2 = 0.0f;

            float raw[10] = {0};
            raw[0] = L0;
            raw[2] = phi;
            raw[3] = R;
            raw[4] = L2;
            tryCandidate(raw, QStringLiteral("single_curve"));
        }
    }

    auto solveCLC = [&](FlexVec2 target, float R1, float R2, float startStraight, float endStraight) {
        float angleMax = (float)M_PI;
        float alphaStep = (float)(0.5 * M_PI / 180.0); // 0.5 deg

        auto f = [&](float alpha) -> float {
            float beta = phi - alpha;
            if (std::fabs(beta) > angleMax + 1e-5f)
                return std::numeric_limits<float>::quiet_NaN();
            FlexVec2 pos1 = curveDisp(alpha, R1);

            FlexVec2 d2local = curveDisp(beta, R2);
            FlexVec2 d2 = add(scale(right(alpha), d2local.x), scale(forward(alpha), d2local.y));

            FlexVec2 start2 = sub(target, d2);
            FlexVec2 v = sub(start2, pos1);
            return dot(v, right(alpha)); // lateral mismatch
        };

        float prevAlpha = -angleMax;
        float prevF = f(prevAlpha);
        bool prevValid = std::isfinite(prevF);
        for (float alpha = -angleMax + alphaStep; alpha <= angleMax + 1e-6f; alpha += alphaStep) {
            float curF = f(alpha);
            bool curValid = std::isfinite(curF);

            auto testRoot = [&](float rootAlpha) {
                float beta = phi - rootAlpha;
                if (std::fabs(beta) > angleMax + 1e-5f)
                    return;
                FlexVec2 pos1 = curveDisp(rootAlpha, R1);
                FlexVec2 d2local = curveDisp(beta, R2);
                FlexVec2 d2 = add(scale(right(rootAlpha), d2local.x), scale(forward(rootAlpha), d2local.y));
                FlexVec2 start2 = sub(target, d2);
                FlexVec2 v = sub(start2, pos1);
                float d = dot(v, forward(rootAlpha));
                if (d < -0.05f)
                    return;
                if (d < 0.0f) d = 0.0f;

                float raw[10] = {0};
                raw[0] = startStraight;
                raw[2] = rootAlpha;
                raw[3] = R1;
                raw[4] = d;
                raw[6] = beta;
                raw[7] = R2;
                raw[8] = endStraight;
                tryCandidate(raw, QStringLiteral("clc"));
            };

            if (curValid && std::fabs(curF) < 0.05f) {
                testRoot(alpha);
            }

            if (prevValid && curValid && (prevF * curF) < 0.0f) {
                float lo = prevAlpha;
                float hi = alpha;
                float flo = prevF;
                for (int it = 0; it < 25; it++) {
                    float mid = (lo + hi) * 0.5f;
                    float fmid = f(mid);
                    if (!std::isfinite(fmid))
                        break;
                    if ((flo * fmid) <= 0.0f) {
                        hi = mid;
                        curF = fmid;
                    } else {
                        lo = mid;
                        flo = fmid;
                    }
                }
                testRoot((lo + hi) * 0.5f);
            }

            prevAlpha = alpha;
            prevF = curF;
            prevValid = curValid;
        }
    };

    // First: try without end straights, equal radii.
    for (float R : radii) {
        if (R < minAllowedRadius)
            continue;
        solveCLC(targetPos, R, R, 0.0f, 0.0f);
    }

    // Then: full grid, still without end straights.
    for (float R1 : radii) {
        if (R1 < minAllowedRadius)
            continue;
        for (float R2 : radii) {
            if (R2 < minAllowedRadius)
                continue;
            solveCLC(targetPos, R1, R2, 0.0f, 0.0f);
        }
    }

    // Fallback: allow end straights (small discrete set), cost-penalized via scoring.
    // Also try this if the best "no-end-straights" solution self-intersects.
    if (!found || best.selfIntersect) {
        for (float L0 : endStraightOptions) {
            for (float L2 : endStraightOptions) {
                if (L0 == 0.0f && L2 == 0.0f)
                    continue;
                FlexVec2 targetShift = targetPos;
                targetShift.y -= L0;
                targetShift = sub(targetShift, scale(forward(phi), L2));

                for (float R1 : radii) {
                    if (R1 < minAllowedRadius)
                        continue;
                    for (float R2 : radii) {
                        if (R2 < minAllowedRadius)
                            continue;
                        solveCLC(targetShift, R1, R2, L0, L2);
                    }
                }
            }
        }
    }

    if (!found) {
        if (Game::gui && myLabel != nullptr && img != nullptr)
            myLabel->setPixmap(QPixmap::fromImage(*img));
        logFlexCase(false);
        return false;
    }

    std::copy(best.sections, best.sections + 10, dyntrackSections);

    // Draw final trimmed polyline in world space.
    std::vector<FlexVec2> ptsLocal;
    samplePath(dyntrackSections, ptsLocal);
    for (size_t i = 0; i + 1 < ptsLocal.size(); i++) {
        FlexVec2 a = add(P0, add(scale(r0, ptsLocal[i].x), scale(f0, ptsLocal[i].y)));
        FlexVec2 b = add(P0, add(scale(r0, ptsLocal[i + 1].x), scale(f0, ptsLocal[i + 1].y)));
        drawLine(czerwony, (int)a.x, (int)a.y, (int)b.x, (int)b.y);
    }

    if (Game::gui && myLabel != nullptr && img != nullptr)
        myLabel->setPixmap(QPixmap::fromImage(*img));
    flipCurveAngleSignsForDyntrack(dyntrackSections);
    logFlexCase(true);
    return true;
}

void Flex::drawLine(QPen niebieski, int x1, int y1, int x2, int y2){
        if (!Game::gui || painter == nullptr || img == nullptr)
            return;
        int off = 400;
        int start = 800;
        //x1 /= 4;
        //x2 /= 4;
        //y1 /= 4;
        //y2 /= 4;
        
        painter->begin(img);
        painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);
        painter->setPen(niebieski); 
        painter->drawLine((x1-offx+off),start-(y1-offy+off),(x2-offx+off),start-(y2-offy+off));
        painter->end();
}
