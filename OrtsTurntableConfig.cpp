// TSRE GenX. Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "OrtsTurntableConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStringConverter>
#include <QUuid>
#include <QVector>
#include <cmath>

bool OrtsTurntableConfig::is42mShapeReference(QString fileName) {
    fileName.replace('\\', '/');
    return fileName.section('/', -1).compare("A1t42mTurntable.s", Qt::CaseInsensitive) == 0;
}

namespace {
struct Token { QString value; qsizetype start, end; bool quoted; };
bool tokenize(const QString &text, QVector<Token> &tokens) {
    for(qsizetype i = 0; i < text.size();) {
        if(text[i].isSpace()) { ++i; continue; }
        if(text.mid(i, 2) == "//") {
            while(i < text.size() && text[i] != '\n') ++i;
            continue;
        }
        const qsizetype start = i;
        if(text[i] == '"') {
            ++i;
            const qsizetype content = i;
            while(i < text.size() && text[i] != '"') ++i;
            if(i == text.size()) return false;
            tokens.push_back({text.mid(content, i-content), start, i+1, true});
            ++i;
        } else if(text[i] == '(' || text[i] == ')') {
            tokens.push_back({text.mid(i, 1), i, i+1, false}); ++i;
        } else {
            while(i < text.size() && !text[i].isSpace() && text[i] != '(' && text[i] != ')' && text[i] != '"') ++i;
            tokens.push_back({text.mid(start, i-start), start, i, false});
        }
    }
    return true;
}
bool symbol(const Token &t, const char *s) { return !t.quoted && t.value == s; }
}

