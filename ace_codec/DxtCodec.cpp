#include "DxtCodec.h"
#include <QtAlgorithms>
#include <QtEndian>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <vector>

namespace {
using Pixel = std::array<int, 4>;
quint16 pack565(const Pixel &p) {
    return quint16(((p[0] * 31 + 127) / 255) << 11 | ((p[1] * 63 + 127) / 255) << 5 |
                   (p[2] * 31 + 127) / 255);
}
Pixel unpack565(quint16 v) {
    return {int(v >> 11) * 255 / 31, int((v >> 5) & 63) * 255 / 63, int(v & 31) * 255 / 31, 255};
}
std::array<Pixel, 4> colors(quint16 a, quint16 b, bool bc1,
                          DxtCodec::ColorExpansion expansion = DxtCodec::ColorExpansion::Scaled) {
    std::array<Pixel, 4> c{unpack565(a), unpack565(b), Pixel{}, Pixel{}};
    // Match GenX's existing ACE CPU reader only when explicitly requested.
    // Keep unpack565 and encoder calls unchanged, including DXT2-5 behavior.
    if (bc1 && expansion == DxtCodec::ColorExpansion::LegacyBitReplication) {
        for (int i = 0; i < 2; ++i) {
            const quint16 v = i == 0 ? a : b;
            const int r = v >> 11, g = (v >> 5) & 63, blue = v & 31;
            c[i] = {(r << 3) | (r >> 2), (g << 2) | (g >> 4),
                    (blue << 3) | (blue >> 2), 255};
        }
    }
    for (int k = 0; k < 3; ++k) {
        c[2][k] = bc1 && a <= b ? (c[0][k] + c[1][k]) / 2 : (2 * c[0][k] + c[1][k]) / 3;
        c[3][k] = bc1 && a <= b ? 0 : (c[0][k] + 2 * c[1][k]) / 3;
    }
    c[2][3] = 255;
    c[3][3] = bc1 && a <= b ? 0 : 255;
    return c;
}
std::array<int, 8> alphas(int a, int b) {
    std::array<int, 8> v{a, b, 0, 0, 0, 0, 0, 0};
    if (a > b)
        for (int i = 1; i <= 6; ++i)
            v[i + 1] = ((7 - i) * a + i * b) / 7;
    else {
        for (int i = 1; i <= 4; ++i)
            v[i + 1] = ((5 - i) * a + i * b) / 5;
        v[6] = 0;
        v[7] = 255;
    }
    return v;
}
bool dimensions(int w, int h) { return w > 0 && h > 0 && qint64(w) * h <= 64 * 1024 * 1024; }
} // namespace

