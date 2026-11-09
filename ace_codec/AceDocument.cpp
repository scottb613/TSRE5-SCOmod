#include "AceDocument.h"
#include "DxtCodec.h"
#include <QFile>
#include <QSaveFile>
#include <QHash>
#include <QtEndian>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz/miniz.h>

namespace {
// Parser-local view: unlike the private std-backed QByteArray::fromRawData,
// this never copies the whole uncompressed file. Retained records still own bytes.
struct ReadView {
    const char *bytes = nullptr;
    qsizetype length = 0;
    const char *constData() const { return bytes; }
    qsizetype size() const { return length; }
    QByteArray mid(qsizetype pos, qsizetype n = -1) const {
        return QByteArray(bytes + pos, n < 0 ? length - pos : n);
    }
    QByteArray left(qsizetype n) const { return mid(0, n); }
};
template <class Bytes> quint32 u32(const Bytes &b, qsizetype p) {
    return qFromLittleEndian<quint32>(b.constData() + p);
}
void put32(QByteArray &b, qsizetype p, quint32 v) { qToLittleEndian(v, b.data() + p); }
void append32(QByteArray &b, quint32 v) {
    char p[4];
    qToLittleEndian(v, p);
    b.append(p, 4);
}
void append64(QByteArray &b, quint64 v) {
    char p[8];
    qToLittleEndian(v, p);
    b.append(p, 8);
}
template <class Bytes> bool span(const Bytes &b, qint64 p, qint64 n) {
    return p >= 0 && n >= 0 && p <= b.size() && n <= b.size() - p;
}
bool fail(QString &e, const QString &s) {
    e = s;
    return false;
}
bool dxt(quint32 s) { return s >= 18 && s <= 22; }
DxtCodec::Format dxtFormat(quint32 s) { return static_cast<DxtCodec::Format>(s - 18); }
qint64 rowBytes(int w, const QVector<AceChannel> &channels) {
    qint64 n = 0;
    for (const auto &c : channels) {
        if (c.bits < 1 || c.bits > 64)
            return -1;
        n += (qint64(w) * c.bits + 7) / 8;
    }
    return n;
}
int levelCount(int w) {
    int n = 0;
    do {
        ++n;
        w >>= 1;
    } while (w);
    return n;
}
bool power2(int v) { return v > 0 && (v & (v - 1)) == 0; }
// Common RGB(A) profiles have no per-pixel dispatch. Keeping the component
// count constant lets the compiler vectorize planar-to-interleaved copies.
template <int Components>
void rgbRows(const unsigned char *src, unsigned char *dst, int w, int h, qint64 row,
             const qint64 *offsets, char *mask) {
    for (int y = 0; y < h; ++y) {
        const auto *line = src + y * row;
        auto *out = dst + qsizetype(y) * w * Components;
        for (int x = 0; x < w; ++x) {
            out[x * Components] = line[offsets[3] + x];
            out[x * Components + 1] = line[offsets[4] + x];
            out[x * Components + 2] = line[offsets[5] + x];
        }
        if constexpr (Components == 4) {
            if (offsets[6] >= 0)
                for (int x = 0; x < w; ++x)
                    out[x * 4 + 3] = line[offsets[6] + x];
            else if (offsets[2] >= 0)
                for (int x = 0; x < w; ++x)
                    out[x * 4 + 3] = ((line[offsets[2] + x / 8] >> (7 - x % 8)) & 1) * 255;
            else
                for (int x = 0; x < w; ++x)
                    out[x * 4 + 3] = 255;
        }
        if (mask)
            for (int x = 0; x < w; ++x)
                mask[qsizetype(y) * w + x] = (line[offsets[2] + x / 8] >> (7 - x % 8)) & 1;
    }
}
qint64 rawBytes(int w, int h, quint32 s) {
    if (dxt(s))
        return DxtCodec::byteSize(w, h, dxtFormat(s));
    if (s == 14 || s == 16 || s == 17)
        return qint64(w) * h * 2;
    return -1;
}
} // namespace

quint32 AceDocument::surface() const {
    return metadata.header.size() == 152 ? u32(metadata.header, 16) : 0;
}
quint32 AceDocument::options() const {
    return metadata.header.size() == 152 ? u32(metadata.header, 4) : 0;
}
bool AceDocument::hasAlpha() const {
    if (surface() == 16 || surface() == 17 || (surface() >= 19 && surface() <= 22))
        return true;
    for (const auto &c : metadata.channels)
        if (c.id == 2 || c.id == 6)
            return true;
    for (const auto &p : metadata.palettes)
        if (p.type == 8)
            return true;
    return false;
}

