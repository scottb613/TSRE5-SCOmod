/*
 * This file is part of TSRE5.
 *
 * Licensed under GNU General Public License 3.0 or later.
 */

#include "DdsLib.h"
#include "DdsDecoder.h"
#include "RouteFrameProfile.h"
#include "RouteLoadTiming.h"
#include "Texture.h"
#include "ace_codec/AceDocument.h"

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QImage>
#include <QOpenGLFunctions>
#include <QThreadPool>

#include <chrono>
#include <cstring>
#include <future>
#include <map>
#include <new>
#include <tuple>
#include <utility>

namespace {

DdsTextureData packPixels(const DdsImage &image) {
    DdsTextureData result;
    result.width = image.width;
    result.height = image.height;
    // Keep opaque RGB24 and BC1 images in their editable RGB representation.
    result.components = image.opaqueRgb24 || image.opaqueDxt1 ? 3 : 4;
    const int pixelCount = image.width * image.height;
    result.pixels.reset(new unsigned char[pixelCount * result.components]);
    if (result.components == 3) {
        const char *source = image.rgba.constData();
        unsigned char *destination = result.pixels.get();
        for (int i = 0; i < pixelCount; ++i, source += 4, destination += 3)
            std::memcpy(destination, source, 3);
    } else {
        std::memcpy(result.pixels.get(), image.rgba.constData(), image.rgba.size());
    }
    return result;
}

struct FileStamp {
    qint64 size;
    QDateTime modified;