namespace DxtCodec {
int blockBytes(Format f) {
    return f == Format::Dxt1 ? 8 : f >= Format::Dxt2 && f <= Format::Dxt5 ? 16 : 0;
}
qint64 byteSize(int w, int h, Format f) {
    return dimensions(w, h) ? ((qint64(w) + 3) / 4) * ((qint64(h) + 3) / 4) * blockBytes(f) : 0;
}

bool decode(const QByteArray &data, int w, int h, Format f, bool alpha, QByteArray &out,
            QString &error, ColorExpansion expansion) {
    if (!dimensions(w, h) || !blockBytes(f) || data.size() != byteSize(w, h, f)) {
        error = "Invalid DXT dimensions, format or payload size";
        return false;
    }
    const int components = alpha ? 4 : 3;
    QByteArray result(qsizetype(w) * h * components, Qt::Uninitialized);
    if (!decodeInto(data, w, h, f, alpha, reinterpret_cast<unsigned char *>(result.data()),
                    result.size(), error, expansion))
        return false;
    out = std::move(result);
    return true;
}

bool decodeInto(const QByteArray &data, int w, int h, Format f, bool alpha, unsigned char *dst,
                qsizetype size, QString &error, ColorExpansion expansion) {
    const int components = alpha ? 4 : 3;
    if (!dimensions(w, h) || !blockBytes(f) || data.size() != byteSize(w, h, f) || !dst ||
        size != qsizetype(w) * h * components) {
        error = "Invalid DXT dimensions, format, payload or destination size";
        return false;
    }
    const auto *src = reinterpret_cast<const unsigned char *>(data.constData());
    for (int by = 0; by < h; by += 4)
        for (int bx = 0; bx < w; bx += 4, src += blockBytes(f)) {
            const auto *color = src + (f == Format::Dxt1 ? 0 : 8);
            const auto c = colors(qFromLittleEndian<quint16>(color),
                                  qFromLittleEndian<quint16>(color + 2), f == Format::Dxt1, expansion);
            const quint32 selectors = qFromLittleEndian<quint32>(color + 4);
            const auto a = alphas(src[0], src[1]);
            quint64 indices = 0;
            if (f == Format::Dxt4 || f == Format::Dxt5)
                for (int i = 0; i < 6; ++i)
                    indices |= quint64(src[2 + i]) << (8 * i);
            for (int y = 0; y < 4 && by + y < h; ++y)
                for (int x = 0; x < 4 && bx + x < w; ++x) {
                    const int i = y * 4 + x;
                    Pixel p = c[(selectors >> (2 * i)) & 3];
                    if (f == Format::Dxt2 || f == Format::Dxt3)
                        p[3] = ((src[i / 2] >> ((i % 2) * 4)) & 15) * 17;
                    if (f == Format::Dxt4 || f == Format::Dxt5)
                        p[3] = a[(indices >> (3 * i)) & 7];
                    if (f == Format::Dxt2 || f == Format::Dxt4)
                        for (int k = 0; k < 3; ++k)
                            p[k] = p[3] ? std::min(255, (p[k] * 255 + p[3] / 2) / p[3]) : 0;
                    unsigned char *pixel = dst + (qsizetype(by + y) * w + bx + x) * components;
                    for (int k = 0; k < components; ++k)
                        pixel[k] = static_cast<unsigned char>(p[k]);
                }
        }
    error.clear();
    return true;
}

bool encode(const unsigned char *pixels, qsizetype size, int w, int h, int components, Format f,
            bool alpha, QByteArray &out, QString &error) {
    if (!dimensions(w, h) || !blockBytes(f) || (components != 3 && components != 4) || !pixels ||
        size != qsizetype(w) * h * components) {
        error = "Invalid DXT input pixels";
        return false;
    }
    QByteArray result(byteSize(w, h, f), '\0');
    auto *dst = reinterpret_cast<unsigned char *>(result.data());
    for (int by = 0; by < h; by += 4)
        for (int bx = 0; bx < w; bx += 4, dst += blockBytes(f)) {
            std::array<Pixel, 16> block;
            Pixel lo{255, 255, 255, 255}, hi{0, 0, 0, 0};
            bool transparent = false;
            for (int y = 0; y < 4; ++y)
                for (int x = 0; x < 4; ++x) {
                    const auto *src = pixels + (qsizetype(std::min(h - 1, by + y)) * w +
                                                std::min(w - 1, bx + x)) *
                                                   components;
                    Pixel p{src[0], src[1], src[2], components == 4 ? src[3] : 255};
                    transparent |= alpha && p[3] < 128;
                    if (f == Format::Dxt2 || f == Format::Dxt4)
                        for (int k = 0; k < 3; ++k)
                            p[k] = (p[k] * p[3] + 127) / 255;
                    block[y * 4 + x] = p;
                    for (int k = 0; k < 4; ++k) {
                        lo[k] = std::min(lo[k], p[k]);
                        hi[k] = std::max(hi[k], p[k]);
                    }
                }
            const bool cutout = f == Format::Dxt1 && transparent;
            // Fit actual colors rather than opposite corners of an RGB box:
            // e.g. a red/green block must not become a black/yellow ramp.
            Pixel mean{};
            int visible = 0;
            for (const auto &p : block)
                if (!cutout || p[3] >= 128) {
                    for (int k = 0; k < 3; ++k)
                        mean[k] += p[k];
                    ++visible;
                }
            if (visible)
                for (int k = 0; k < 3; ++k)
                    mean[k] /= visible;
            auto farthest = [&](const Pixel &origin) {
                Pixel result = origin;
                int maximum = -1;
                for (const auto &p : block)
                    if (!cutout || p[3] >= 128) {
                        int distance = 0;
                        for (int k = 0; k < 3; ++k) {
                            const int d = p[k] - origin[k];
                            distance += d * d;
                        }
                        if (distance > maximum) {
                            maximum = distance;
                            result = p;
                        }
                    }
                return result;
            };
            const Pixel endpoint = farthest(mean);
            quint16 a = pack565(endpoint), b = pack565(farthest(endpoint));
            if (cutout) {
                if (a > b)
                    std::swap(a, b);
            } else {
                if (a < b)
                    std::swap(a, b);
                if (a == b) {
                    if (b > 0)
                        a = b, b--;
                    else
                        a = 1;
                }
            }
            const auto c = colors(a, b, f == Format::Dxt1);
            quint32 selectors = 0;
            for (int i = 0; i < 16; ++i) {
                int best = 0, distance = std::numeric_limits<int>::max();
                if (cutout && block[i][3] < 128)
                    best = 3;
                else
                    for (int j = 0; j < (cutout ? 3 : 4); ++j) {
                        int d = 0;
                        for (int k = 0; k < 3; ++k) {
                            int v = block[i][k] - c[j][k];
                            d += v * v;
                        }
                        if (d < distance) {
                            distance = d;
                            best = j;
                        }
                    }
                selectors |= quint32(best) << (2 * i);
            }
            auto *color = dst + (f == Format::Dxt1 ? 0 : 8);
            qToLittleEndian(a, color);
            qToLittleEndian(b, color + 2);
            qToLittleEndian(selectors, color + 4);
            if (f == Format::Dxt2 || f == Format::Dxt3)
                for (int i = 0; i < 16; ++i)
                    dst[i / 2] |= ((block[i][3] + 8) / 17) << ((i % 2) * 4);
            if (f == Format::Dxt4 || f == Format::Dxt5) {
                dst[0] = hi[3];
                dst[1] = lo[3];
                const auto values = alphas(hi[3], lo[3]);
                quint64 bits = 0;
                for (int i = 0; i < 16; ++i) {
                    int best = 0, distance = 256;
                    for (int j = 0; j < 8; ++j) {
                        int d = std::abs(block[i][3] - values[j]);
                        if (d < distance) {
                            distance = d;
                            best = j;
                        }
                    }
                    bits |= quint64(best) << (3 * i);
                }
                for (int i = 0; i < 6; ++i)
                    dst[2 + i] = (bits >> (8 * i)) & 255;
            }
        }
    out = std::move(result);
    error.clear();
    return true;
}

bool dxt1ToBc3(const QByteArray &dxt1, QByteArray &bc3) {
    if (dxt1.size() % 8 != 0)
        return false;
    QByteArray result(dxt1.size() * 2, Qt::Uninitialized);
    const auto *src = reinterpret_cast<const unsigned char *>(dxt1.constData());
    auto *dst = reinterpret_cast<unsigned char *>(result.data());
    for (qsizetype n = dxt1.size() / 8; n > 0; --n, src += 8, dst += 16) {
        quint32 selectors = qFromLittleEndian<quint32>(src + 4);
        // Alpha endpoints 255 and 0: index 0 opaque, index 1 transparent.
        quint64 alphaIndices = 0;
        if (qFromLittleEndian<quint16>(src) <= qFromLittleEndian<quint16>(src + 2)) {
            quint32 colorIndices = 0;
            for (int i = 0; i < 16; ++i) {
                const quint32 s = (selectors >> (2 * i)) & 3;
                if (s == 3)
                    alphaIndices |= quint64(1) << (3 * i);
                else
                    colorIndices |= s << (2 * i);
            }
            selectors = colorIndices;
        }
        dst[0] = 255;
        dst[1] = 0;
        for (int i = 0; i < 6; ++i)
            dst[2 + i] = (alphaIndices >> (8 * i)) & 255;
        std::copy(src, src + 4, dst + 8);
        qToLittleEndian(selectors, dst + 12);
    }
    bc3 = std::move(result);
    return true;
}

void bleedBc3Transparent(QByteArray &bc3, int width, int height) {
    const int bw = (width + 3) / 4, bh = (height + 3) / 4;
    if (bw <= 0 || bh <= 0 || bc3.size() != qsizetype(bw) * bh * 16)
        return;
    auto *data = reinterpret_cast<unsigned char *>(bc3.data());
    const qsizetype count = qsizetype(bw) * bh;
    // Each block's transparent texels (bit i). Blocks as dxt1ToBc3 writes
    // them (alpha endpoints 255 and 0) need no alpha palette: index 1 is the
    // only transparent one, every index 0 an opaque block.
    // The mean of the colour endpoints of blocks with opaque texels colours
    // the transparent blocks no such block touches.
    std::vector<quint16> clear(static_cast<size_t>(count), 0);
    long sum[3] = {0, 0, 0}, endpoints = 0;
    bool any = false;
    for (qsizetype b = 0; b < count; ++b) {
        const unsigned char *block = data + b * 16;
        quint16 mask = 0;
        quint64 indices = 0;
        for (int i = 0; i < 6; ++i)
            indices |= quint64(block[2 + i]) << (8 * i);
        if (block[0] == 255 && block[1] == 0) {
            for (int i = 0; indices != 0 && i < 16; ++i)
                if (((indices >> (3 * i)) & 7) == 1)
                    mask |= quint16(1u << i);
        } else if (block[0] != 255 || indices != 0) {
            const std::array<int, 8> a = alphas(block[0], block[1]);
            for (int i = 0; i < 16; ++i)
                if (a[(indices >> (3 * i)) & 7] == 0)
                    mask |= quint16(1u << i);
        }
        clear[size_t(b)] = mask;
        any = any || mask != 0;
        if (mask != 0xFFFF) {
            for (int e = 0; e < 2; ++e) {
                const Pixel c = unpack565(qFromLittleEndian<quint16>(block + 8 + 2 * e));
                for (int k = 0; k < 3; ++k)
                    sum[k] += c[k];
            }
            endpoints += 2;
        }
    }
    if (!any || endpoints == 0)
        return;
    // Partly transparent blocks: the colour index of an opaque texel next to
    // each transparent one, else two texels away, else any in the block.
    static const auto around = [] {
        std::array<std::array<quint16, 16>, 2> masks{};
        for (int i = 0; i < 16; ++i)
            for (int j = 0; j < 16; ++j) {
                const int d = std::max(std::abs((i & 3) - (j & 3)), std::abs((i >> 2) - (j >> 2)));
                for (int r = 0; r < 2; ++r)
                    if (j != i && d <= r + 1)
                        masks[size_t(r)][size_t(i)] |= quint16(1u << j);
            }
        return masks;
    }();
    for (qsizetype b = 0; b < count; ++b) {
        const quint16 mask = clear[size_t(b)];
        if (mask == 0 || mask == 0xFFFF)
            continue;
        const quint16 opaque = quint16(~mask);
        unsigned char *block = data + b * 16;
        const quint32 selectors = qFromLittleEndian<quint32>(block + 12);
        quint32 result = selectors;
        for (int i = 0; i < 16; ++i) {
            if (!(mask & (1u << i)))
                continue;
            quint16 from = opaque & around[0][size_t(i)];
            if (from == 0)
                from = opaque & around[1][size_t(i)];
            if (from == 0)
                from = opaque;
            const int j = qCountTrailingZeroBits(from);
            result = (result & ~(3u << (2 * i))) | (((selectors >> (2 * j)) & 3u) << (2 * i));
        }
        qToLittleEndian(result, block + 12);
    }
    // Transparent blocks: one colour, the mean of the blocks beside them with
    // opaque texels (all their texels now an opaque one's colour), else the
    // endpoints' mean. They read only such blocks: in place is safe.
    const Pixel fallback{int(sum[0] / endpoints), int(sum[1] / endpoints), int(sum[2] / endpoints), 255};
    for (qsizetype b = 0; b < count; ++b) {
        if (clear[size_t(b)] != 0xFFFF)
            continue;
        const int bx = int(b % bw), by = int(b / bw);
        long near[3] = {0, 0, 0}, n = 0;
        const int around[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (const auto &step : around) {
            const int nx = bx + step[0], ny = by + step[1];
            if (nx < 0 || ny < 0 || nx >= bw || ny >= bh)
                continue;
            const qsizetype nb = qsizetype(ny) * bw + nx;
            if (clear[size_t(nb)] == 0xFFFF)
                continue;
            const unsigned char *block = data + nb * 16;
            const std::array<Pixel, 4> c = colors(qFromLittleEndian<quint16>(block + 8),
                                                  qFromLittleEndian<quint16>(block + 10), false);
            const quint32 selectors = qFromLittleEndian<quint32>(block + 12);
            for (int j = 0; j < 16; ++j) {
                const Pixel &p = c[(selectors >> (2 * j)) & 3];
                for (int k = 0; k < 3; ++k)
                    near[k] += p[k];
                ++n;
            }
        }
        const Pixel colour = n == 0 ? fallback
                                    : Pixel{int(near[0] / n), int(near[1] / n), int(near[2] / n), 255};
        unsigned char *block = data + b * 16;
        const quint16 packed = pack565(colour);
        qToLittleEndian(packed, block + 8);
        qToLittleEndian(packed, block + 10);
        qToLittleEndian<quint32>(0, block + 12);
    }
}
} // namespace DxtCodec