bool AceDocument::read(const QString &path, AceDocument &out, QString &error,
                       const AceReadOptions &o) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return fail(error, file.errorString());
    if (file.size() < 16 || file.size() > o.maxBytes)
        return fail(error, "ACE file size exceeds reader bounds");
    QByteArray bytes = file.readAll();
    if (bytes.size() != file.size())
        return fail(error, "Short ACE read");
    return parse(bytes, out, error, o);
}

bool AceDocument::parse(const QByteArray &file, AceDocument &out, QString &error,
                        const AceReadOptions &o) {
    if (o.maxBytes < 152 || o.maxBytes > std::numeric_limits<int>::max() || o.maxPixels < 1 ||
        o.maxDimension < 1)
        return fail(error, "Invalid ACE reader resource limits");
    if (file.size() < 16 || file.size() > o.maxBytes)
        return fail(error, "Invalid ACE envelope size");
    QByteArray inflated;
    ReadView body;
    AceDocument doc;
    doc.legacyDxt1Colors = o.legacyDxt1Colors;
    // Non-owning view is confined to this call; all retained records own their bytes.
    if (file.startsWith("SIMISA@@@@@@@@@@"))
        body = {file.constData() + 16, file.size() - 16};
    else if (file.startsWith("SIMISA@F") && file.mid(12, 4) == "@@@@") {
        const quint32 size = u32(file, 8);
        if (size < 152 || size > o.maxBytes || size > quint32(std::numeric_limits<int>::max()))
            return fail(error, "Inflated ACE size exceeds reader bounds");
        inflated.resize(size);
        body = {inflated.constData(), inflated.size()};
        mz_stream z{};
        z.next_in = reinterpret_cast<const unsigned char *>(file.constData() + 16);
        z.avail_in = file.size() - 16;
        z.next_out = reinterpret_cast<unsigned char *>(inflated.data());
        z.avail_out = size;
        if (mz_inflateInit(&z) != MZ_OK)
            return fail(error, "ACE inflater initialization failed");
        int result = mz_inflate(&z, MZ_FINISH);
        bool ok = result == MZ_STREAM_END && z.total_out == size && z.avail_in == 0;
        mz_inflateEnd(&z);
        if (!ok)
            return fail(error, "Invalid/truncated zlib ACE or inflated-length mismatch");
        doc.compressedEnvelope = true;
    } else
        return fail(error, "Unrecognized ACE SIMISA envelope");
    if (!span(body, 0, 152) || u32(body, 0) != 1)
        return fail(error, "Unsupported ACE version/header");
    quint32 w = u32(body, 8), h = u32(body, 12), count = u32(body, 20), palettes = u32(body, 24),
            flags = u32(body, 4), surface = u32(body, 16);
    if (!w || !h || w > quint32(o.maxDimension) || h > quint32(o.maxDimension) ||
        quint64(w) * h > quint64(o.maxPixels) || count > 64 || palettes > 64)
        return fail(error, "ACE dimensions/counts exceed reader bounds");
    if ((flags & 1) && (w != h || !power2(w)))
        return fail(error, "MSTS ACE mip chains require square power-of-two dimensions");
    doc.metadata.header = body.left(152);
    qint64 cursor = 152;
    for (quint32 i = 0; i < count; ++i, cursor += 16) {
        if (!span(body, cursor, 16))
            return fail(error, "Truncated ACE channel descriptor");
        AceChannel channel{qFromLittleEndian<quint64>(body.constData() + cursor),
                           qFromLittleEndian<quint64>(body.constData() + cursor + 8)};
        if (channel.bits < 1 || channel.bits > 64)
            return fail(error, "Unsupported ACE channel bit width");
        doc.metadata.channels.push_back(channel);
    }
    for (quint32 i = 0; i < palettes; ++i, cursor += 12) {
        if (!span(body, cursor, 12))
            return fail(error, "Truncated ACE palette descriptor");
        doc.metadata.palettes.push_back(
            {u32(body, cursor), u32(body, cursor + 4), u32(body, cursor + 8), {}});
    }
    for (auto &p : doc.metadata.palettes) {
        if (quint64(p.count) * p.stride > quint64(o.maxBytes))
            return fail(error, "ACE palette exceeds reader bounds");
        qint64 bytes = qint64(p.count) * p.stride;
        if (bytes > o.maxBytes || !span(body, cursor, bytes))
            return fail(error, "Truncated/oversized ACE palette");
        p.data = body.mid(cursor, bytes);
        cursor += bytes;
    }
    const int levels = (flags & 1) ? levelCount(w) : 1;
    qint64 entries = 0;
    for (int m = 0; m < levels; ++m)
        entries += (flags & 16) ? 1 : (h >> m);
    if (entries > 1000000 || !span(body, cursor, entries * 4))
        return fail(error, "Truncated/oversized ACE offset table");
    const qint64 dataStart = cursor + entries * 4;
    qint64 end = dataStart, totalPayload = 0;
    bool legacy = false;
    if (o.allowLegacyRgbOffsets && flags == 0 && surface == 14 && count == 3 && palettes == 0 &&
        doc.metadata.channels[0].id == 3 && doc.metadata.channels[1].id == 4 &&
        doc.metadata.channels[2].id == 5 && rowBytes(w, doc.metadata.channels) == qint64(w) * 3 &&
        h > 1 && u32(body, cursor) == dataStart && span(body, dataStart, qint64(w) * h * 3)) {
        legacy = true;
        for (quint32 y = 0; y < h; ++y)
            if (u32(body, cursor + y * 4) != dataStart + qint64(y) * w * 12) {
                legacy = false;
                break;
            }
        if (legacy)
            doc.warnings << "Recovered exact legacy TSRE RGB four-times row-offset stride";
    }
    for (int m = 0; m < levels; ++m) {
        AceLevel level;
        level.width = w >> m;
        level.height = h >> m;
        level.raw = (flags & 16) && !(dxt(surface) && level.width < 4);
        qint64 row = rowBytes(level.width, doc.metadata.channels), bytes = 0;
        if (flags & 16) {
            qint64 at = u32(body, cursor);
            cursor += 4;
            if (at < dataStart)
                return fail(error, "ACE image offset points into header/table");
            if (level.raw) {
                if (!span(body, at, 4))
                    return fail(error, "Missing raw ACE length");
                bytes = u32(body, at);
                at += 4;
                const qint64 expected = rawBytes(level.width, level.height, surface);
                if (expected >= 0 && bytes != expected)
                    return fail(error, "Raw ACE length disagrees with surface/dimensions");
            } else
                bytes = row * level.height;
            if (!span(body, at, bytes))
                return fail(error, "Raw ACE payload exceeds file");
            level.data = body.mid(at, bytes);
            end = std::max(end, at + bytes);
        } else {
            bytes = row * level.height;
            if (bytes > o.maxBytes || row <= 0)
                return fail(error, "Oversized/empty ACE row layout");
            // Copy once for canonical sequential files; gather only nonsequential rows.
            const qint64 first = legacy ? dataStart : u32(body, cursor);
            bool sequential = true;
            for (int y = 0; y < level.height; ++y) {
                qint64 at =
                    legacy ? dataStart + qint64(y) * row : u32(body, cursor + qint64(y) * 4);
                if (at < dataStart || !span(body, at, row))
                    return fail(error, "ACE row offset exceeds image bounds");
                end = std::max(end, at + row);
                sequential &= at == first + y * row;
            }
            if (sequential)
                level.data = body.mid(first, bytes);
            else {
                level.data.resize(bytes);
                for (int y = 0; y < level.height; ++y)
                    memcpy(level.data.data() + y * row,
                           body.constData() + u32(body, cursor + qint64(y) * 4), row);
            }
            cursor += qint64(level.height) * 4;
        }
        totalPayload += bytes;
        if (totalPayload > o.maxBytes)
            return fail(error, "Aggregate ACE payload exceeds reader bounds");
        doc.levels.push_back(std::move(level));
    }
    doc.metadata.trailing = body.mid(end);
    if (o.retainOriginal)
        doc.original = QByteArray(file.constData(), file.size());
    out = std::move(doc);
    error.clear();
    return true;
}

