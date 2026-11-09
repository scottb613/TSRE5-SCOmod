// ACE container/CPU codec. No renderer, Game settings, or OpenGL dependency.
#ifndef TSRE_ACE_DOCUMENT_H
#define TSRE_ACE_DOCUMENT_H

#include <QByteArray>
#include <QStringList>
#include <QVector>

struct AceChannel {
    quint64 bits = 0;
    quint64 id = 0;
};
struct AcePalette {
    quint32 count = 0, stride = 0, type = 0;
    QByteArray data;
};
struct AceLevel {
    int width = 0, height = 0;
    bool raw = false; // False also for DXT 2x2/1x1 planar tails.
    QByteArray data;  // Normalized contiguous rows, or raw bytes without size DWORD.
};
struct AceMetadata {
    QByteArray header; // All 152 bytes, including unresolved fields.
    QVector<AceChannel> channels;
    QVector<AcePalette> palettes;
    QByteArray trailing; // Includes any Photoshop resources; opt-in in Texture.
};
struct AceReadOptions {
    qint64 maxBytes = 512LL * 1024 * 1024;
    qint64 maxPixels = 64LL * 1024 * 1024;
    int maxDimension = 16384;
    bool retainOriginal = false;
    bool allowLegacyRgbOffsets = true; // ONLY the exact historical TSRE 4x stride.
    // Decode-only opt-in for GenX ACE/DXT1 pixel parity; does not alter encoding.
    bool legacyDxt1Colors = false;
};
enum class AceEncoding {
    Rgb,
    Mask,
    Rgba,
    Rgb565,
    Argb1555,
    Argb4444,
    Dxt1,
    Dxt1Mask,
    Dxt2,
    Dxt3,
    Dxt4,
    Dxt5,
    IndexedRgb,
    IndexedRgba
};
struct AceWriteOptions {
    AceEncoding encoding = AceEncoding::Rgb;
    bool mipmaps = false;
    bool zlib = false;
    QByteArray headerTemplate; // Optional 152-byte unknown-field template.
    QByteArray mask;           // Optional unpacked base-level mask, one 0/1 byte per pixel.
};

class AceDocument {
  public:
    AceMetadata metadata;
    QVector<AceLevel> levels;
    QStringList warnings;
    bool compressedEnvelope = false;

    quint32 surface() const;
    quint32 options() const;
    bool hasAlpha() const;
    static bool read(const QString &path, AceDocument &out, QString &error,
                     const AceReadOptions &options = {});
    static bool parse(const QByteArray &file, AceDocument &out, QString &error,
                      const AceReadOptions &options = {});
    bool decode(int level, QByteArray &pixels, int &components, QString &error,
                QByteArray *independentMask = nullptr) const;
    // Destination must not alias document storage; contents may be partial on failure.
    bool decodeInto(int level, unsigned char *pixels, qsizetype size, int &components,
                    QString &error, QByteArray *independentMask = nullptr) const;
    // Area-filter a selected level to top-down premultiplied BGRA. No upscaling.
    // Output dimensions must be 1..1024 and no larger than the source level.
    // Decodes at most four source rows at once; never allocates full-size RGB(A).
    // Encoded payloads remain owned by this document. Output is unchanged on failure.
    bool decodeThumbnail(int level, int width, int height, QByteArray &bgra,
                         QString &error) const;
    bool serialize(QByteArray &file, bool zlib, QString &error) const;
    bool write(const QString &path, bool zlib, QString &error) const;
    // Lossless original-envelope copy is explicit; it does not serialize metadata edits.
    const QByteArray &originalBytes() const { return original; }
    static bool fromPixels(const unsigned char *pixels, qsizetype size, int width, int height,
                           int components, const AceWriteOptions &options, AceDocument &out,
                           QString &error);

  private:
    QByteArray original;
    bool legacyDxt1Colors = false;
};
#endif
