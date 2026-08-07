/*
 * Native DDS decoder used by the Qt6 port.
 *
 * Qt does not build its DDS image plugin by default.  TSRE therefore decodes
 * the legacy rolling-stock formats itself instead of passing DDS files to
 * QImage.
 */

#ifndef DDSDECODER_H
#define DDSDECODER_H

#include <QByteArray>
#include <QString>

struct DdsImage {
    int width = 0;
    int height = 0;
    QByteArray rgba;
};

class DdsDecoder {
public:
    static bool decodeFile(
        const QString &path, DdsImage &image, QString *error = nullptr);
    static bool decode(
        const QByteArray &fileData, DdsImage &image, QString *error = nullptr);
};

#endif