bool AceDocument::decode(int mip, QByteArray &out, int &components, QString &error,
                         QByteArray *mask) const {
    if (mip < 0 || mip >= levels.size())
        return fail(error, "ACE mip index out of range");
    const auto &level = levels[mip];
    if (level.width <= 0 || level.height <= 0 ||
        qint64(level.width) * level.height > 64 * 1024 * 1024)
        return fail(error, "ACE decode dimensions exceed bounds");
    QByteArray pixels(qsizetype(level.width) * level.height * (hasAlpha() ? 4 : 3),
                      Qt::Uninitialized);
    if (!decodeInto(mip, reinterpret_cast<unsigned char *>(pixels.data()), pixels.size(),
                    components, error, mask))
        return false;
    out = std::move(pixels);
    return true;
}

bool AceDocument::decodeThumbnail(int mip, int dw, int dh, QByteArray &out,
                                  QString &error) const {
    if (mip < 0 || mip >= levels.size())
        return fail(error, "ACE mip index out of range");
    const auto &level = levels[mip];
    const int sw = level.width, sh = level.height;
    if (sw <= 0 || sh <= 0 || sw > 16384 || sh > 16384 ||
        qint64(sw) * sh > 64 * 1024 * 1024 || dw < 1 || dh < 1 ||
        dw > 1024 || dh > 1024 || dw > sw || dh > sh)
        return fail(error, "ACE thumbnail dimensions exceed bounds");
    const bool blocks = level.raw && dxt(surface());
    const int stripHeight = blocks ? 4 : 1;
    const qint64 stride = blocks ? ((qint64(sw) + 3) / 4) * DxtCodec::blockBytes(dxtFormat(surface()))
                                : level.raw ? qint64(sw) * 2 : rowBytes(sw, metadata.channels);
    const qint64 rows = blocks ? (sh + 3) / 4 : sh;
    if (stride <= 0 || level.data.size() != stride * rows)
        return fail(error, "Invalid ACE thumbnail payload length");
    // Bound metadata copies in the Qt-free implementation as well as pixel scratch.
    if (metadata.channels.size() > 64 || metadata.palettes.size() > 1 ||
        (!metadata.palettes.isEmpty() && metadata.palettes[0].data.size() > 1024))
        return fail(error, "Unsupported ACE thumbnail metadata");
    AceDocument strip;
    strip.legacyDxt1Colors = legacyDxt1Colors;
    strip.metadata.header = metadata.header;
    strip.metadata.channels = metadata.channels;
    strip.metadata.palettes = metadata.palettes;
    strip.levels.push_back({sw, 1, level.raw, {}});
    const int components = hasAlpha() ? 4 : 3;
    QByteArray decoded(qsizetype(sw) * stripHeight * components, Qt::Uninitialized);
    QByteArray result(qsizetype(dw) * dh * 4, Qt::Uninitialized);
    std::vector<double> sums(std::size_t(dw) * 4);
    int cachedStrip = -1;
    const double xscale = double(sw) / dw;
    for (int y = 0; y < dh; ++y) {
        std::fill(sums.begin(), sums.end(), 0.0);
        const double top = double(y) * sh / dh, bottom = double(y + 1) * sh / dh;
        for (int sy = int(top); sy < std::min(sh, int(std::ceil(bottom))); ++sy) {
            const int current = sy / stripHeight;
            if (current != cachedStrip) {
                auto &part = strip.levels[0];
                part.height = std::min(stripHeight, sh - current * stripHeight);
                part.data = level.data.mid(qsizetype(current * stride), qsizetype(stride));
                int actualComponents = 0;
                if (!strip.decodeInto(0, reinterpret_cast<unsigned char *>(decoded.data()),
                                      qsizetype(sw) * part.height * components, actualComponents, error))
                    return false;
                cachedStrip = current;
            }
            const double wy = std::min(bottom, double(sy + 1)) - std::max(top, double(sy));
            const auto *line = reinterpret_cast<const unsigned char *>(decoded.constData()) +
                               qsizetype(sy % stripHeight) * sw * components;
            for (int x = 0; x < dw; ++x) {
                const double left = double(x) * sw / dw, right = double(x + 1) * sw / dw;
                auto *sum = sums.data() + x * 4;
                for (int sx = int(left); sx < std::min(sw, int(std::ceil(right))); ++sx) {
                    const double weight = wy * (std::min(right, double(sx + 1)) - std::max(left, double(sx)));
                    const auto *p = line + sx * components;
                    const double a = components == 4 ? p[3] : 255;
                    for (int k = 0; k < 3; ++k) sum[k] += p[k] * a / 255.0 * weight;
                    sum[3] += a * weight;
                }
            }
        }
        const double area = (bottom - top) * xscale;
        auto *dst = reinterpret_cast<unsigned char *>(result.data()) + qsizetype(y) * dw * 4;
        for (int x = 0; x < dw; ++x)
            for (int k = 0; k < 4; ++k)
                dst[x * 4 + (k == 3 ? 3 : 2 - k)] = static_cast<unsigned char>(
                    std::clamp(std::lround(sums[x * 4 + k] / area), 0L, 255L));
    }
    out = std::move(result);
    error.clear();
    return true;
}

