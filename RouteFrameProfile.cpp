// TSRE GenX opt-in route editor profiling. Licensed under GNU GPL v3 or later.
#include "RouteFrameProfile.h"

#include <QDebug>
#include <QStringList>
#include <algorithm>
#include <cstddef>
#include <utility>

namespace {
constexpr std::array<const char *, static_cast<int>(RouteFrameProfile::Stage::Count)> stageNames{
    "shadows", "distant_terrain", "near_terrain", "world", "world_tile_load",
    "world_object_init", "shape_lookup", "shape_load", "shape_read", "gltf_load", "gltf_graphics",
    "texture_request", "texture_cache_lookup", "dds_decode", "dds_publish", "texture_upload",
    "forest_build", "forest_draw", "polyveg_candidate", "polyveg_schema", "polyveg_flush",
    "shape_parse_inclusive", "shape_graphics", "terrain_mesh_snapshot", "terrain_mesh_build",
    "terrain_mesh_upload", "terrain_border_read", "static_shape_flush", "shape_worker_parse",
    "world_tile_draw", "world_track_overlay", "grade_cache_rebuild",
    "trackdb_overlay_build", "track_overlay_labels", "track_section_overlay_index", "track_section_overlay_build"
};
constexpr std::array<const char *, static_cast<int>(RouteFrameProfile::Counter::Count)> counterNames{
    "object_render_attempts", "shape_load_deferred", "forest_build_deferred",
    "shape_draws", "gltf_draws", "forest_trees_built", "forest_instanced_draws", "forest_tree_instances",
    "polyveg_instanced_draws", "polyveg_instances", "terrain_dds_submitted", "terrain_dds_completed",
    "shape_read_submitted", "shape_read_completed", "terrain_mesh_submitted", "terrain_mesh_completed",
    "terrain_preview_uploaded", "terrain_full_uploaded", "terrain_full_evicted",
    "terrain_native_ace", "terrain_native_dds", "terrain_native_bytes",
    "terrain_full_preflight_rejected", "terrain_full_rejected_after_read",
    "static_shape_instanced_draws", "static_shape_primitive_instances", "static_shape_placements"
};

QString milliseconds(qint64 nanoseconds) {
    return QString::number(nanoseconds / 1000000.0, 'f', 3);
}

void reportTopDraws(const char *kind, QHash<QString, qint64> &counts) {
    if (counts.isEmpty())
        return;
    QList<QPair<QString, qint64>> ranked;
    ranked.reserve(counts.size());
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        ranked.emplaceBack(it.key(), it.value());
    const auto end = ranked.begin() + qMin<qsizetype>(8, ranked.size());
    std::partial_sort(ranked.begin(), end, ranked.end(), [](const auto &a, const auto &b) {
        return a.second == b.second ? a.first < b.first : a.second > b.second;
    });
    for (auto it = ranked.begin(); it != end; ++it)
        qInfo() << "Editor profile ordinary shape draws" << kind << it->second << it->first;
    counts.clear();
}

template <typename Samples>
QString distribution(const QString &prefix, Samples &values, int size) {
    if (!size)
        return {};
    std::sort(values.begin(), values.begin() + size);
    const qint64 median = (values[(size - 1) / 2] + values[size / 2]) / 2;
    const int p95 = (95 * size + 99) / 100 - 1;
    return QStringLiteral("%1_median_ms=%2 %1_p95_ms=%3 %1_max_ms=%4")
        .arg(prefix, milliseconds(median), milliseconds(values[p95]),
             milliseconds(values[size - 1]));
}
} // namespace

bool RouteFrameProfile::enabled() {
    static const bool profiling = qEnvironmentVariableIntValue("TSRE_PROFILE_EDITOR") != 0;
    return profiling;
}

void RouteFrameProfile::recordDuration(Stage stage, qint64 nanoseconds, const QString &resource) {
    if (!active)
        return;
    auto &stat = active->pendingStages[static_cast<int>(stage)];
    ++stat.count;
    stat.total += nanoseconds;
    if (nanoseconds > stat.peak) {
        stat.peak = nanoseconds;
        stat.peakResource = resource;
    }
}

RouteFrameProfile::Frame::Frame(RouteFrameProfile &owner, bool normalPaint) {
    if (!normalPaint || !enabled() || active)
        return;
    profile = &owner;
    static const bool profileShapeDraws =
        qEnvironmentVariableIntValue("TSRE_PROFILE_SHAPE_DRAWS") != 0;
    owner.breakDownShapeDraws = profileShapeDraws;
    if (!owner.clock.isValid())
        owner.clock.start();
    start = owner.clock.nsecsElapsed();
    active = profile;
}

