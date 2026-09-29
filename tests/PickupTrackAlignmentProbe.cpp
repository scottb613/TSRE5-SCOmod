#include "PickupTrackAlignment.h"
#include <iostream>

namespace {
constexpr float pi = 3.14159265358979323846f;
int failures = 0;

void check(bool ok, const char* label) {
    if (!ok) {
        std::cerr << label << '\n';
        ++failures;
    }
}

void checkHeading(const float* q, float expected, const char* label) {
    // Compare physical forward directions, independent of quaternion sign or
    // angle wrapping, rather than repeating the heading extraction formula.
    const float forwardX = 2 * q[1] * q[3];
    const float forwardZ = 1 - 2 * q[1] * q[1];
    check(std::abs(forwardX - std::sin(expected)) < 0.0001f
          && std::abs(forwardZ - std::cos(expected)) < 0.0001f
          && q[0] == 0 && q[2] == 0, label);
}
}

int main() {
    // Track yaw followed by pitch, matching TDB's quaternion composition.
    for (const float yaw : {-3.0f, -1.2f, 0.0f, 0.7f, 3.0f}) {
        for (const float pitch : {-0.3f, 0.0f, 0.3f}) {
            float q[4] = {std::cos(yaw / 2) * std::sin(pitch / 2),
                          std::sin(yaw / 2) * std::cos(pitch / 2),
                          -std::sin(yaw / 2) * std::sin(pitch / 2),
                          std::cos(yaw / 2) * std::cos(pitch / 2)};
            PickupTrackAlignment::upright(q, PickupTrackAlignment::heading(q));
            checkHeading(q, yaw, "Placement must follow heading without track pitch");
        }
    }

    for (int turns = 0; turns < 4; ++turns) {
        const float offset = turns * pi / 2;
        PickupTrackAlignment alignment;
        alignment.reset(2.9f);
        float q[4];
        PickupTrackAlignment::upright(q, 2.9f + offset);
        // Follow a curve across the +/- pi discontinuity.
        for (const float yaw : {3.0f, -3.1f, -2.8f, -2.0f}) {
            alignment.follow(q, yaw, 0);
            checkHeading(q, yaw + offset, "Curve must retain quarter-turn offset");
        }
        // Undo clones must retain the reference.
        auto restored = alignment;
        restored.follow(q, -1.8f, 0);
        checkHeading(q, -1.8f + offset, "Copied reference must retain offset");
        // Reload has only QDirection; reconstruct from the old local tangent.
        PickupTrackAlignment reloaded;
        for (float& component : q)
            component = -component;
        reloaded.follow(q, -1.6f, -1.8f);
        checkHeading(q, -1.6f + offset, "Reload/sign normalization must retain offset");
        // Only Rot may sample changing headings without moving the origin.
        reloaded.follow(q, 0.4f, -1.8f);
        reloaded.follow(q, 0.4f, -1.8f);
        checkHeading(q, 0.4f + offset, "Repeated rotation-only snap must not accumulate drift");
    }
    float q[4];
    PickupTrackAlignment::upright(q, 0.7f);
    for (int i = 0; i < 4; ++i)
        PickupTrackAlignment::upright(q, PickupTrackAlignment::heading(q) + pi / 2);
    checkHeading(q, 0.7f, "Four quarter turns must return to original heading");
    return failures ? 1 : 0;
}