bool AceDocument::decodeInto(int mip, unsigned char *dst, qsizetype size, int &components,
                             QString &error, QByteArray *mask) const {
    if (mip < 0 || mip >= levels.size())
        return fail(error, "ACE mip index out of range");
    const auto &level = levels[mip];
    const quint32 s = surface();
    const int w = level.width, h = level.height;
    const bool alpha = hasAlpha();
    components = alpha ? 4 : 3;
    if (w <= 0 || h <= 0 || qint64(w) * h > 64 * 1024 * 1024)
        return fail(error, "ACE decode dimensions exceed bounds");
    if (!dst || size != qsizetype(w) * h * components)
        return fail(error, "ACE decode destination size mismatch");
    if (mask)
        mask->clear();
    if (level.raw && dxt(s)) {
        if (!DxtCodec::decodeInto(level.data, w, h, dxtFormat(s), alpha, dst, size, error,
                legacyDxt1Colors ? DxtCodec::ColorExpansion::LegacyBitReplication
                                 : DxtCodec::ColorExpansion::Scaled))
            return false;
        if (mask && s == 18 && alpha) {
            mask->resize(qsizetype(w) * h);
            char *bits = mask->data();
            for (qsizetype i = 0; i < qsizetype(w) * h; ++i)
                bits[i] = dst[i * 4 + 3] != 0;
        }
        return true;
    }
    const auto *src = reinterpret_cast<const unsigned char *>(level.data.constData());
    if (level.raw) {
        if ((s != 14 && s != 16 && s != 17) || level.data.size() != qint64(w) * h * 2)
            return fail(error, "Unsupported raw ACE surface");
        if (mask && s == 16)
            mask->resize(qsizetype(w) * h);
        for (qsizetype i = 0; i < qsizetype(w) * h; ++i) {
            const quint16 v = qFromLittleEndian<quint16>(src + i * 2);
            auto *p = dst + i * components;
            if (s == 14) {
                p[0] = (v >> 11) * 255 / 31;
                p[1] = ((v >> 5) & 63) * 255 / 63;
                p[2] = (v & 31) * 255 / 31;
                if (alpha)
                    p[3] = 255;
            } else if (s == 16) {
                p[0] = ((v >> 10) & 31) * 255 / 31;
                p[1] = ((v >> 5) & 31) * 255 / 31;
                p[2] = (v & 31) * 255 / 31;
                p[3] = (v >> 15) * 255;
                if (mask)
                    (*mask)[i] = v >> 15;
            } else {
                p[0] = ((v >> 8) & 15) * 17;
                p[1] = ((v >> 4) & 15) * 17;
                p[2] = (v & 15) * 17;
                p[3] = (v >> 12) * 17;
            }
        }
    } else {
        const qint64 row = rowBytes(w, metadata.channels);
        if (row <= 0 || level.data.size() != row * h)
            return fail(error, "Invalid ACE planar payload length");
        qint64 offsets[7];
        std::fill(std::begin(offsets), std::end(offsets), -1);
        qint64 cursor = 0;
        int indexBits = 0;
        for (const auto &c : metadata.channels) {
            if (c.id > 6 || c.id == 0 || offsets[c.id] >= 0 ||
                (c.id == 2 ? c.bits != 1
                           : (c.id == 1 ? (c.bits != 1 && c.bits != 2 && c.bits != 4 && c.bits != 8)
                                        : c.bits != 8)))
                return fail(error, "Unsupported or duplicate ACE planar channel");
            offsets[c.id] = cursor;
            if (c.id == 1)
                indexBits = c.bits;
            cursor += (qint64(w) * c.bits + 7) / 8;
        }
        const AcePalette *palette = metadata.palettes.isEmpty() ? nullptr : &metadata.palettes[0];
        if (palette) {
            if (metadata.palettes.size() != 1 || (s != 4 && s != 12) || offsets[1] < 0 ||
                palette->count > 256 || !palette->count ||
                !((palette->type == 7 && palette->stride == 3) ||
                  (palette->type == 8 && palette->stride == 4)) ||
                palette->data.size() != qint64(palette->count) * palette->stride)
                return fail(error, "Unsupported ACE indexed/palette profile");
        } else if (offsets[3] < 0 || offsets[4] < 0 || offsets[5] < 0 || offsets[1] >= 0)
            return fail(error, "ACE planar image lacks supported RGB channels");
        if (mask && offsets[2] >= 0)
            mask->resize(qsizetype(w) * h);
        if (!palette) {
            char *bits = mask && !mask->isEmpty() ? mask->data() : nullptr;
            if (alpha)
                rgbRows<4>(src, dst, w, h, row, offsets, bits);
            else
                rgbRows<3>(src, dst, w, h, row, offsets, bits);
            error.clear();
            return true;
        }
        for (int y = 0; y < h; ++y) {
            const auto *line = src + y * row;
            for (int x = 0; x < w; ++x) {
                auto *p = dst + (qsizetype(y) * w + x) * components;
                if (palette) {
                    const int index = (line[offsets[1] + (x * indexBits) / 8] >>
                                       (8 - indexBits - (x * indexBits) % 8)) &
                                      ((1 << indexBits) - 1);
                    if (index >= int(palette->count))
                        return fail(error, "ACE palette index out of range");
                    const auto *entry = reinterpret_cast<const unsigned char *>(
                        palette->data.constData() + index * palette->stride);
                    p[0] = entry[0];
                    p[1] = entry[1];
                    p[2] = entry[2];
                    if (alpha)
                        p[3] = palette->stride == 4 ? entry[3] : 255;
                } else {
                    p[0] = line[offsets[3] + x];
                    p[1] = line[offsets[4] + x];
                    p[2] = line[offsets[5] + x];
                    if (alpha)
                        p[3] = 255;
                }
                int bit = offsets[2] >= 0 ? (line[offsets[2] + x / 8] >> (7 - x % 8)) & 1 : 1;
                if (alpha && offsets[6] >= 0)
                    p[3] = line[offsets[6] + x];
                else if (alpha && offsets[2] >= 0)
                    p[3] = bit ? p[3] : 0; // Indexed mask gates palette alpha, not replaces it.
                if (mask && !mask->isEmpty())
                    (*mask)[qsizetype(y) * w + x] = bit;
            }
        }
    }
    error.clear();
    return true;
}