RouteFrameProfile::Frame::~Frame() {
    if (!profile)
        return;
    profile->pendingPaint += profile->clock.nsecsElapsed() - start;
    ++profile->paintsPending;
    active = nullptr;
}

void RouteFrameProfile::presented() {
    if (!paintsPending)
        return;
    const qint64 now = clock.nsecsElapsed();
    if (previousSwap) {
        const qint64 gap = now - previousSwap;
        swapIntervals[intervalSamples++] = gap;
        if (gap > worstGap) {
            worstGap = gap;
            worstGapPaint = pendingPaint;
            worstGapStages = pendingStages;
        }
    }
    for (std::size_t i = 0; i < stages.size(); ++i) {
        auto &window = stages[i];
        auto &frame = pendingStages[i];
        window.count += frame.count;
        window.total += frame.total;
        if (frame.peak > window.peak) {
            window.peak = frame.peak;
            window.peakResource = std::move(frame.peakResource);
        }
    }
    pendingStages = {};
    previousSwap = now;
    paintTimes[samples++] = pendingPaint;
    paintPasses += paintsPending;
    paintsPending = 0;
    pendingPaint = 0;
    if (windows == 0 || samples == windowFrames || now - windowStart >= 2000000000LL)
        report(now);
}

void RouteFrameProfile::report(qint64 now) {
    qInfo().noquote() << "Editor profile frame"
        << "window=" + QString::number(++windows)
        << "elapsed_ms=" + milliseconds(now)
        << "window_ms=" + milliseconds(now - windowStart)
        << "swaps=" + QString::number(samples)
        << "swap_intervals=" + QString::number(intervalSamples)
        << "paint_passes=" + QString::number(paintPasses)
        << distribution(QStringLiteral("paint"), paintTimes, samples)
        << distribution(QStringLiteral("swap_interval"), swapIntervals, intervalSamples);

    QStringList stageFields;
    for (std::size_t i = 0; i < stages.size(); ++i) {
        const auto &stat = stages[i];
        if (!stat.count)
            continue;
        stageFields << QStringLiteral("%1=%2/%3/%4")
            .arg(QString::fromLatin1(stageNames[i]), QString::number(stat.count),
                 milliseconds(stat.total), milliseconds(stat.peak));
        if (stat.peak >= 5000000 && !stat.peakResource.isEmpty()) {
            qInfo() << "Editor profile slow resource" << stageNames[i]
                    << "ms=" << milliseconds(stat.peak) << stat.peakResource;
        }
    }
    if (!stageFields.isEmpty())
        qInfo().noquote() << "Editor profile stages (calls/total_ms/max_ms)" << stageFields.join(' ');

    if (worstGap >= 33000000) {
        QStringList frameFields;
        for (std::size_t i = 0; i < worstGapStages.size(); ++i) {
            const auto &stat = worstGapStages[i];
            if (stat.total < 100000 || i == static_cast<std::size_t>(Stage::ShapeWorkerParse))
                continue;
            frameFields << QStringLiteral("%1=%2/%3/%4")
                .arg(QString::fromLatin1(stageNames[i]), QString::number(stat.count),
                     milliseconds(stat.total), milliseconds(stat.peak));
        }
        qInfo().noquote() << "Editor profile worst gap stages (calls/total_ms/max_ms)"
            << "gap_ms=" + milliseconds(worstGap) << "paint_ms=" + milliseconds(worstGapPaint)
            << frameFields.join(' ');
    }

    QStringList counterFields;
    for (std::size_t i = 0; i < counters.size(); ++i) {
        if (counters[i]) {
            counterFields << QStringLiteral("%1=%2")
                .arg(QString::fromLatin1(counterNames[i]), QString::number(counters[i]));
        }
    }
    if (!counterFields.isEmpty())
        qInfo().noquote() << "Editor profile counts" << counterFields.join(' ');
    reportTopDraws("shape", drawsByShape);
    reportTopDraws("material", drawsByMaterial);

    stages = {};
    worstGapStages = {};
    worstGap = 0;
    worstGapPaint = 0;
    counters = {};
    samples = 0;
    intervalSamples = 0;
    paintPasses = 0;
    windowStart = now;
}
