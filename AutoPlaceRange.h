// Auto Place distance limits. GPL v3 or later.
#ifndef AUTO_PLACE_RANGE_H
#define AUTO_PLACE_RANGE_H
#include <cmath>
#include <cstddef>
#include <optional>
namespace AutoPlaceRange {
inline double startPosition(int mode, double pointer, double range) {
    // Backward runs measure from the pointer in the caller. Unlimited normal
    // runs retain the legacy whole-section start; bounded runs start locally.
    return mode == 1 || (mode == 0 && range > 0) ? pointer : 0;
}
inline std::optional<double> distance(std::size_t index, double spacing,
                                     double available, double range) {
    if(!std::isfinite(spacing) || spacing < 1 || !std::isfinite(available)
            || !std::isfinite(range) || range < 0) return std::nullopt;
    const double value = index * spacing;
    // Section end remains exclusive, as in legacy AP; user range is inclusive.
    if(value >= available || (range > 0 && value > range + 1e-9)) return std::nullopt;
    return value;
}
inline std::optional<double> station(std::size_t index, double spacing,
                                    double length, double pointer,
                                    double range, int mode) {
    if(!std::isfinite(length) || !std::isfinite(pointer)
            || pointer < 0 || pointer > length) return std::nullopt;
    const double start = startPosition(mode, pointer, range);
    const double available = mode == 2 ? pointer : length - start;
    const auto offset = distance(index, spacing, available, range);
    if(!offset) return std::nullopt;
    return mode == 2 ? pointer - *offset : start + *offset;
}
}
#endif
