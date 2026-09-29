// TSRE GenX. Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "OrtsTurntableConfig.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStringConverter>
#include <QTemporaryDir>
#include <iostream>
#include <cstdlib>

static void require(bool ok, const char *message) {
    if(!ok) { std::cerr << message << '\n'; std::exit(1); }
}
static QByteArray read(const QString &path) {
    QFile file(path); require(file.open(QIODevice::ReadOnly), "read fixture"); return file.readAll();
}
static void write(const QString &path, const QByteArray &bytes) {
    QFile file(path); require(file.open(QIODevice::WriteOnly), "write fixture");
    require(file.write(bytes) == bytes.size(), "write all bytes");
}
int main(int argc, char **argv) {
    QCoreApplication application(argc, argv);
    for(const QString &reference : {QString("A1t42mTurntable.s"),
            QString("../../ROUTES/Test Route/SHAPES/a1t42mturntable.s"),
            QString("..\\..\\ROUTES\\Test Route\\SHAPES\\A1t42mTurntable.s")})
        require(OrtsTurntableConfig::is42mShapeReference(reference), "global or route-local reference");
    for(const QString &reference : {QString("Other.s"), QString("A1t42mTurntable.s.bak"),
            QString("A1t42mTurntable.s/Other.s"), QString()})
        require(!OrtsTurntableConfig::is42mShapeReference(reference), "reject unrelated reference");
    OrtsTurntableConfig::Entry entry;
    entry.worldFile = "w-000001+000002.w"; entry.uid = 12;
    entry.shapeIndex = 24864; entry.animation = "TRACKPIECE";
    entry.z = 21; entry.diameter = 42;
    QByteArray output, duplicate; QString error; bool present = false;
    require(OrtsTurntableConfig::append({}, entry, output, present, error), "new config");
    require(!present && output.startsWith(QByteArray::fromHex("fffe")), "new UTF16LE BOM");
    QStringDecoder decode(QStringConverter::Utf16LE);
    const QString generated = decode(output.mid(2));
    require(generated.startsWith("\r\n1\r\n") && generated.contains("ZOffset ( 21 )") &&
            generated.contains("TrackShapeIndex ( 24864 )"), "OR header and bridge data");
    require(OrtsTurntableConfig::append(output, entry, duplicate, present, error) &&
            present && duplicate == output, "idempotent activation");

    const QByteArray existing = "\n2\n_INFO ( \"preserve (notes)\" )\n"
        "Turntable ( WFile ( \"w-000001+000002.w\" ) UID ( 9 ) MaxAngle ( 40 ) )\n"
        "Transfertable ( WFile ( \"w+000003-000004.w\" ) UID ( 12 ) Length ( 30 ) )\n";
    require(OrtsTurntableConfig::append(existing, entry, output, present, error) && !present,
            "append alongside transfer table and same UID on different tile");
    QByteArray changedCount = existing; changedCount[1] = '3';
    require(output.startsWith(changedCount), "preserve existing text exactly except count");
    require(output.count("Turntable (") == 2, "one new entry only");
    entry.uid = 9;
    require(OrtsTurntableConfig::append(existing, entry, output, present, error) &&
            present && output == existing, "preserve custom existing limits");
    entry.uid = 12;
    QStringEncoder utf16(QStringConverter::Utf16BE);
    const QByteArray encoded = utf16(QString::fromUtf8(existing));
    const QByteArray be = QByteArray::fromHex("feff") + encoded;
    require(OrtsTurntableConfig::append(be, entry, output, present, error) &&
            output.startsWith(QByteArray::fromHex("feff")), "preserve UTF16BE");
    for(const QByteArray &bad : {QByteArray("\n2\n"), QByteArray("\n1\nTurntable ("),
            QByteArray("\n0\nBogus ( 4 )"), QByteArray("\n1\nTurntable ( UID ( 7 ) )"),
            QByteArray("\n0\n_INFO ( \"unclosed )"), QByteArray("\xff\xfe\x61", 3),
            QByteArray("\n0\n\xc3", 4)}) {
        require(!OrtsTurntableConfig::append(bad, entry, output, present, error) &&
                output.isEmpty() && !error.isEmpty(), "reject malformed config");
    }
    entry.animation = "bad\"name";
    require(!OrtsTurntableConfig::append({}, entry, output, present, error), "reject unsafe name");
    entry.animation = "TRACKPIECE";

    auto transfer = entry;
    transfer.kind = OrtsTurntableConfig::Entry::Kind::Transfer;
    transfer.length = 30;
    transfer.diameter = 0;
    require(OrtsTurntableConfig::append({}, transfer, output, present, error), "new transfer config");
    QStringDecoder transferDecoder(QStringConverter::Utf16LE);
    const QString transferText = transferDecoder(output.mid(2));
    require(transferText.contains("Transfertable (") && transferText.contains("Length ( 30 )") &&
            !transferText.contains("Diameter"), "transfer uses Length instead of Diameter");
    require(OrtsTurntableConfig::append(output, transfer, duplicate, present, error) && present &&
            duplicate == output, "repeat transfer activation preserves entry");
    require(!OrtsTurntableConfig::append(output, entry, duplicate, present, error),
            "same identity cannot silently change table type");
    auto secondTable = entry;
    ++secondTable.uid;
    require(OrtsTurntableConfig::append(output, secondTable, duplicate, present, error) && !present,
            "append turntable alongside transfer");
    transfer.length = 0;
    require(!OrtsTurntableConfig::append({}, transfer, output, present, error), "reject zero transfer length");

    QTemporaryDir directory; require(directory.isValid(), "temporary route");
    require(OrtsTurntableConfig::activate(directory.path(), entry, present, error) && !present,
            "create file atomically");
    const QString folder = directory.path()+"/OpenRails";
    const QString path = folder+"/turntables.dat";
    const QByteArray first = read(path);
    require(OrtsTurntableConfig::activate(directory.path(), entry, present, error) && present &&
            read(path) == first, "second activation leaves file unchanged");
    ++entry.uid;
    require(OrtsTurntableConfig::activate(directory.path(), entry, present, error), "append file");
    const QStringList backups = QDir(folder).entryList({"*.bak"}, QDir::Files);
    require(backups.size() == 1 && read(folder+"/"+backups[0]) == first, "exact original backup");
    write(path, "\n17\n");
    require(!OrtsTurntableConfig::activate(directory.path(), entry, present, error) &&
            read(path) == "\n17\n", "malformed existing file never overwritten");
    write(path, {});
    require(!OrtsTurntableConfig::activate(directory.path(), entry, present, error) &&
            read(path).isEmpty(), "existing empty file is not treated as a new file");
    std::cout << "OrtsTurntableConfigTest passed\n";
}
