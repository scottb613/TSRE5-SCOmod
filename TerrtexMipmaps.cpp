#include "TerrtexMipmaps.h"
#include "SaveProgress.h"
#include <QDataStream>
#include <QIODevice>
#include <QHash>
#include <array>
#include <cmath>

namespace {
float linear(float value) {
    return value <= 0.04045f ? value / 12.92f
        : std::pow((value + 0.055f) / 1.055f, 2.4f);
}
int srgb(float value) {
    value = qBound(0.0f, value, 1.0f);
    value = value <= 0.0031308f ? value * 12.92f
        : 1.055f * std::pow(value, 1.0f/2.4f) - 0.055f;
    return qBound(0, qRound(value * 255.0f), 255);
}
double lanczos(double x) {
    x = std::abs(x);
    if(x < 1.e-10) return 1;
    if(x >= 3) return 0;
    constexpr double pi = 3.14159265358979323846;
    return std::sin(pi*x) * std::sin(pi*x/3) / (pi*pi*x*x/3);
}
struct Tap { int index; float weight; };
using Filter = QVector<QVector<Tap>>;
Filter filter(int source, int destination) {
    Filter result(destination);
    const double ratio = double(source)/destination;
    for(int x = 0; x < destination; ++x) {
        const double center = (x+0.5)*ratio-0.5;
        double total = 0;
        for(int i = int(std::ceil(center-3*ratio)); i <= int(std::floor(center+3*ratio)); ++i) {
            const double weight = lanczos((i-center)/ratio);
            if(weight == 0) continue;
            result[x].append({(i%source+source)%source, float(weight)});
            total += weight;
        }
        for(Tap &tap : result[x]) tap.weight /= float(total);
    }
    return result;
}
QImage downsample(const QImage &source) {
    const int width = qMax(1, source.width()/2), height = qMax(1, source.height()/2);
    QImage target(width, height, QImage::Format_RGBA8888);
    if(target.isNull()) return {};
    const auto horizontal = filter(source.width(), width);
    const auto vertical = source.width() == source.height()
        ? horizontal : filter(source.height(), height);
    static const auto decode = [] {
        std::array<float, 256> values{};
        for(int i = 0; i < 256; ++i) values[i] = linear(i/255.0f);
        return values;
    }();
    // Cache only the rows needed by the current vertical filter, rather than
    // allocating a full floating-point copy of a potentially 8192-square image.
    QHash<int, QVector<float>> rows;
    QVector<const float *> verticalRows;
    for(int y = 0; y < height; ++y) {
        for(auto it = rows.begin(); it != rows.end();) {
            bool needed = false;
            for(const Tap &tap : vertical[y]) needed |= tap.index == it.key();
            if(!needed) it = rows.erase(it); else ++it;
        }
        for(const Tap &v : vertical[y]) {
            if(rows.contains(v.index)) continue;
            QVector<float> row(width*4, 0);
            const uchar *pixels = source.constScanLine(v.index);
            for(int x = 0; x < width; ++x) {
                for(const Tap &h : horizontal[x]) {
                    const uchar *p = pixels + h.index*4;
                    const float alpha = p[3]/255.0f;
                    for(int c = 0; c < 3; ++c) row[x*4+c] += decode[p[c]]*alpha*h.weight;
                    row[x*4+3] += alpha*h.weight;
                }
            }
            rows.insert(v.index, std::move(row));
        }
        // Resolve cached rows once per scanline, not once per tap per pixel.
        // No cache entries change until this output scanline is complete.
        verticalRows.clear();
        verticalRows.reserve(vertical[y].size());
        for(const Tap &v : vertical[y])
            verticalRows.append(rows.constFind(v.index).value().constData());
        uchar *out = target.scanLine(y);
        for(int x = 0; x < width; ++x) {
            float p[4] = {};
            for(qsizetype tap = 0; tap < vertical[y].size(); ++tap) {
                const float *row = verticalRows[tap] + x*4;
                const float weight = vertical[y][tap].weight;
                for(int c = 0; c < 4; ++c) p[c] += row[c]*weight;
            }
            // Filter premultiplied linear RGB, then return straight sRGB.
            for(int c = 0; c < 3; ++c) out[x*4+c] = p[3] > 1.e-7f ? srgb(p[c]/p[3]) : 0;
            out[x*4+3] = qBound(0, qRound(p[3]*255), 255);
        }
        SaveProgress::set(double(y+1)/height);
        if(y%16 == 0) SaveProgress::report(QString("Generating TERRTEX mip %1 x %2: row %3 of %4")
            .arg(width).arg(height).arg(y+1).arg(height));
    }
    return target;
}
}

bool TerrtexMipmaps::generate(const QImage &base, QVector<QImage> &levels, QString *error) {
    levels.clear();
    if(error) error->clear();
    // Open Rails requires square power-of-two ACE mipmaps. Never resize level 0.
    if(base.isNull() || base.width() != base.height() || base.width() < 2
            || base.width() > 8192 || (base.width() & (base.width()-1))) {
        if(error) *error = "ACE mipmaps require a square power-of-two texture from 2 to 8192 pixels. Repaint with Create mipmaps disabled to save this texture without resizing.";
        return false;
    }
    return generateDds(base,levels,error);
}