bool OrtsTurntableConfig::append(const QByteArray &original, const Entry &entry,
        QByteArray &result, bool &alreadyPresent, QString &error) {
    result.clear(); alreadyPresent = false; error.clear();
    const bool transfer = entry.kind == Entry::Kind::Transfer;
    const double span = transfer ? entry.length : entry.diameter;
    const QRegularExpression worldPattern("^w[+-][0-9]{6}[+-][0-9]{6}\\.w$",
                                         QRegularExpression::CaseInsensitiveOption);
    if(!worldPattern.match(entry.worldFile).hasMatch() || entry.shapeIndex < 0 ||
            !QRegularExpression("^[A-Za-z_][A-Za-z0-9_]*$").match(entry.animation).hasMatch() ||
            !std::isfinite(entry.x) || !std::isfinite(entry.y) || !std::isfinite(entry.z) ||
            !std::isfinite(span) || span <= 0) {
        error = "The moving-table definition is invalid."; return false;
    }
    auto encoding = QStringConverter::Utf8;
    QByteArray bom;
    if(original.startsWith(QByteArray::fromHex("fffe"))) {
        encoding = QStringConverter::Utf16LE; bom = original.left(2);
    } else if(original.startsWith(QByteArray::fromHex("feff"))) {
        encoding = QStringConverter::Utf16BE; bom = original.left(2);
    } else if(original.startsWith(QByteArray::fromHex("efbbbf"))) bom = original.left(3);
    QStringDecoder decoder(encoding);
    QString text = decoder(original.mid(bom.size()));
    if(decoder.hasError()) { error = "Cannot safely read the existing turntable file encoding."; return false; }
    if(!original.isEmpty()) {
        // Streaming decoders can retain an incomplete final code unit without
        // reporting an error yet. Require a lossless round trip before editing.
        QStringEncoder check(encoding);
        const QByteArray roundTrip = check(text);
        if(check.hasError() || bom + roundTrip != original) {
            error = "The existing turntable file cannot be preserved losslessly."; return false;
        }
    }
    if(original.isEmpty()) { text = "\r\n0\r\n"; encoding = QStringConverter::Utf16LE; bom = QByteArray::fromHex("fffe"); }
    QVector<Token> tokens;
    if(!tokenize(text, tokens) || tokens.isEmpty()) {
        error = "The existing turntable file is malformed."; return false;
    }
    bool countOk = false;
    const int declared = tokens[0].value.toInt(&countOk);
    if(!countOk || declared < 0 || tokens[0].quoted) {
        error = "The existing turntable count is invalid."; return false;
    }
    int count = 0;
    for(qsizetype i = 1; i < tokens.size();) {
        const QString kind = tokens[i].value.toLower();
        if(tokens[i].quoted || i+1 >= tokens.size() || !symbol(tokens[i+1], "(")) {
            error = "The existing turntable file has an invalid block."; return false;
        }
        const bool table = kind == "turntable" || kind == "transfertable";
        if(!table && kind != "_info" && kind != "skip" && kind != "comment") {
            error = "The existing turntable file contains an unsupported block."; return false;
        }
        const qsizetype begin = i+2;
        qsizetype end = begin; int depth = 1;
        for(; end < tokens.size() && depth; ++end) {
            if(symbol(tokens[end], "(")) ++depth;
            if(symbol(tokens[end], ")")) --depth;
        }
        if(depth) { error = "The existing turntable file has an unclosed block."; return false; }
        if(table) {
            ++count;
            QString world; unsigned int uid = 0; bool uidFound = false, worldFound = false;
            int fieldDepth = 0;
            for(qsizetype j = begin; j+3 < end; ++j) {
                if(symbol(tokens[j], "(")) { ++fieldDepth; continue; }
                if(symbol(tokens[j], ")")) { --fieldDepth; continue; }
                if(fieldDepth || tokens[j].quoted || !symbol(tokens[j+1], "(") || !symbol(tokens[j+3], ")")) continue;
                if(tokens[j].value.compare("WFile", Qt::CaseInsensitive) == 0) {
                    if(worldFound) { error = "Duplicate WFile in an existing entry."; return false; }
                    world = tokens[j+2].value; worldFound = true;
                }
                if(tokens[j].value.compare("UID", Qt::CaseInsensitive) == 0) {
                    if(uidFound) { error = "Duplicate UID in an existing entry."; return false; }
                    uid = tokens[j+2].value.toUInt(&uidFound);
                    if(!uidFound) { error = "Invalid UID in an existing entry."; return false; }
                }
            }
            if(!worldFound || !uidFound) { error = "An existing moving table is missing WFile or UID."; return false; }
            if(world.compare(entry.worldFile, Qt::CaseInsensitive) == 0 && uid == entry.uid) {
                if(kind != (transfer ? "transfertable" : "turntable")) {
                    error = "This object already has a different moving-table type in turntables.dat.";
                    return false;
                }
                alreadyPresent = true;
            }
        }
        i = end;
    }
    if(count != declared) { error = "The existing turntable count does not match its entries."; return false; }
    if(alreadyPresent) { result = original; return true; }
    const QString nl = text.contains("\r\n") ? "\r\n" : "\n";
    text.replace(tokens[0].start, tokens[0].end-tokens[0].start, QString::number(count+1));
    QString block = QString("\n%9 (\n    WFile ( \"%1\" )\n    UID ( %2 )\n"
            "    XOffset ( %3 )\n    YOffset ( %4 )\n    ZOffset ( %5 )\n"
            "    TrackShapeIndex ( %6 )\n    Animation ( \"%7\" )\n    %10 ( %8 )\n)\n")
        .arg(entry.worldFile).arg(entry.uid).arg(entry.x, 0, 'g', 12)
        .arg(entry.y, 0, 'g', 12).arg(entry.z, 0, 'g', 12)
        .arg(entry.shapeIndex).arg(entry.animation).arg(span, 0, 'g', 12)
        .arg(transfer ? "Transfertable" : "Turntable").arg(transfer ? "Length" : "Diameter");
    block.replace("\n", nl); text += block;
    QStringEncoder encoder(encoding);
    const QByteArray encoded = encoder(text);
    result = bom + encoded;
    if(encoder.hasError()) { error = "Cannot preserve the turntable file encoding."; result.clear(); return false; }
    return true;
}

bool OrtsTurntableConfig::activate(const QString &routeDirectory, const Entry &entry,
        bool &alreadyPresent, QString &error) {
    alreadyPresent = false;
    const QString directory = routeDirectory + "/OpenRails";
    const QString path = directory + "/turntables.dat";
    QFile input(path); QByteArray original;
    const bool existed = input.exists();
    if(existed) {
        if(!input.open(QIODevice::ReadOnly)) { error = input.errorString(); return false; }
        original = input.readAll();
        if(input.error() != QFileDevice::NoError) { error = input.errorString(); return false; }
        input.close();
        if(original.isEmpty()) { error = "The existing turntable file is empty."; return false; }
    }
    QByteArray updated;
    if(!append(original, entry, updated, alreadyPresent, error)) return false;
    if(alreadyPresent) return true;
    if(!QDir().mkpath(directory)) { error = "Cannot create the route OpenRails folder."; return false; }
    if(existed) {
        const QString backup = path + "." + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".bak";
        if(!QFile::copy(path, backup)) { error = "Cannot back up the existing turntable file."; return false; }
    }
    QSaveFile output(path);
    if(!output.open(QIODevice::WriteOnly) || output.write(updated) != updated.size() || !output.commit()) {
        error = output.errorString(); return false;
    }
    return true;
}
