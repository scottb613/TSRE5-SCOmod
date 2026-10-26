// TSRE GenX texture paint regression coverage. GNU GPL v3 or later.
#include "TexturePaintMath.h"
#include <QJsonDocument>
#include <cmath>
#include <iostream>

int main() {
    bool passed = true;
    auto check = [&](bool condition, const char *message) {
        if(!condition) { std::cerr << message << '\n'; passed = false; }
    };
    // Old presets must not inherit a previously used scale.
    check(TexturePaintMath::presetScalePercent({}) == 100, "legacy preset default");
    check(TexturePaintMath::presetScalePercent({{"scale", "bad"}}) == 100, "invalid preset default");
    check(TexturePaintMath::presetScalePercent({{"scale", 0}}) == 25, "lower bound");
    check(TexturePaintMath::presetScalePercent({{"scale", 999}}) == 400, "upper bound");
    const QJsonObject saved{{"scale", 175}, {"rotation", 37}};
    const auto loaded = QJsonDocument::fromJson(QJsonDocument(saved).toJson()).object();
    check(TexturePaintMath::presetScalePercent(loaded) == 175, "preset JSON round trip");
    check(TexturePaintMath::sourceCoordinate(80, 50, 256) == 160, "half size details");
    check(TexturePaintMath::sourceCoordinate(80, 200, 256) == 40, "double size details");
    check(TexturePaintMath::sourceCoordinate(-80, 200, 128) == 88, "negative wrapping");
    check(TexturePaintMath::sourceCoordinate(400, 25, 128) == 64, "repeated wrapping");
    // Compare default sampling to the original rotation/wrapping calculation.
    for(int angle : {0, 37, 90, 180, 270, 360}) {
        const float radians = float(angle % 360) * 3.14159265358979323846f / 180.0f;
        for(int x = 0; x < 512; x += 7) {
            for(int y = 0; y < 512; y += 11) {
                const float u = angle % 360 == 0 ? float(x) : std::cos(radians)*x + std::sin(radians)*y;
                const float v = angle % 360 == 0 ? float(y) : -std::sin(radians)*x + std::cos(radians)*y;
                for(int limit : {128, 256, 512}) {
                    check(TexturePaintMath::sourceCoordinate(u, 100, limit) == ((qRound(u)%limit)+limit)%limit,
                          "100 percent column parity");
                    check(TexturePaintMath::sourceCoordinate(v, 100, limit) == ((qRound(v)%limit)+limit)%limit,
                          "100 percent row parity");
                    for(int scale : {25, 50, 100, 175, 200, 400}) {
                        const int coord = TexturePaintMath::sourceCoordinate(v, scale, limit);
                        check(coord >= 0 && coord < limit, "scaled rotated coordinate stays in bounds");
                    }
                }
            }
        }
    }
    return passed ? 0 : 1;
}
