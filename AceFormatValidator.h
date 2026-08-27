#ifndef ACEFORMATVALIDATOR_H
#define ACEFORMATVALIDATOR_H

#include <QtGlobal>
#include <QString>

struct AceFormatLayout {
    int width = 0;
    int height = 0;
    int bitsPerPixel = 0;
    int channelType = -1;
    int compression = 0;
    qsizetype dataOffset = 0;
    qsizetype encodedBytes = 0;
    qsizetype decodedBytes = 0;
};

class AceFormatValidator {
public:
    static bool inspect(const unsigned char *data, qsizetype length,
                        AceFormatLayout &layout, QString *error = NULL);
};

#endif
