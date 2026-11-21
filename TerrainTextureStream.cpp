// TSRE GenX terrain display cache. Licensed under GNU GPL v3 or later.
#include "TerrainTextureStream.h"
#include "DdsLib.h"
#include "GLUU.h"
#include "Game.h"
#include "RouteFrameProfile.h"
#include "Texture.h"

#include <QDir>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPointer>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace {
using Clock = std::chrono::steady_clock;
constexpr int previewSide = 128;
constexpr qint64 previewBudget = 512LL * 1024 * 1024;
constexpr qint64 fullBudget = 768LL * 1024 * 1024;
constexpr int fullTileRadius = 2;
constexpr GLenum bc1Rgb = 0x83F0; // GL_COMPRESSED_RGB_S3TC_DXT1_EXT

struct Level {
    GLuint texture = 0;
    qint64 bytes = 0;
    Clock::time_point used;
    bool failed = false;
};

struct Entry {
    Level preview;
    Level full;
    bool invalidated = false;
    bool nativeRejected = false;
    QPoint tile;
    QPointF center;
    qint64 fullRequired = 0;
    Clock::time_point retryFull;
};

std::map<QString, Entry> entries;
QPointer<QOpenGLContext> owner;
qint64 previewBytes = 0;
qint64 fullBytes = 0;
Clock::time_point lastSweep;
QPoint viewTile;
QPointF viewCenter;
bool viewSet = false;
bool nativeBc1 = false;
std::uint64_t revision = 0;

struct FullTransfer {
    QString path;
    DdsTextureData source;
    Level level;
    int rows = 0;
};
std::optional<FullTransfer> transfer;

struct OffscreenPatch {
    QString path;
    QPoint tile;
    QPointF center;
    double distanceSquared;
};
std::vector<OffscreenPatch> offscreen;

bool useCurrentContext() {
    auto *context = QOpenGLContext::currentContext();
    if (!context)
        return false;
    if (owner.data() != context) {
        TerrainTextureStream::clear();
        owner = context;
        static const bool enableNative =
            qEnvironmentVariableIntValue("TSRE_DISABLE_NATIVE_TERRAIN_DXT1") == 0;
        nativeBc1 = enableNative && (context->hasExtension(QByteArrayLiteral("GL_EXT_texture_compression_s3tc"))
                            || context->hasExtension(QByteArrayLiteral("GL_EXT_texture_compression_dxt1")));
    }
    return true;
}

bool inDetailArea(QPoint tile) {
    return viewSet && std::abs(tile.x() - viewTile.x()) <= fullTileRadius
                   && std::abs(tile.y() - viewTile.y()) <= fullTileRadius;
}

double distanceSquared(QPoint tile, QPointF center) {
    const auto delta = QPointF(tile - viewTile) * 2048 + center - viewCenter;
    return QPointF::dotProduct(delta, delta);
}

double distance(const Entry &entry) {
    return distanceSquared(entry.tile, entry.center);
}

void release(Level &level, qint64 &bytes) {
    if (level.texture && owner && QOpenGLContext::currentContext() == owner.data()) {
        owner->functions()->glDeleteTextures(1, &level.texture);
        GLUU::get()->invalidateTextureBinding();
    }
    bytes -= level.bytes;
    level = {};
}

void releaseFull(Level &level) {
    if (level.texture)
        RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainFullEvicted);
    release(level, fullBytes);
}

void cancelTransfer() {
    if (transfer) {
        release(transfer->level, fullBytes);
        transfer.reset();
    }
}

void removeEntry(Entry &entry) {
    if (entry.preview.texture)
        ++revision;
    release(entry.preview, previewBytes);
    releaseFull(entry.full);
}

bool makeRoom(Level Entry::*which, qint64 needed, qint64 budget, qint64 &usedBytes,
              const Entry &requester) {
    if (needed > budget)
        return false;
    while (usedBytes > budget - needed) {
        auto oldest = entries.end();
        for (auto it = entries.begin(); it != entries.end(); ++it) {
            const Level &level = it->second.*which;
            if (!level.texture)
                continue;
            if (which == &Entry::full) {
                if (distance(it->second) > distance(requester)
                        && (oldest == entries.end() || distance(it->second) > distance(oldest->second)))
                    oldest = it;
            } else if (oldest == entries.end() || level.used < (oldest->second.*which).used) {
                oldest = it;
            }
        }
        if (oldest == entries.end())
            return false;
        if (which == &Entry::full)
            releaseFull(oldest->second.full);
        else {
            release(oldest->second.preview, previewBytes);
            ++revision;
        }
    }
    return true;
}

