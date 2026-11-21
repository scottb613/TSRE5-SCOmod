// TSRE GenX opt-in route editor profiling. Licensed under GNU GPL v3 or later.
#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QString>

#include <array>

class RouteFrameProfile {
public:
    enum class Stage {
        Shadows, DistantTerrain, NearTerrain, World, WorldTileLoad,
        WorldObjectInit, ShapeLookup, ShapeLoad, ShapeRead, GltfLoad, GltfGraphics,
        TextureRequest, TextureCacheLookup, DdsDecode, DdsPublish, TextureUpload,
        ForestBuild, ForestDraw, PolyVegCandidate, PolyVegSchema, PolyVegFlush,
        ShapeParse, ShapeGraphics, TerrainMeshSnapshot, TerrainMeshBuild, TerrainMeshUpload,
        TerrainBorderRead, StaticShapeFlush, ShapeWorkerParse,
        WorldTileDraw, WorldTrackOverlay, GradeCacheRebuild,
        TrackDbOverlayBuild, TrackOverlayLabels, TrackSectionOverlayIndex, TrackSectionOverlayBuild,
        Count
    };
    enum class Counter {
        ObjectRenderAttempts, ShapeLoadDeferred, ForestBuildDeferred,
        ShapeDraws, GltfDraws, ForestTreesBuilt, ForestInstancedDraws, ForestTreeInstances,
        PolyVegInstancedDraws, PolyVegInstances, TerrainDdsSubmitted, TerrainDdsCompleted,
        ShapeReadSubmitted, ShapeReadCompleted, TerrainMeshSubmitted, TerrainMeshCompleted,
        TerrainPreviewUploaded, TerrainFullUploaded, TerrainFullEvicted,
        TerrainNativeAce, TerrainNativeDds, TerrainNativeBytes,
        TerrainFullPreflightRejected, TerrainFullRejectedAfterRead,
        StaticShapeInstancedDraws, StaticShapePrimitiveInstances, StaticShapePlacements, Count
    };

    static bool enabled();
    static void recordDuration(Stage stage, qint64 nanoseconds, const QString &resource);

    // Measures the CPU wall time in the normal paint pass. Driver waits in GL
    // calls are included. Picking passes do not open a sample.
    class Frame {
    public:
        Frame(RouteFrameProfile &profile, bool normalPaint);
        ~Frame();
        Frame(const Frame &) = delete;
        Frame &operator=(const Frame &) = delete;

    private:
        RouteFrameProfile *profile = nullptr;
        qint64 start = 0;
    };

    // Inclusive timings. Each stage can contain other named stages.
    class Scope {
    public:
        explicit Scope(Stage stage, const QString *resource = nullptr)
            : profile(active), resource(resource), stage(stage) {
            if (profile) {
                start = profile->clock.nsecsElapsed();
                if (stage == Stage::World)
                    profile->inWorld = true;
            }
        }
        ~Scope() { finish(); }
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

        void finish() {
            if (!profile)
                return;
            if (stage == Stage::World)
                profile->inWorld = false;
            auto &stat = profile->pendingStages[static_cast<int>(stage)];
            const qint64 elapsed = profile->clock.nsecsElapsed() - start;
            ++stat.count;
            stat.total += elapsed;
            if (elapsed > stat.peak) {
                stat.peak = elapsed;
                stat.peakResource = resource ? *resource : QString{};
            }
            profile = nullptr;
        }

    private:
        RouteFrameProfile *profile;
        const QString *resource;
        Stage stage;
        qint64 start = 0;
    };

    static void count(Counter counter, qint64 amount = 1) {
        if (active)
            active->counters[static_cast<int>(counter)] += amount;
    }

    static bool collectingShapeDraws() {
        return active && active->inWorld && active->breakDownShapeDraws;
    }

    static void countShapeDraw(const QString &shape, const QString &material) {
        ++active->drawsByShape[shape];
        ++active->drawsByMaterial[material];
    }

    // Called after Qt completes composition and swaps the widget's frame.
    void presented();

private:
    struct Stat {
        qint64 count = 0;
        qint64 total = 0;
        qint64 peak = 0;
        QString peakResource;
    };

    static constexpr int windowFrames = 60;
    using Durations = std::array<qint64, windowFrames>;
    using StageStats = std::array<Stat, static_cast<int>(Stage::Count)>;
    inline static thread_local RouteFrameProfile *active = nullptr;
    StageStats stages{};
    StageStats pendingStages{};
    StageStats worstGapStages{};
    std::array<qint64, static_cast<int>(Counter::Count)> counters{};
    QElapsedTimer clock;
    Durations paintTimes{};
    Durations swapIntervals{};
    qint64 pendingPaint = 0;
    qint64 previousSwap = 0;
    qint64 windowStart = 0;
    qint64 worstGap = 0;
    qint64 worstGapPaint = 0;
    int paintsPending = 0;
    int paintPasses = 0;
    int samples = 0;
    int intervalSamples = 0;
    int windows = 0;
    bool inWorld = false;
    bool breakDownShapeDraws = false;
    QHash<QString, qint64> drawsByShape;
    QHash<QString, qint64> drawsByMaterial;

    void report(qint64 now);
};