bool AceDocument::serialize(QByteArray &out, bool zipped, QString &error) const {
    if (metadata.header.size() != 152 || levels.isEmpty() || levels.size() > 32 ||
        metadata.channels.size() > 64 || metadata.palettes.size() > 64)
        return fail(error, "Invalid ACE document header/counts");
    const int w = levels[0].width, h = levels[0].height;
    if (w <= 0 || h <= 0 || qint64(w) * h > 64 * 1024 * 1024 ||
        ((options() & 1) && (w != h || !power2(w))) ||
        levels.size() != ((options() & 1) ? levelCount(w) : 1))
        return fail(error, "Invalid ACE document mip chain");
    QByteArray body = metadata.header;
    put32(body, 0, 1);
    put32(body, 8, w);
    put32(body, 12, h);
    put32(body, 20, metadata.channels.size());
    put32(body, 24, metadata.palettes.size());
    for (const auto &c : metadata.channels) {
        append64(body, c.bits);
        append64(body, c.id);
    }
    for (const auto &p : metadata.palettes) {
        if (quint64(p.data.size()) != quint64(p.count) * p.stride)
            return fail(error, "Invalid ACE palette length");
        append32(body, p.count);
        append32(body, p.stride);
        append32(body, p.type);
    }
    for (const auto &p : metadata.palettes)
        body += p.data;
    const qsizetype table = body.size();
    qint64 entries = 0, total = body.size() + metadata.trailing.size();
    for (const auto &l : levels) {
        entries += (options() & 16) ? 1 : l.height;
        total += l.data.size() + 4;
    }
    total += entries * 4;
    if (entries > 1000000 || total > 512LL * 1024 * 1024)
        return fail(error, "Serialized ACE exceeds writer bounds");
    body.reserve(total);
    body.append(QByteArray(entries * 4, '\0'));
    qsizetype at = table;
    for (int m = 0; m < levels.size(); ++m) {
        const auto &l = levels[m];
        const qint64 row = rowBytes(l.width, metadata.channels);
        const bool raw = (options() & 16) && !(dxt(surface()) && l.width < 4);
        qint64 expected = raw ? rawBytes(l.width, l.height, surface()) : row * l.height;
        if (l.width != (w >> m) || l.height != (h >> m) || raw != l.raw || (!raw && row <= 0) ||
            (expected >= 0 && l.data.size() != expected))
            return fail(error, "ACE level disagrees with header/layout");
        if (options() & 16) {
            put32(body, at, body.size());
            at += 4;
            if (raw)
                append32(body, l.data.size());
        } else
            for (int y = 0; y < l.height; ++y) {
                put32(body, at, body.size() + y * row);
                at += 4;
            }
        body += l.data;
    }
    body += metadata.trailing;
    if (zipped) {
        QByteArray file("SIMISA@F", 8);
        append32(file, body.size());
        file += "@@@@";
        file += qCompress(body, 6).mid(4);
        out = std::move(file);
    } else
        out = QByteArray("SIMISA@@@@@@@@@@", 16) + body;
    error.clear();
    return true;
}