bool roomForFull(const Entry &entry, qint64 needed) {
    qint64 available = fullBudget - fullBytes;
    if (available >= needed)
        return true;
    const double priority = distance(entry);
    for (const auto &[path, other] : entries) {
        if (other.full.texture && distance(other) > priority)
            available += other.full.bytes;
        if (available >= needed)
            return true;
    }
    return false;
}

void uploadFullStripe(Entry &entry) {
    auto &work = *transfer;
    const auto &source = work.source;
    auto *f = owner->functions();
    const std::size_t rowBytes = std::size_t(source.width) * source.components;
    // A large painted texture can take longer than an entire frame to copy
    // in one call. Keep drawing the preview until all rows have arrived.
    constexpr std::size_t stripeBytes = 1024 * 1024;
    const int rows = std::min(source.height - work.rows, int(std::max<std::size_t>(1, stripeBytes / rowBytes)));
    const GLenum format = source.components == 3 ? GL_RGB : GL_RGBA;
    RouteFrameProfile::Scope uploadTime(RouteFrameProfile::Stage::TextureUpload, &work.path);
    GLint rowAlignment = 4;
    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &rowAlignment);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    f->glGetError();
    f->glBindTexture(GL_TEXTURE_2D, work.level.texture);
    f->glTexSubImage2D(GL_TEXTURE_2D, 0, 0, work.rows, source.width, rows,
                       format, GL_UNSIGNED_BYTE, source.pixels.get() + work.rows * rowBytes);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, rowAlignment);
    GLUU::get()->invalidateTextureBinding();
    uploadTime.finish();
    if (f->glGetError() != GL_NO_ERROR) {
        cancelTransfer();
        entry.full.failed = true;
        return;
    }
    work.rows += rows;
    work.level.used = Clock::now();
    if (work.rows == source.height) {
        entry.full = work.level;
        transfer.reset();
        RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainFullUploaded);
    }
}

void startFullTransfer(Entry &entry, const QString &path, DdsTextureData data, qint64 size) {
    if (data.components == 4 && Game::AASamples > 0 && Game::AARemoveBorder) {
        const auto rowBytes = std::size_t(data.width) * 4;
        auto *pixels = data.pixels.get();
        for (int y = 0; y < data.height; ++y) {
            pixels[y * rowBytes + 3] = 0;
            pixels[y * rowBytes + rowBytes - 1] = 0;
        }
        for (int x = 0; x < data.width; ++x) {
            pixels[x * 4 + 3] = 0;
            pixels[(data.height - 1) * rowBytes + x * 4 + 3] = 0;
        }
    }
    RouteFrameProfile::Scope allocation(RouteFrameProfile::Stage::TextureUpload, &path);
    auto *f = owner->functions();
    f->glGetError();
    Level level;
    f->glGenTextures(1, &level.texture);
    f->glBindTexture(GL_TEXTURE_2D, level.texture);
    const GLenum format = data.components == 3 ? GL_RGB : GL_RGBA;
    f->glTexImage2D(GL_TEXTURE_2D, 0, format, data.width, data.height, 0,
                    format, GL_UNSIGNED_BYTE, nullptr);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLUU::get()->invalidateTextureBinding();
    if (!level.texture || f->glGetError() != GL_NO_ERROR) {
        release(level, fullBytes);
        entry.full.failed = true;
        return;
    }
    level.bytes = size;
    level.used = Clock::now();
    fullBytes += size;
    transfer.emplace(FullTransfer{path, std::move(data), level, 0});
    allocation.finish();
    uploadFullStripe(entry);
}

bool uploadBc1(Level &level, const QString &path, const DdsTextureData &data, qint64 &usedBytes) {
    RouteFrameProfile::Scope upload(RouteFrameProfile::Stage::TextureUpload, &path);
    auto *f = owner->functions();
    f->glGetError();
    f->glGenTextures(1, &level.texture);
    f->glBindTexture(GL_TEXTURE_2D, level.texture);
    f->glCompressedTexImage2D(GL_TEXTURE_2D, 0, bc1Rgb, data.width, data.height, 0,
                              int(data.bc1Blocks.size()), data.bc1Blocks.constData());
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    GLUU::get()->invalidateTextureBinding();
    if (!level.texture || f->glGetError() != GL_NO_ERROR) {
        release(level, usedBytes);
        return false;
    }
    level.bytes = data.bc1Blocks.size();
    level.used = Clock::now();
    usedBytes += level.bytes;
    RouteFrameProfile::count(path.endsWith(".ace", Qt::CaseInsensitive)
        ? RouteFrameProfile::Counter::TerrainNativeAce : RouteFrameProfile::Counter::TerrainNativeDds);
    RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainNativeBytes, level.bytes);
    return true;
}

