// TSRE GenX - temporary Auto Place appearance controls. GPL v3 or later.
#ifndef TSRE_POLE_WOBBLE_H
#define TSRE_POLE_WOBBLE_H

#include <QQuaternion>
#include <QString>
#include <cmath>

namespace PoleWobble {
struct Sample {
    bool affected;
    float azimuth;
    float lean;
    float heading;
};

// Fixed hash/PRNG rather than Qt's process-randomized hash or global RNG.
inline Sample sample(const QString &identity, int percent) {
    quint32 seed = 2166136261u;
    for(QChar c : identity) seed = (seed ^ c.unicode()) * 16777619u;
    auto random = [&seed]() {
        seed += 0x9e3779b9u;
        quint32 n = seed;
        n = (n ^ (n >> 16)) * 0x21f0aaadu;
        n = (n ^ (n >> 15)) * 0x735a2d97u;
        n ^= n >> 15;
        return float(n >> 8) / 16777216.0f;
    };
    const bool affected = random() >= 0.5f;
    const float azimuth = random() * 360.0f;
    const float strength = qBound(0, percent, 100) / 100.0f;
    const float lean = random() * 10.0f * strength;
    const float heading = (2.0f * random() - 1.0f) * 10.0f * strength;
    return {affected, azimuth, affected ? lean : 0.0f,
            affected ? heading : 0.0f};
}

inline QQuaternion rotation(const QQuaternion &base, const Sample &s) {
    if(!s.affected || (s.lean == 0 && s.heading == 0)) return base;
    const float radians = s.azimuth * 0.017453292519943295f;
    const QQuaternion tilt = QQuaternion::fromAxisAndAngle(
        std::cos(radians), 0, std::sin(radians), s.lean);
    const QQuaternion heading = QQuaternion::fromAxisAndAngle(0, 1, 0, s.heading);
    return (tilt * heading * base).normalized();
}

inline bool sameRotation(const QQuaternion &a, const QQuaternion &b) {
    // Component comparison also recognizes q and -q; avoids dot-product
    // rounding hiding the small rotations produced by low percentages.
    const auto close = [](const QQuaternion &q) {
        return std::abs(q.scalar()) < 0.000001f
            && std::abs(q.x()) < 0.000001f && std::abs(q.y()) < 0.000001f
            && std::abs(q.z()) < 0.000001f;
    };
    return close(a - b) || close(a + b);
}

}
#endif