bool AceDocument::write(const QString &path, bool zipped, QString &error) const {
    QByteArray bytes;
    if (!serialize(bytes, zipped, error))
        return false;
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        return fail(error, file.errorString());
    return true;
}

bool AceDocument::fromPixels(const unsigned char *input, qsizetype size, int w, int h,
                             int components, const AceWriteOptions &o, AceDocument &out,
                             QString &error) {
    if (!input || w <= 0 || h <= 0 || w > 16384 || h > 16384 || qint64(w) * h > 64 * 1024 * 1024 ||
        (components != 3 && components != 4) || size != qsizetype(w) * h * components ||
        (o.mipmaps && (w != h || !power2(w))) ||
        (!o.mask.isEmpty() && o.mask.size() != qint64(w) * h) ||
        (!o.headerTemplate.isEmpty() && o.headerTemplate.size() != 152))
        return fail(error, "Invalid ACE writer pixels/dimensions/options");
    const auto encoding = o.encoding;
    quint32 surface = 14;
    bool raw = false, alpha = false, mask = false, indexed = false;
    switch (encoding) {
    case AceEncoding::Rgb:
        break;
    case AceEncoding::Mask:
        surface = 16;
        mask = true;
        break;
    case AceEncoding::Rgba:
        surface = 17;
        mask = alpha = true;
        break;
    case AceEncoding::Rgb565:
        raw = true;
        break;
    case AceEncoding::Argb1555:
        surface = 16;
        raw = mask = true;
        break;
    case AceEncoding::Argb4444:
        surface = 17;
        raw = mask = alpha = true;
        break;
    case AceEncoding::Dxt1:
        surface = 18;
        raw = true;
        break;
    case AceEncoding::Dxt1Mask:
        surface = 18;
        raw = mask = true;
        break;
    case AceEncoding::Dxt2:
        surface = 19;
        raw = mask = alpha = true;
        break;
    case AceEncoding::Dxt3:
        surface = 20;
        raw = mask = alpha = true;
        break;
    case AceEncoding::Dxt4:
        surface = 21;
        raw = mask = alpha = true;
        break;
    case AceEncoding::Dxt5:
        surface = 22;
        raw = mask = alpha = true;
        break;
    case AceEncoding::IndexedRgb:
        surface = 4;
        indexed = true;
        break;
    case AceEncoding::IndexedRgba:
        surface = 12;
        indexed = mask = alpha = true;
        break;
    default:
        return fail(error, "Unknown ACE writer encoding");
    }
    AceDocument doc;
    doc.metadata.header = o.headerTemplate.isEmpty() ? QByteArray(152, '\0') : o.headerTemplate;
    auto &header = doc.metadata.header;
    put32(header, 0, 1);
    put32(header, 4, (u32(header, 4) & ~17u) | (o.mipmaps ? 1 : 0) | (raw ? 16 : 0));
    put32(header, 8, w);
    put32(header, 12, h);
    put32(header, 16, surface);
    put32(header, 0x74, 0);
    put32(header, 0x7c, dxt(surface) ? (alpha ? 17 : mask ? 16 : 14) : 0);
    put32(header, 0x80, dxt(surface) ? surface : 0);
    doc.metadata.channels = {{8, 3}, {8, 4}, {8, 5}};
    if (mask)
        doc.metadata.channels.push_back({1, 2});
    if (alpha)
        doc.metadata.channels.push_back({8, 6});
    if (indexed) {
        doc.metadata.channels = {{8, 1}};
        // MSTS's surface-12 converters consume an index plane AND a 1-bit mask.
        // Palette alpha remains independent; the default mask must not threshold it.
        if (mask)
            doc.metadata.channels.push_back({1, 2});
    }
    QHash<quint32, int> indices;
    AcePalette palette;
    palette.count = 256;
    palette.stride = alpha ? 4 : 3;
    palette.type = alpha ? 8 : 7;
    if (indexed)
        palette.data = QByteArray(palette.count * palette.stride, '\0');
    // Base pixels remain borrowed only during this call. Encoded levels own their
    // storage; generated lower mip levels replace this view with owning arrays.
    QByteArray pixels = QByteArray::fromRawData(reinterpret_cast<const char *>(input), size);
    QByteArray binaryMask = o.mask;
    const int count = o.mipmaps ? levelCount(w) : 1;
    for (int mip = 0; mip < count; ++mip) {
        AceLevel l;
        l.width = w >> mip;
        l.height = h >> mip;
        l.raw = raw && !(dxt(surface) && l.width < 4);
        const int lw = l.width, lh = l.height;
        const auto *p = reinterpret_cast<const unsigned char *>(pixels.constData());
        if (l.raw && dxt(surface)) {
            QByteArray masked;
            if (mask && !alpha && !binaryMask.isEmpty()) {
                masked.resize(qsizetype(lw) * lh * 4);
                for (qsizetype i = 0; i < qsizetype(lw) * lh; ++i) {
                    memcpy(masked.data() + i * 4, p + i * components, 3);
                    masked[i * 4 + 3] = binaryMask[i] ? char(255) : 0;
                }
            }
            if (!DxtCodec::encode(
                    masked.isEmpty() ? p
                                     : reinterpret_cast<const unsigned char *>(masked.constData()),
                    masked.isEmpty() ? pixels.size() : masked.size(), lw, lh,
                    masked.isEmpty() ? components : 4, dxtFormat(surface), mask, l.data, error))
                return false;
        } else if (l.raw) {
            l.data.resize(qsizetype(lw) * lh * 2);
            for (qsizetype i = 0; i < qsizetype(lw) * lh; ++i) {
                const auto *c = p + i * components;
                int a = components == 4 ? c[3] : 255;
                quint16 v = 0;
                if (surface == 14)
                    v = ((c[0] * 31 + 127) / 255) << 11 | ((c[1] * 63 + 127) / 255) << 5 |
                        (c[2] * 31 + 127) / 255;
                if (surface == 16)
                    v = ((binaryMask.isEmpty() ? a >= 128 : binaryMask[i] != 0) << 15) |
                        ((c[0] * 31 + 127) / 255) << 10 | ((c[1] * 31 + 127) / 255) << 5 |
                        (c[2] * 31 + 127) / 255;
                if (surface == 17)
                    v = ((a + 8) / 17) << 12 | ((c[0] + 8) / 17) << 8 | ((c[1] + 8) / 17) << 4 |
                        (c[2] + 8) / 17;
                qToLittleEndian(v, l.data.data() + i * 2);
            }
        } else {
            const qsizetype row = rowBytes(lw, doc.metadata.channels);
            l.data = QByteArray(row * lh, '\0');
            for (int y = 0; y < lh; ++y) {
                auto *dst = reinterpret_cast<unsigned char *>(l.data.data() + y * row);
                qsizetype off = 0;
                for (const auto &channel : doc.metadata.channels) {
                    for (int x = 0; x < lw; ++x) {
                        const qsizetype i = qsizetype(y) * lw + x;
                        const auto *c = p + i * components;
                        const int a = components == 4 ? c[3] : 255;
                        if (channel.id == 1) {
                            quint32 key = quint32(c[0]) | (quint32(c[1]) << 8) |
                                          (quint32(c[2]) << 16) | (quint32(alpha ? a : 255) << 24);
                            auto found = indices.constFind(key);
                            int index;
                            if (found == indices.cend()) {
                                index = indices.size();
                                if (index >= 256)
                                    return fail(error,
                                                "Indexed ACE requires <=256 distinct colors across "
                                                "all mips (no implicit quantization)");
                                indices.insert(key, index);
                                char *entry = palette.data.data() + index * palette.stride;
                                memcpy(entry, c, 3);
                                if (alpha)
                                    entry[3] = a;
                            } else
                                index = found.value();
                            dst[off + x] = index;
                        } else if (channel.id == 2) {
                            if (binaryMask.isEmpty() ? (indexed || a >= 128) : binaryMask[i] != 0)
                                dst[off + x / 8] |= 1 << (7 - x % 8);
                        } else
                            dst[off + x] = channel.id == 6 ? a : c[channel.id - 3];
                    }
                    off += (qsizetype(lw) * channel.bits + 7) / 8;
                }
            }
        }
        doc.levels.push_back(std::move(l));
        if (mip + 1 < count) {
            const int nw = lw / 2, nh = lh / 2;
            QByteArray next(qsizetype(nw) * nh * components, Qt::Uninitialized), nextMask;
            if (!binaryMask.isEmpty())
                nextMask.resize(qsizetype(nw) * nh);
            for (int y = 0; y < nh; ++y)
                for (int x = 0; x < nw; ++x) {
                    for (int k = 0; k < components; ++k) {
                        int sum = 0;
                        for (int dy = 0; dy < 2; ++dy)
                            for (int dx = 0; dx < 2; ++dx)
                                sum +=
                                    p[(qsizetype(2 * y + dy) * lw + 2 * x + dx) * components + k];
                        next[(qsizetype(y) * nw + x) * components + k] = (sum + 2) / 4;
                    }
                    if (!nextMask.isEmpty())
                        nextMask[qsizetype(y) * nw + x] = binaryMask[qsizetype(2 * y) * lw + 2 * x];
                }
            pixels = std::move(next);
            binaryMask = std::move(nextMask);
        }
    }
    if (indexed)
        doc.metadata.palettes.push_back(std::move(palette));
    put32(header, 20, doc.metadata.channels.size());
    put32(header, 24, doc.metadata.palettes.size());
    doc.compressedEnvelope = o.zlib;
    out = std::move(doc);
    error.clear();
    return true;
}