bool upload(Entry &entry, const QString &path, bool preview, bool prefetch = false) {
    auto &level = preview ? entry.preview : entry.full;
    if (!preview && transfer) {
        if (transfer->path == path) {
            uploadFullStripe(entry);
            return true;
        }
        if (Clock::now() - transfer->level.used <= std::chrono::milliseconds(250))
            return false;
        cancelTransfer();
    }
    if (!preview && entry.fullRequired) {
        if (Clock::now() < entry.retryFull)
            return false;
        if (!roomForFull(entry, entry.fullRequired)) {
            RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainFullPreflightRejected);
            entry.retryFull = Clock::now() + std::chrono::seconds(1);
            return false;
        }
    }
    DdsTextureData data;
    if (!DdsLib::takeStreamed(path, data, preview ? previewSide : 0,
                             nativeBc1 && !entry.nativeRejected, Game::UnsafeMode, prefetch))
        return false;
    const bool compressed = !data.bc1Blocks.isEmpty();
    const qint64 size = compressed ? data.bc1Blocks.size() : qint64(data.width) * data.height * 4;
    auto &budgetUsed = preview ? previewBytes : fullBytes;
    if ((!data.pixels && !compressed) || size > (preview ? previewBudget : fullBudget)) {
        level.failed = true;
        return true;
    }
    if (preview && !entry.fullRequired)
        entry.fullRequired = data.minimumFullUploadBytes;
    if ((!preview && !roomForFull(entry, size))
            || !makeRoom(preview ? &Entry::preview : &Entry::full, size,
                  preview ? previewBudget : fullBudget, budgetUsed, entry)) {
        if (!preview) {
            RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainFullRejectedAfterRead);
            entry.fullRequired = size;
            entry.retryFull = Clock::now() + std::chrono::seconds(1);
        }
        return true;
    }
    if (compressed) {
        if (!uploadBc1(level, path, data, budgetUsed)) {
            // Retry on the worker with CPU pixels if the driver rejects BC1.
            entry.nativeRejected = true;
            entry.fullRequired = preview ? 0 : qint64(data.width) * data.height * 4;
            return false;
        }
        if (!preview)
            entry.fullRequired = size;
        RouteFrameProfile::count(preview ? RouteFrameProfile::Counter::TerrainPreviewUploaded
                                        : RouteFrameProfile::Counter::TerrainFullUploaded);
        return true;
    }
    if (!preview) {
        entry.fullRequired = size;
        startFullTransfer(entry, path, std::move(data), size);
        return true;
    }

    auto *f = owner->functions();
    f->glGetError();
    Texture upload(path);
    DdsLib::publish(upload, std::move(data));
    upload.GLTextures();
    level.texture = upload.tex[0];
    delete[] upload.tex;
    upload.tex = nullptr;
    GLUU::get()->invalidateTextureBinding();
    if (!level.texture || f->glGetError() != GL_NO_ERROR) {
        release(level, budgetUsed);
        level.failed = true;
        return true;
    }

    level.bytes = size;
    level.used = Clock::now();
    budgetUsed += size;
    RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainPreviewUploaded);
    return true;
}
}

TerrainTextureStream::Result TerrainTextureStream::request(
        const QString &path, bool allowWork, bool detailed, QPoint tile, QPointF patchCenter) {
    if (!useCurrentContext())
        return {};
    auto it = entries.find(path);
    if (it == entries.end()) {
        if (!allowWork)
            return {-1, true};
        it = entries.try_emplace(path).first;
    }
    auto &entry = it->second;
    if (entry.invalidated) {
        if (transfer && transfer->path == path)
            cancelTransfer();
        removeEntry(entry);
        entry = {};
    }
    auto &preview = entry.preview;
    auto &full = entry.full;
    const bool fullDetail = detailed && inDetailArea(tile);
    if (!preview.texture && !preview.failed && allowWork)
        upload(entry, path, true);
    const auto now = Clock::now();
    if (preview.texture)
        preview.used = now;
    if (fullDetail && preview.texture && !full.failed && !full.texture && allowWork) {
        entry.tile = tile;
        entry.center = patchCenter;
        upload(entry, path, false);
    }
    if (fullDetail && full.texture) {
        full.used = now;
        return {int(full.texture), false};
    }
    if (preview.texture)
        return {int(preview.texture), false};
    return {-1, !preview.failed};
}