bool TerrtexMipmaps::generateDds(const QImage &base, QVector<QImage> &levels, QString *error) {
    levels.clear();
    if(error) error->clear();
    if(base.isNull() || base.width()>8192 || base.height()>8192) {
        if(error) *error="DDS mipmap dimensions must be from 1 to 8192 pixels.";
        return false;
    }
    levels.append(base.convertToFormat(QImage::Format_RGBA8888));
    if(levels.back().isNull()) {
        levels.clear();
        if(error) *error = "Could not allocate the TERRTEX base level.";
        return false;
    }
    double totalPixels = 0;
    for(int w=base.width(), h=base.height(); w>1 || h>1;) {
        w=qMax(1,w/2); h=qMax(1,h/2); totalPixels+=double(w)*h;
    }
    double completedPixels = 0;
    while(levels.back().width() > 1 || levels.back().height() > 1) {
        const double pixels = double(qMax(1,levels.back().width()/2))*qMax(1,levels.back().height()/2);
        SaveProgress::Range mip(completedPixels/totalPixels, qMin(1.0,(completedPixels+pixels)/totalPixels));
        QImage next = downsample(levels.back());
        if(next.isNull()) {
            levels.clear();
            if(error) *error = "Could not allocate the TERRTEX mip chain.";
            return false;
        }
        levels.append(std::move(next));
        completedPixels += pixels;
    }
    return true;
}

bool TerrtexMipmaps::serializeAce(const QImage &base, bool alpha, QByteArray &data, QString *error) {
    data.clear();
    QVector<QImage> levels;
    { SaveProgress::Range generation(0, 0.75);
      if(!generate(base, levels, error)) return false; }
    SaveProgress::Range encoding(0.75, 1);
    qsizetype encodedBytes = 16 + 152 + (alpha ? 5 : 3)*16;
    for(const QImage &level : levels) {
        const qsizetype rowBytes = level.width()*(alpha ? 4 : 3)
            + (alpha ? (level.width()+7)/8 : 0);
        encodedBytes += level.height()*(4 + rowBytes);
    }
    data.reserve(encodedBytes);
    QDataStream out(&data, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out.writeRawData("SIMISA@@@@@@@@@@", 16);
    out << quint32(1) << quint32(1) << quint32(base.width()) << quint32(base.height())
        << quint32(14) << quint32(alpha ? 5 : 3);
    for(int i = 0; i < 32; ++i) out << quint32(0);
    for(quint64 channel : {3, 4, 5}) out << quint64(8) << channel;
    if(alpha) out << quint64(1) << quint64(2) << quint64(8) << quint64(6);
    quint32 rows = 0;
    for(const QImage &level : levels) rows += level.height();
    // ACE offsets are relative to the payload after the 16-byte SIMISA header.
    quint32 offset = 152 + (alpha ? 5 : 3)*16 + rows*4;
    for(const QImage &level : levels) {
        const quint32 rowBytes = level.width()*(alpha ? 4 : 3) + (alpha ? (level.width()+7)/8 : 0);
        for(int y = 0; y < level.height(); ++y) { out << offset; offset += rowBytes; }
    }
    double pixelsDone = 0, pixelsTotal = 0;
    for(const QImage &level : levels) pixelsTotal += double(level.width())*level.height();
    for(const QImage &level : levels) {
        SaveProgress::Range mip(pixelsDone/pixelsTotal,
            (pixelsDone+double(level.width())*level.height())/pixelsTotal);
        pixelsDone += double(level.width())*level.height();
        const int maskBytes = alpha ? (level.width()+7)/8 : 0;
        QByteArray encodedRow(level.width()*(alpha ? 4 : 3)+maskBytes, '\0');
        for(int y = 0; y < level.height(); ++y) {
            SaveProgress::set(double(y)/level.height());
            if(y%32 == 0) SaveProgress::report(QString("Encoding uncompressed TERRTEX mip %1 x %2: row %3 of %4")
                .arg(level.width()).arg(level.height()).arg(y+1).arg(level.height()));
            const uchar *row = level.constScanLine(y);
            char *encoded = encodedRow.data();
            for(int c = 0; c < 3; ++c)
                for(int x = 0; x < level.width(); ++x)
                    encoded[c*level.width()+x] = char(row[x*4+c]);
            if(alpha) {
                for(int x = 0; x < level.width(); x += 8) {
                    quint8 mask = 0;
                    for(int b = 0; b < 8 && x+b < level.width(); ++b)
                        if(row[(x+b)*4+3] >= 128) mask |= 1 << (7-b);
                    encoded[level.width()*3+x/8] = char(mask);
                }
                for(int x = 0; x < level.width(); ++x)
                    encoded[level.width()*3+maskBytes+x] = char(row[x*4+3]);
            }
            out.writeRawData(encodedRow.constData(), int(encodedRow.size()));
        }
    }
    if(out.status() != QDataStream::Ok) {
        data.clear();
        if(error) *error = "Could not serialize uncompressed TERRTEX mipmaps.";
        return false;
    }
    return true;
}
