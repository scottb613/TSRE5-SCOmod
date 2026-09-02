// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include <QByteArray>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>
#endif

namespace {

void setUtf16Encoding(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf16);
#else
    stream.setCodec("UTF-16");
#endif
}

void setUtf8Encoding(QTextStream &stream)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
}

}

int main(int argc, char *argv[])
{
    if (argc != 2)
        return 1;

    const QString expectedUtf8 =
        QString::fromUtf8("user=sample; route=Zażółć; currency=€\n");
    QByteArray utf8Bytes = expectedUtf8.toUtf8();
    QTextStream utf8Input(&utf8Bytes, QIODevice::ReadOnly);
    setUtf8Encoding(utf8Input);
    if (utf8Input.readAll() != expectedUtf8)
        return 2;

    const QString outputPath = QString::fromLocal8Bit(argv[1]);
    QFile output(outputPath);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text))
        return 3;

    QTextStream utf16Output(&output);
    setUtf16Encoding(utf16Output);
    utf16Output.setGenerateByteOrderMark(true);
    utf16Output << "SIMISA@@@@@@@@@@JINX0a0t______\n";
    utf16Output << "Tr_Activity ( \""
                << QString::fromUtf8("Zażółć €")
                << "\" )\n";
    utf16Output.flush();

    if (utf16Output.status() != QTextStream::Ok)
        return 4;

    output.close();

    if (!output.open(QIODevice::ReadOnly))
        return 5;
    const QByteArray result = output.readAll();

    if (result.size() < 2
            || static_cast<unsigned char>(result.at(0)) != 0xff
            || static_cast<unsigned char>(result.at(1)) != 0xfe) {
        return 6;
    }

    return 0;
}
