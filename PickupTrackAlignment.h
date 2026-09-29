#ifndef PICKUP_TRACK_ALIGNMENT_H
#define PICKUP_TRACK_ALIGNMENT_H

#include <cmath>

// Editor-only reference; ordinary QDirection is the saved representation.
// Reconstruct the reference from the track after loading.
struct PickupTrackAlignment {
    bool hasReference = false;
    float referenceHeading = 0;

    static float heading(const float* q) {
        return std::atan2(2 * (q[3] * q[1] + q[0] * q[2]),
                          1 - 2 * (q[0] * q[0] + q[1] * q[1]));
    }

    static void upright(float* q, float yaw) {
        q[0] = q[2] = 0;
        q[1] = std::sin(yaw / 2);
        q[3] = std::cos(yaw / 2);
    }

    void reset(float yaw) {
        referenceHeading = yaw;
        hasReference = true;
    }

    void follow(float* q, float newHeading, float previousHeading) {
        const float offset = heading(q) - (hasReference ? referenceHeading : previousHeading);
        upright(q, newHeading + offset);
        reset(newHeading);
    }
};

#endif