    explicit FileStamp(const QString &path) {
        const QFileInfo file(path);
        size = file.size();
        modified = file.lastModified();
    }
    bool matches(const FileStamp &other) const {
        return size == other.size && modified == other.modified;
    }
};

struct StreamedDds {
    FileStamp source;
    DdsTextureData data;
};

DdsTextureData readTerrainAce(const QString &path, int previewSide, bool nativeDxt1, bool allowOdd) {
    DdsTextureData result;
    AceReadOptions options;
    options.maxDimension = 8192;
    options.legacyDxt1Colors = true;
    AceDocument document;
    if (!AceDocument::read(path, document, result.error, options) || document.levels.isEmpty()) {
        result.missing = !QFileInfo::exists(path);
        return result;
    }
    const auto &top = document.levels.front();
    if (top.width <= 1 || top.height <= 1 || (!allowOdd && (top.width % 2 || top.height % 2))) {
        result.error = QStringLiteral("Unsupported editor texture dimensions.");
        return result;
    }
    int mip = 0;
    if (previewSide > 0) {
        while (mip + 1 < document.levels.size()
                && qMax(document.levels[mip].width, document.levels[mip].height) > previewSide)
            ++mip;
    }
    const auto &image = document.levels[mip];
    result.width = image.width;
    result.height = image.height;
    result.components = document.hasAlpha() ? 4 : 3;
    const qint64 topBytes = qint64(top.width) * top.height * 4;
    const bool nativeTop = nativeDxt1 && document.surface() == 18 && top.raw
        && result.components == 3 && top.width % 4 == 0 && top.height % 4 == 0
        && top.data.size() == topBytes / 8;
    result.minimumFullUploadBytes = nativeTop ? topBytes / 8 : topBytes;
    const bool needsResize = previewSide > 0 && qMax(image.width, image.height) > previewSide;
    if (!needsResize && nativeDxt1 && document.surface() == 18 && image.raw
            && result.components == 3 && image.width % 4 == 0 && image.height % 4 == 0
            && image.data.size() == qint64(image.width / 4) * (image.height / 4) * 8) {
        result.bc1Blocks = image.data;
        return result;
    }

    const qsizetype bytes = qsizetype(result.width) * result.height * result.components;
    std::unique_ptr<unsigned char[]> decoded(new unsigned char[bytes]);
    int components = 0;
    if (!document.decodeInto(mip, decoded.get(), bytes, components, result.error))
        return result;
    if (!needsResize) {
        result.pixels = std::move(decoded);
        return result;
    }
    const auto format = components == 3 ? QImage::Format_RGB888 : QImage::Format_RGBA8888;
    const QImage source(decoded.get(), image.width, image.height,
                        qsizetype(image.width) * components, format);
    const auto preview = source.scaled(previewSide, previewSide, Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation).convertToFormat(format);
    if (preview.isNull()) {
        result.error = QStringLiteral("Could not generate texture preview.");
        return result;
    }
    result.width = preview.width();
    result.height = preview.height();
    const qsizetype stride = qsizetype(result.width) * components;
    result.pixels.reset(new unsigned char[stride * result.height]);
    for (int row = 0; row < result.height; ++row)
        std::memcpy(result.pixels.get() + row * stride, preview.constScanLine(row), stride);
    return result;
}

StreamedDds readStreamed(const QString &path, int previewSide, bool nativeDxt1, bool allowOddAce) {
    StreamedDds result{FileStamp(path), {}};
    try {
        if (path.endsWith(".ace", Qt::CaseInsensitive)) {
            result.data = readTerrainAce(path, previewSide, nativeDxt1, allowOddAce);
            return result;
        }
        DdsBc1Level bc1;
        const qint64 minimumFullUploadBytes = previewSide > 0
            ? DdsDecoder::minimumUploadBytes(path, nativeDxt1) : 0;
        if (nativeDxt1 && DdsDecoder::readOpaqueBc1(path, previewSide, bc1)) {
            result.data.width = bc1.width;
            result.data.height = bc1.height;
            result.data.components = 3;
            result.data.bc1Blocks = std::move(bc1.blocks);
            result.data.minimumFullUploadBytes = minimumFullUploadBytes;
            return result;
        }
        DdsImage image;
        if ((previewSide > 0 && DdsDecoder::decodeStoredPreview(path, previewSide, image))
                || DdsDecoder::decodeFile(path, image, &result.data.error)) {
            if (previewSide > 0 && qMax(image.width, image.height) > previewSide) {
                const QImage rgba(reinterpret_cast<const uchar *>(image.rgba.constData()),
                                  image.width, image.height, QImage::Format_RGBA8888);
                const QImage scaled = rgba.scaled(previewSide, previewSide, Qt::KeepAspectRatio,
                                                  Qt::SmoothTransformation)
                                          .convertToFormat(QImage::Format_RGBA8888);
                if (scaled.isNull()) {
                    result.data.error = QStringLiteral("Could not generate texture preview.");
                    return result;
                }
                image.width = scaled.width();
                image.height = scaled.height();
                image.rgba = QByteArray(reinterpret_cast<const char *>(scaled.constBits()),
                                       scaled.sizeInBytes());
            }
            result.data = packPixels(image);
            result.data.minimumFullUploadBytes = minimumFullUploadBytes;
        } else
            result.data.missing = !QFileInfo::exists(path);
    } catch (const std::bad_alloc &) {
        result.data.error = QStringLiteral("Not enough memory to decode texture.");
    }
    return result;
}

using Clock = std::chrono::steady_clock;
struct PendingDds {
    std::future<StreamedDds> result;
    Clock::time_point lastRequest;
    bool ready() const {
        return result.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
};

struct TerrainDdsStream {
    QThreadPool workers;
    std::map<std::tuple<QString, int, bool, bool>, PendingDds> pending;

