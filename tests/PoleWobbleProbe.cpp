// TSRE GenX - bounded, repeatable pole wobble regression probe. GPL v3 or later.
#include "PoleWobble.h"
#include "WireAttachmentOrder.h"
#include "AutoPlaceRange.h"
#include "AutoPlaceInput.h"
#include <QDebug>
#include <QVector3D>

int main() {
    int failures = 0;
    auto check = [&failures](bool ok, const char *message) {
        if(!ok) { qCritical() << message; ++failures; }
    };
    auto clickMode = [](Qt::KeyboardModifiers modifiers) {
        // No preceding viewport key event: e.g. Shift held in the Range field.
        QMouseEvent click(QEvent::MouseButtonPress, QPointF(10, 10),
            QPointF(10, 10), Qt::LeftButton, Qt::LeftButton, modifiers);
        return AutoPlaceInput::mode(click);
    };
    const int reverse = clickMode(Qt::ShiftModifier);
    check(reverse == 2, "Shift on the click must select reverse without a viewport key press");
    check(clickMode(Qt::ShiftModifier | Qt::ControlModifier) == 2,
        "Shift takes precedence over Control");
    check(clickMode(Qt::ControlModifier) == 1, "Control selects pointer-forward placement");
    check(clickMode(Qt::NoModifier) == 0, "Released Shift must not leave reverse placement latched");
    auto reverseStations = [reverse](double pointer, double range) {
        std::vector<double> values;
        for(std::size_t i = 0; i < 200; ++i) {
            const auto station = AutoPlaceRange::station(i, 100, 10000, pointer, range, reverse);
            if(!station) break;
            values.push_back(*station);
        }
        return values;
    };
    for(double origin : {10000.0, 9975.0, 5000.0}) {
        const auto values = reverseStations(origin, 2000);
        check(values.size() == 21, "Shift range 2000 at spacing 100 must produce 21 stations");
        for(std::size_t i = 0; i < values.size(); ++i)
            check(values[i] == origin - 100 * i, "Reverse stations must descend from the click");
    }
    check(reverseStations(5000, 225) == std::vector<double>({5000, 4900, 4800}),
        "Reverse partial intervals must not exceed range");
    check(reverseStations(5000, 25) == std::vector<double>({5000}),
        "Reverse range shorter than spacing places only the start");
    check(reverseStations(250, 2000) == std::vector<double>({250, 150, 50}),
        "Reverse range must stop at the vector boundary");
    check(reverseStations(250, 0) == std::vector<double>({250, 150, 50}),
        "Unlimited reverse still stops at the vector boundary");
    auto stations = [](double spacing, double available, double range) {
        std::vector<double> values;
        for(std::size_t i = 0; i < 100; ++i) {
            auto distance = AutoPlaceRange::distance(i, spacing, available, range);
            if(!distance) break;
            values.push_back(*distance);
        }
        return values;
    };
    const double pointer = 8000;
    const double sectionLength = 10000;
    const double boundedStart = AutoPlaceRange::startPosition(0, pointer, 225);
    check(boundedStart == pointer,
        "Finite normal runs must start at the clicked position, not a distant section start");
    auto boundedStations = stations(50, sectionLength - boundedStart, 225);
    for(double &station : boundedStations) station += boundedStart;
    check(boundedStations == std::vector<double>({8000, 8050, 8100, 8150, 8200}),
        "Finite placement must cover the requested range near the pointer");
    check(AutoPlaceRange::startPosition(0, pointer, 0) == 0,
        "Unlimited normal runs preserve whole-section placement");
    for(double range : {0.0, 225.0}) {
        check(AutoPlaceRange::startPosition(1, pointer, range) == pointer,
            "Ctrl-click always starts at the pointer");
        check(AutoPlaceRange::startPosition(2, pointer, range) == 0,
            "Backward runs retain pointer-relative distance arithmetic");
    }
    check(stations(50, sectionLength - 9975, 225) == std::vector<double>({0}),
        "Finite runs near the section end must not cross the boundary");
    check(stations(50, 1000, 225) == std::vector<double>({0, 50, 100, 150, 200}),
        "Range must not squeeze a pole into a partial last interval");
    check(stations(50, 1000, 200) == std::vector<double>({0, 50, 100, 150, 200}),
        "An exact user range includes its regularly spaced endpoint");
    check(stations(50, 125, 0) == std::vector<double>({0, 50, 100}),
        "Unlimited range must still stop at the section boundary");
    check(stations(50, 100, 1000) == std::vector<double>({0, 50}),
        "User range must not extend beyond the section");
    check(stations(50, 1000, 25) == std::vector<double>({0}),
        "A short range places only the start");
    check(stations(1.1, 1000, 5.5).size() == 6,
        "Decimal spacing must include an exact decimal endpoint");
    check(stations(0, 1000, 50).empty(), "Invalid spacing must not loop");
    int unchanged = 0;
    float largestLean = 0;
    bool quadrants[4] = {};
    const QQuaternion base = QQuaternion::fromEulerAngles(0.3f, 47.0f, -0.2f);
    for(int i = 0; i < 2048; ++i) {
        const QString key = QString("-10/20/%1/pole.s").arg(i);
        const auto full = PoleWobble::sample(key, 100);
        const auto half = PoleWobble::sample(key, 50);
        const auto again = PoleWobble::sample(key, 100);
        largestLean = qMax(largestLean, full.lean);
        const auto thirty = PoleWobble::sample(key, 30);
        check(thirty.lean <= 3.00001f && std::abs(thirty.heading) <= 3.00001f,
              "Thirty percent must stay within three degrees");
        check(full.affected == again.affected && full.azimuth == again.azimuth
            && full.lean == again.lean && full.heading == again.heading,
            "Wobble must be repeatable for a support identity");
        check(full.lean >= 0 && full.lean <= 10 && std::abs(full.heading) <= 10,
              "Maximum angles exceeded");
        check(half.affected == full.affected && half.azimuth == full.azimuth
            && std::abs(half.lean * 2 - full.lean) < 0.000001f
            && std::abs(half.heading * 2 - full.heading) < 0.000001f,
            "Percentage must scale magnitude without rerolling direction");
        const auto zero = PoleWobble::rotation(base, PoleWobble::sample(key, 0));
        check(zero == base, "Zero must preserve the starting rotation exactly");
        const auto rotated = PoleWobble::rotation(base, full);
        check(std::abs(rotated.length() - 1) < 0.00001f,
              "Result quaternion must remain normalized");
        if(!full.affected) {
            ++unchanged;
            check(rotated == base, "Unaffected poles must preserve their rotation");
        } else {
            const QVector3D up = PoleWobble::rotation(QQuaternion(), full)
                .rotatedVector(QVector3D(0, 1, 0));
            check(up.y() >= std::cos(10.001f * 0.01745329252f),
                  "Combined rotation exceeds the ten-degree lean cone");
            quadrants[(up.x() < 0 ? 2 : 0) + (up.z() < 0 ? 1 : 0)] = true;
        }
        check(PoleWobble::sameRotation(rotated, -rotated),
              "Equivalent signed quaternions must compare equal");
    }
    check(unchanged > 700 && unchanged < 1350,
          "A substantial subset must remain unchanged");
    check(largestLean > 9.0f, "Full strength must reach the expanded neglected-line range");
    for(bool quadrant : quadrants) check(quadrant, "Lean must cover all 360 degrees");
    check(!PoleWobble::sameRotation(QQuaternion(),
        QQuaternion::fromAxisAndAngle(1, 0, 0, 0.01f)),
        "Small deliberate rotations must not be mistaken for unchanged poles");
    check(WireAttachmentOrder::attachmentId("snap_24") == 24, "Case-insensitive SNAP number");
    check(WireAttachmentOrder::attachmentId("SNAP_1000") == 1000, "Sparse high attachment number");
    for(const QString &name : {QString("SNAP_W1"), QString("MIRROR_W1"), QString("SNAP_01"),
            QString("SNAP_0"), QString("SNAP_-1"), QString("SNAP_1x"), QString("SNAP_"),
            QString("SNAP_999999999999999999999")})
        check(WireAttachmentOrder::attachmentId(name) == 0, "Malformed/legacy attachment rejected");
    for(int count : {16, 24, 32}) {
    QMap<int, QVector3D> local, a, b;
    for(int i = 0; i < count; ++i) {
        // Sparse IDs verify matching uses names rather than dense array offsets.
        local[2*i+1] = QVector3D((i % 8 - 3.5f) * 0.3f, 8.4f - (i / 8) * 0.9f, 0.17f);
    }
    // Exercise polarity changes at every neighboring span, including full wobble.
    const float headings[] = {0, 180, 180, 0, 180, 0};
    for(int span = 0; span < 5; ++span) {
        const auto qa = QQuaternion::fromEulerAngles(7, headings[span] + 3, -6);
        const auto qb = QQuaternion::fromEulerAngles(-8, headings[span + 1] - 4, 5);
        for(int i = 0; i < count; ++i) {
            a[2*i+1] = qa.rotatedVector(local[2*i+1]);
            b[2*i+1] = qb.rotatedVector(local[2*i+1]) + QVector3D(16, 2, 50);
        }
        const auto mapping = WireAttachmentOrder::match(local, a, b);
        for(int i = 0; i < count; ++i) {
            const int expected = headings[span] == headings[span + 1] ? i : (i / 8) * 8 + 7 - i % 8;
            check(mapping[2*i+1] == 2*expected+1, "Wire polarity must follow every span without crossing rows");
        }
    }
    b.remove(7);
    const auto incomplete = WireAttachmentOrder::match(local, a, b);
    for(int i = 0; i < count; ++i)
        check(incomplete[2*i+1] == 2*i+1, "Incomplete rows must not be silently permuted");
    }
    if(!failures) qInfo() << "Pole wobble and per-span wire polarity checks passed";
    return failures ? 1 : 0;
}