void TerrainTextureStream::setView(QPoint tile, QPointF camera) {
    if (!useCurrentContext())
        return;
    offscreen.clear();
    const auto now = Clock::now();
    const bool movedTile = !viewSet || viewTile != tile;
    viewSet = true;
    viewTile = tile;
    viewCenter = camera;
    if (!movedTile && now - lastSweep < std::chrono::seconds(1))
        return;
    lastSweep = now;
    if (transfer) {
        const auto it = entries.find(transfer->path);
        if (it == entries.end() || it->second.invalidated || !inDetailArea(it->second.tile)
                || now - transfer->level.used > std::chrono::seconds(1))
            cancelTransfer();
    }
    for (auto it = entries.begin(); it != entries.end();) {
        auto &entry = it->second;
        if (entry.invalidated) {
            removeEntry(entry);
            it = entries.erase(it);
            continue;
        }
        if (entry.full.texture && !inDetailArea(entry.tile))
            releaseFull(entry.full);
        if (!entry.preview.texture && !entry.preview.failed && !entry.full.texture)
            it = entries.erase(it);
        else
            ++it;
    }
}

bool TerrainTextureStream::wantsOffscreen(QPoint tile) {
    return inDetailArea(tile);
}

void TerrainTextureStream::queueOffscreen(const QString &path, QPoint tile, QPointF patchCenter) {
    const auto it = entries.find(path);
    if (it != entries.end()) {
        const auto &entry = it->second;
        if (entry.invalidated || entry.preview.failed || entry.full.failed || entry.full.texture
                || Clock::now() < entry.retryFull)
            return;
    }
    offscreen.push_back({path, tile, patchCenter, distanceSquared(tile, patchCenter)});
}

void TerrainTextureStream::prefetchOffscreen(qint64 budgetNs) {
    if (budgetNs <= 0 || offscreen.empty()
            || !owner || QOpenGLContext::currentContext() != owner.data())
        return;

    const auto deadline = Clock::now() + std::chrono::nanoseconds(budgetNs);
    owner->functions()->glActiveTexture(GL_TEXTURE0);
    if (transfer) {
        const auto patch = std::find_if(offscreen.begin(), offscreen.end(), [](const auto &item) {
            return item.path == transfer->path;
        });
        if (patch != offscreen.end())
            uploadFullStripe(entries.at(patch->path));
        return;
    }

    constexpr std::size_t maxAttempts = 4;
    const auto end = offscreen.begin() + std::min(maxAttempts, offscreen.size());
    std::partial_sort(offscreen.begin(), end, offscreen.end(), [](const auto &a, const auto &b) {
        return a.distanceSquared < b.distanceSquared;
    });
    for (auto it = offscreen.begin(); it != end && Clock::now() < deadline && !transfer; ++it) {
        auto &entry = entries[it->path];
        if (entry.invalidated || entry.preview.failed || entry.full.failed || entry.full.texture)
            continue;
        entry.tile = it->tile;
        entry.center = it->center;
        if (!entry.preview.texture)
            upload(entry, it->path, true, true);
        else
            upload(entry, it->path, false, true);
    }
}

void TerrainTextureStream::invalidate(const QString &path) {
    const QString key = QDir::cleanPath(QString(path).replace('\\', '/')).toLower();
    const auto it = entries.find(key);
    if (it != entries.end()) {
        it->second.invalidated = true;
        ++revision;
        DdsLib::discardStreamed(key);
    }
}

std::uint64_t TerrainTextureStream::previewRevision() {
    return revision;
}

void TerrainTextureStream::clear() {
    cancelTransfer();
    for (auto &[path, entry] : entries) {
        removeEntry(entry);
        DdsLib::discardStreamed(path);
    }
    entries.clear();
    offscreen.clear();
    owner.clear();
    nativeBc1 = false;
    lastSweep = {};
    viewSet = false;
}