    TerrainDdsStream() { workers.setMaxThreadCount(2); }
};

TerrainDdsStream &terrainStream() {
    static TerrainDdsStream stream;
    return stream;
}

} // namespace

bool DdsLib::takeStreamed(const QString &path, DdsTextureData &result, int previewSide,
                          bool nativeDxt1, bool allowOddAce, bool prefetch) {
    auto &stream = terrainStream();
    auto &pending = stream.pending;
    const auto now = Clock::now();
    const auto key = std::make_tuple(path, previewSide, nativeDxt1, allowOddAce);
    auto it = pending.find(key);
    if (it != pending.end()) {
        it->second.lastRequest = now;
        if (!it->second.ready())
            return false;
        auto completed = it->second.result.get();
        pending.erase(it);
        // Rewrite, conversion, or reload can replace a file during streaming.
        if (!completed.source.matches(FileStamp(path)))
            return false;
        result = std::move(completed.data);
        RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainDdsCompleted);
        return true;
    }

    // Keep finished pixels for visible patches to take on subsequent frames.
    // Drop results from views the camera has moved away from to free slots.
    constexpr std::size_t maxOutstanding = 4;
    // Keep one worker and one result slot available for new visible work.
    const auto resultLimit = maxOutstanding - (prefetch ? 1 : 0);
    if (pending.size() >= resultLimit) {
        for (auto stale = pending.begin(); stale != pending.end();) {
            if (now - stale->second.lastRequest > std::chrono::milliseconds(300) &&
                    stale->second.ready())
                stale = pending.erase(stale);
            else
                ++stale;
        }
        if (pending.size() >= resultLimit)
            return false;
    }

    const int workerLimit = stream.workers.maxThreadCount() - (prefetch ? 1 : 0);
    if (stream.workers.activeThreadCount() >= workerLimit)
        return false;
    std::packaged_task<StreamedDds()> job([path, previewSide, nativeDxt1, allowOddAce] {
        return readStreamed(path, previewSide, nativeDxt1, allowOddAce);
    });
    auto future = job.get_future();
    // tryStart does not add work to an unbounded queue when both workers are busy.
    if (stream.workers.tryStart(std::move(job))) {
        pending.emplace(key, PendingDds{std::move(future), now});
        RouteFrameProfile::count(RouteFrameProfile::Counter::TerrainDdsSubmitted);
    }
    return false;
}

void DdsLib::discardStreamed(const QString &path) {
    // A packaged_task owns its shared state until it exits. Discarding the
    // future never waits for that task and prevents an obsolete publication.
    auto &pending = terrainStream().pending;
    for (auto it = pending.lower_bound({path, 0, false, false});
            it != pending.end() && std::get<0>(it->first) == path;)
        it = pending.erase(it);
}

void DdsLib::publish(Texture &target, DdsTextureData result) {
    if (!result.pixels) {
        target.loaded = false;
        target.missing = result.missing;
        target.error = !result.missing;
        qWarning() << "DDS decode failed:" << target.pathid << "-" << result.error;
        return;
    }
    delete[] target.imageData;
    target.imageData = result.pixels.release();
    target.width = target.sourceWidth = result.width;
    target.height = target.sourceHeight = result.height;
    target.bytesPerPixel = result.components;
    target.bpp = result.components * 8;
    target.imageSize = result.width * result.height * result.components;
    target.type = result.components == 3 ? GL_RGB : GL_RGBA;
    target.saveDds24 = true;
    target.loaded = true;
    target.editable = true;
    target.missing = false;
    target.error = false;
}

void DdsLib::run() {
    if (!texture)
        return;
    RouteLoadTiming timing;
    DdsImage image;
    DdsTextureData result;
    RouteFrameProfile::Scope decode(RouteFrameProfile::Stage::DdsDecode, &texture->pathid);
    if (!DdsDecoder::decodeFile(texture->pathid, image, &result.error)) {
        result.missing = !QFileInfo::exists(texture->pathid);
        publish(*texture, std::move(result));
        return;
    }
    decode.finish();
    timing.mark("DDS read/decode");

    RouteFrameProfile::Scope publication(RouteFrameProfile::Stage::DdsPublish, &texture->pathid);
    publish(*texture, packPixels(image));
    publication.finish();
    timing.mark("DDS RGB/RGBA publication");
}
