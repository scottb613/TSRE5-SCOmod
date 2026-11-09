// Shared CPU BC1/BC2/BC3 codec. No OpenGL context or file container required.
#ifndef TSRE_DXT_CODEC_H
#define TSRE_DXT_CODEC_H

#include <QByteArray>
#include <QString>

namespace DxtCodec {
enum class Format { Dxt1, Dxt2, Dxt3, Dxt4, Dxt5 };
// Decode-only BC1 policy. Encoding and the default decoder retain upstream scaling.
enum class ColorExpansion { Scaled, LegacyBitReplication };
int blockBytes(Format format);
qint64 byteSize(int width, int height, Format format);
bool decode(const QByteArray &blocks, int width, int height, Format format, bool alpha,
            QByteArray &pixels, QString &error,
            ColorExpansion expansion = ColorExpansion::Scaled);
// The output must not alias blocks and must have exactly width*height*(alpha?4:3) bytes.
bool decodeInto(const QByteArray &blocks, int width, int height, Format format, bool alpha,
                unsigned char *pixels, qsizetype size, QString &error,
                ColorExpansion expansion = ColorExpansion::Scaled);
// Portable, deterministic fast endpoint-fit encoder; not an offline quality optimizer.
bool encode(const unsigned char *pixels, qsizetype size, int width, int height, int components,
            Format format, bool alpha, QByteArray &blocks, QString &error);
// DXT1 blocks as BC3 (DXT5) blocks of the same image, for a GPU API without
// BC1 with alpha (QRhi). Alpha is 255, or 0 where a three-colour block marks
// a pixel transparent; such a pixel takes the first endpoint's colour instead
// of black. A three-colour block's midpoint becomes BC3's one-third point, at
// most a sixth of the endpoints' distance off. False when the size is not
// whole blocks.
bool dxt1ToBc3(const QByteArray &dxt1, QByteArray &bc3);
// A BC3 image's fully transparent texels (alpha 0) take the colour of the
// opaque ones beside them, alpha kept: in a block the colour index of its
// nearest opaque texel, a block of transparent texels only one colour, the
// mean of the blocks beside it with opaque texels, or of the whole image's.
// Filtering at a cutout's edge then mixes in no colour of what was behind the
// cutout (DXT1 converted: the first endpoint's, often the white or blue an
// artist's tool fitted it to). Nothing when the size does not match.
void bleedBc3Transparent(QByteArray &bc3, int width, int height);
} // namespace DxtCodec
#endif
