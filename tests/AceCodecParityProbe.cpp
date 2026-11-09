// TSRE GenX regression support. GNU GPL v3 or later. See ../LICENSE.md.
#include "AceDocument.h"
#include "AceInputValidation.h"
#include "DxtCodec.h"
#include "AceLib.h"
#include "AceLegacyReader.h"
#include "AceFormatValidator.h"
#include "ReadFile.h"
#include "FileBuffer.h"
#include "TerrtexMipmaps.h"
#include "Game.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDirIterator>
#include <QDir>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QOpenGLFunctions>
#include <QtEndian>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>

namespace {
constexpr qint64 InputLimit = 64LL * 1024 * 1024;
int failures = 0, checks = 0;
void check(bool ok, const QString &label) {
    ++checks;
    if (!ok) { ++failures; std::cerr << "FAIL: " << label.toStdString() << '\n'; }
}
void word(QByteArray &b, qsizetype at, quint32 v) { qToLittleEndian(v, b.data() + at); }
void descriptor(QByteArray &b, quint64 bits, quint64 id) {
    char entry[16];
    qToLittleEndian(bits, entry); qToLittleEndian(id, entry + 8); b.append(entry, 16);
}
QByteArray header(int w, int h, int channels, int flags = 0, int surface = 14) {
    QByteArray b(168, '\0');
    std::copy_n("SIMISA@@@@@@@@@@", 16, b.data());
    word(b, 16, 1); word(b, 20, flags); word(b, 24, w); word(b, 28, h);
    word(b, 32, surface); word(b, 36, channels);
    return b;
}
QByteArray pixels(int w, int h, int components) {
    QByteArray b(w * h * components, '\0');
    for (int y = 0; y < h; ++y) for (int x = 0; x < w; ++x) {
        char *p = b.data() + (y*w+x)*components;
        p[0] = char((x*31+y*7)%256); p[1] = char((x*11+y*29)%256);
        p[2] = char((x*23+y*17)%256);
        if (components == 4) p[3] = char((x*37+y*53)%256);
    }
    return b;
}
// Independently authored fixtures: no candidate-codec encoder in this path.
QByteArray planar(int w, int h, int channels, QByteArray &expected, int surface = 14, int flags = 0) {
    const int components = channels == 3 ? 3 : 4;
    expected = pixels(w,h,components);
    QByteArray b = header(w,h,channels,flags,surface);
    for (quint64 id : {3,4,5}) descriptor(b,8,id);
    if (channels >= 4) descriptor(b,1,2);
    if (channels == 5) descriptor(b,8,6);
    const qsizetype table = b.size(); b.append(QByteArray(h*4,'\0'));
    for (int y = 0; y < h; ++y) {
        word(b,table+y*4,quint32(b.size()-16));
        for (int c = 0; c < 3; ++c) for (int x = 0; x < w; ++x)
            b.append(expected[(y*w+x)*components+c]);
        if (channels >= 4) {
            for (int x = 0; x < w; x += 8) {
                quint8 mask = 0;
                for (int k = 0; k < 8 && x+k < w; ++k)
                    if ((x+k+y)%2) mask |= quint8(1 << (7-k));
                b.append(char(mask));
            }
            if (channels == 4) for (int x = 0; x < w; ++x)
                expected[(y*w+x)*4+3] = (x+y)%2 ? char(255) : char(0);
        }
        if (channels == 5) for (int x = 0; x < w; ++x)
            b.append(expected[(y*w+x)*4+3]);
    }
    return b;
}
QByteArray compressed(const QByteArray &b) {
    const QByteArray z = qCompress(b.mid(16));
    QByteArray out("SIMISA@F",8); out.append(QByteArray(8,'@'));
    word(out,8,quint32(b.size()-16)); out.append(z.mid(4)); return out;
}
AceReadOptions limits() {
    AceReadOptions o; o.maxBytes = InputLimit; o.maxDimension = 8192;
    o.maxPixels = 16LL*1024*1024; o.legacyDxt1Colors = true; return o;
}
bool candidate(const QByteArray &bytes, AceDocument &doc, QByteArray &decoded,
               int &components, QString &error) {
    return AceDocument::parse(bytes,doc,error,limits())
        && doc.decode(0,decoded,components,error);
}
bool inputPreflight(const QByteArray &bytes, AceInputValidation::Profile profile, QString &error) {
    QTemporaryFile input;
    if(!input.open() || input.write(bytes)!=bytes.size() || !input.flush()) {
        error="Unable to stage preflight fixture"; return false;
    }
    const QString path=input.fileName(); input.close();
    return AceInputValidation::preflight(path,profile,error);
}
// Preflight the exact pointer spans used by the real legacy decoder. Never
// expose malformed/truncated/new-format input to its unchecked pointer loops.
bool legacySafe(const QByteArray &bytes, const AceDocument &doc, QString &reason) {
    QTemporaryFile input;
    if (!input.open() || input.write(bytes) != bytes.size() || !input.seek(0)) {
        reason = "Temporary preflight input failed"; return false;
    }
    std::unique_ptr<FileBuffer> expanded(ReadFile::read(&input));
    if (!expanded) { reason = "Legacy envelope rejected"; return false; }
    AceFormatLayout layout;
    if (!AceFormatValidator::inspect(expanded->data,expanded->length,layout,&reason)) return false;
    if (doc.metadata.channels.size() < 3 || doc.metadata.channels.size() > 5) {
        reason = "Legacy channel count unsupported"; return false;
    }
    const auto &ch = doc.metadata.channels;
    for (int i = 0; i < 3; ++i) if (ch[i].id != quint64(i+3) || ch[i].bits != 8) {
        reason = "Legacy requires ordered RGB8 descriptors"; return false;
    }
    if (ch.size() >= 4 && (ch[3].id != 2 || ch[3].bits != 1)) {
        reason = "Legacy requires mask descriptor"; return false;
    }
    if (ch.size() == 5 && (ch[4].id != 6 || ch[4].bits != 8)) {
        reason = "Legacy requires alpha descriptor"; return false;
    }
    // Surface labels 14/16/17 do not imply packed storage without flag 16.
    // The legacy reader handles these ordered planar channels identically.
    // Flag 4 changes no row/table arithmetic (legacy explicitly handles 4/5).
    const bool planarProfile = (doc.surface() == 14 || doc.surface() == 16 || doc.surface() == 17)
        && !(doc.options() & ~quint32(5));
    const bool dxt1Profile = doc.surface() == 18 && (doc.options() & 16)
        && !(doc.options() & ~quint32(21));
    if ((!planarProfile && !dxt1Profile)
        || layout.width%2 || layout.height%2 || !doc.metadata.palettes.isEmpty()) {
        reason = QString("Outside safely comparable legacy profile (surface=%1 flags=%2)")
            .arg(doc.surface()).arg(doc.options()); return false;
    }
    return true;
}
bool legacy(const QByteArray &bytes, const AceDocument &doc, QByteArray &decoded,
            int &w, int &h, int &components, QString &error) {
    if (!legacySafe(bytes,doc,error)) return false;
    QTemporaryFile input;
    if (!input.open() || input.write(bytes) != bytes.size() || !input.flush()) {
        error = "Temporary decoder input failed"; return false;
    }
    // Close before QFile opens the same input on Windows; retain auto-removal.
    const QString path = input.fileName(); input.close();
    Texture texture;
    texture.pathid = path;
    runLegacyAceReader(&texture);
    if (!texture.loaded || texture.error || texture.missing || !texture.imageData) {
        error = "Legacy loader failed after preflight"; return false;
    }
    w=texture.width; h=texture.height; components=texture.bytesPerPixel;
    decoded = QByteArray(reinterpret_cast<const char *>(texture.imageData),texture.imageSize);
    return true;
}
qsizetype differences(const QByteArray &a, const QByteArray &b) {
    if (a.size() != b.size()) return -1;
    qsizetype n=0; for(qsizetype i=0;i<a.size();++i) n += a[i]!=b[i]; return n;
}
bool runProductionBytes(const QByteArray &bytes, Texture &texture, bool threaded = false) {
    QTemporaryFile input;
    if(!input.open() || input.write(bytes)!=bytes.size() || !input.flush()) return false;
    texture.pathid=input.fileName(); input.close();
    AceLib worker; worker.texture=&texture;
    if(threaded) { worker.start(); worker.wait(); } else worker.run();
    return true;
}
bool productionMatches(const QByteArray &bytes, const QByteArray &expected,
                       int width, int height, int sourceWidth, int sourceHeight,
                       int components, const QString &label) {
    Texture texture;
    texture.ref=7; texture.hashid.push_back("preserved-alias");
    if(!runProductionBytes(bytes,texture) || !texture.loaded || !texture.editable
        || texture.glLoaded || texture.error || texture.missing || !texture.imageData
        || texture.width!=width || texture.height!=height
        || texture.sourceWidth!=sourceWidth || texture.sourceHeight!=sourceHeight
        || texture.bytesPerPixel!=components || texture.bpp!=components*8
        || texture.imageSize!=expected.size() || texture.saveDds24
        || texture.type!=(components==4?GL_RGBA:GL_RGB)
        || texture.ref!=7 || texture.hashid.size()!=1 || texture.hashid[0]!="preserved-alias") {
        std::cerr << label.toStdString() << ": PRODUCTION ADAPTER STATE MISMATCH\n"; return false;
    }
    const QByteArray actual(reinterpret_cast<const char *>(texture.imageData),texture.imageSize);
    if(actual!=expected) {
        std::cerr << label.toStdString() << ": PRODUCTION PIXEL MISMATCH bytes="
            << differences(actual,expected) << '\n'; return false;
    }
    return true;
}
bool parity(const QByteArray &bytes, const QString &label) {
    AceDocument doc; QByteArray current, old; QString error; int components=0,w=0,h=0,c=0;
    if (!candidate(bytes,doc,current,components,error)) {
        std::cerr << label.toStdString() << ": CANDIDATE REJECTED: " << error.toStdString() << '\n'; return false;
    }
    if(!inputPreflight(bytes,AceInputValidation::Profile::RoutePaint,error)
        || !inputPreflight(bytes,AceInputValidation::Profile::TerrainConversion,error)) {
        std::cerr << label.toStdString() << ": INPUT PREFLIGHT REJECTED: " << error.toStdString() << '\n';
        return false;
    }
    if (!legacy(bytes,doc,old,w,h,c,error)) {
        std::cerr << label.toStdString() << ": LEGACY COMPARISON UNAVAILABLE: " << error.toStdString() << '\n'; return false;
    }
    const qsizetype n = differences(current,old);
    if (w!=doc.levels[0].width || h!=doc.levels[0].height || c!=components || n!=0) {
        int maximum=0; qsizetype first=-1;
        if(n>0) for(qsizetype i=0;i<current.size();++i) {
            maximum=std::max(maximum,std::abs(int(quint8(current[i]))-int(quint8(old[i]))));
            if(first<0 && current[i]!=old[i]) first=i;
        }
        std::cerr << label.toStdString() << ": PIXEL MISMATCH bytes=" << n
                  << " first=" << first << " maxDelta=" << maximum << '\n'; return false;
    }
    return productionMatches(bytes,old,w,h,doc.levels[0].width,doc.levels[0].height,c,label);
}
QByteArray dxtFixture(quint16 endpoint, int channels = 3) {
    // Full 4x4/2x2/1x1 mip chain: matches the legacy offset-table assumption.
    QByteArray b=header(4,4,channels,17,18);
    for(quint64 id : {3,4,5}) descriptor(b,8,id);
    if(channels==4) descriptor(b,1,2);
    const qsizetype table=b.size(); b.append(QByteArray(12,'\0'));
    word(b,table,quint32(b.size()-16));
    const qsizetype block=b.size(); b.append(QByteArray(12,'\0'));
    word(b,block,8); qToLittleEndian(endpoint,b.data()+block+4);
    // All selectors choose endpoint zero. Planar mip tails have independent RGB.
    word(b,table+4,quint32(b.size()-16)); b.append(QByteArray(channels==4?14:12,char(0)));
    word(b,table+8,quint32(b.size()-16)); b.append(QByteArray(channels==4?4:3,char(0)));
    return b;
}
void legacyColorTests() {
    // Every 5/6-bit channel value, all selectors, endpoint orders and mask mode.
    for(int channel=0;channel<3;++channel) {
        const int count=channel==1?64:32, shift=channel==0?11:channel==1?5:0;
        for(int value=0;value<count;++value) for(bool reverse : {false,true})
            for(int channels : {3,4}) {
                QByteArray bytes=dxtFixture(quint16(value<<shift),channels);
                const qsizetype block=168+channels*16+12+4;
                if(reverse) {
                    qToLittleEndian<quint16>(0,bytes.data()+block);
                    qToLittleEndian<quint16>(quint16(value<<shift),bytes.data()+block+2);
                }
                word(bytes,block+4,0xe4e4e4e4);
                check(parity(bytes,QString("DXT1 channel=%1 value=%2 reverse=%3 channels=%4")
                    .arg(channel).arg(value).arg(reverse).arg(channels)),"all DXT1 endpoint/selector parity");
            }
    }
    const QByteArray bytes=dxtFixture(0x2000);
    AceDocument compatible,upstream; QByteArray compatPixels,scaled; QString error;
    int c=0;
    check(candidate(bytes,compatible,compatPixels,c,error),"compatibility document decode");
    AceReadOptions read; // Exercise the actual unchanged upstream default.
    check(AceDocument::parse(bytes,upstream,error,read) && upstream.decode(0,scaled,c,error),
        "upstream scaling opt-out decode");
    check(compatPixels.size()==48 && scaled.size()==48 && quint8(compatPixels[0])==33 && quint8(scaled[0])==32,
        "compatibility is opt-in and leaves upstream defaults intact");
    QByteArray thumbnail;
    check(compatible.decodeThumbnail(0,4,4,thumbnail,error) && thumbnail.size()==64
        && quint8(thumbnail[2])==33,"thumbnail inherits document's decode policy");
    QByteArray decodedInto(48,'\0');
    check(compatible.decodeInto(0,reinterpret_cast<unsigned char *>(decodedInto.data()),48,c,error)
        && decodedInto==compatPixels,"decodeInto inherits document's policy");
    // Reading one document with another policy must replace the previous policy.
    check(AceDocument::parse(bytes,compatible,error,read) && compatible.decode(0,scaled,c,error)
        && scaled.size()==48 && quint8(scaled[0])==32,"reparse replaces compatibility policy");
    QByteArray bc2(16,'\0'),normal,legacyColors;
    for(int i=0;i<8;++i) bc2[i]=char(0xff); // Full opacity.
    qToLittleEndian<quint16>(0x2000,bc2.data()+8);
    check(DxtCodec::decode(bc2,4,4,DxtCodec::Format::Dxt3,true,normal,error)
        && DxtCodec::decode(bc2,4,4,DxtCodec::Format::Dxt3,true,legacyColors,error,
            DxtCodec::ColorExpansion::LegacyBitReplication)
        && normal==legacyColors && normal.size()==64 && quint8(normal[0])==32,
        "BC1 compatibility does not change DXT3 decoding");
}
void blockTests() {
    QString error; QByteArray decoded;
    // DXT1 transparent selector and all four colour selectors, independently
    // specified endpoints/expected pixels (no encode/decode roundtrip oracle).
    QByteArray bc1(8,'\0');
    qToLittleEndian<quint16>(0xffff,bc1.data()+2);
    word(bc1,4,0xe4e4e4e4);
    check(DxtCodec::decode(bc1,4,4,DxtCodec::Format::Dxt1,true,decoded,error),"DXT1 mask block decode");
    QByteArray expected;
    for(int i=0;i<16;++i) {
        const int value=i%4==1?255:i%4==2?127:0;
        expected.append(char(value)); expected.append(char(value)); expected.append(char(value));
        expected.append(i%4==3?char(0):char(255));
    }
    check(decoded==expected,"DXT1 mask and colour selector reference");
    for(const auto format : {DxtCodec::Format::Dxt2,DxtCodec::Format::Dxt3,
                            DxtCodec::Format::Dxt4,DxtCodec::Format::Dxt5}) {
        QByteArray block(16,'\0');
        qToLittleEndian<quint16>(0xffff,block.data()+8);
        const bool explicitAlpha=format==DxtCodec::Format::Dxt2 || format==DxtCodec::Format::Dxt3;
        if(explicitAlpha) for(int i=0;i<8;++i) block[i]=char((2*i)|((2*i+1)<<4));
        else {
            block[0]=char(255); block[1]=char(0);
            quint64 indices=0; for(int i=0;i<16;++i) indices|=quint64(i%8)<<(3*i);
            for(int i=0;i<6;++i) block[2+i]=char(indices>>(8*i));
        }
        check(DxtCodec::decode(block,4,4,format,true,decoded,error),"DXT2-5 alpha block decode");
        expected.clear();
        const int interpolated[8]={255,0,218,182,145,109,72,36};
        const bool premultiplied=format==DxtCodec::Format::Dxt2 || format==DxtCodec::Format::Dxt4;
        for(int i=0;i<16;++i) {
            const int a=explicitAlpha?i*17:interpolated[i%8];
            const char color=premultiplied && a==0?char(0):char(255);
            expected.append(color); expected.append(color); expected.append(color); expected.append(char(a));
        }
        check(decoded==expected,"DXT2-5 independent alpha and premultiplication reference");
        QByteArray ace=header(4,4,5,16,18+int(format));
        for(quint64 id : {3,4,5}) descriptor(ace,8,id);
        descriptor(ace,1,2); descriptor(ace,8,6);
        const qsizetype table=ace.size(); ace.append(QByteArray(8,'\0'));
        word(ace,table,quint32(ace.size()-20)); // Offset of raw byte-count DWORD, relative to body.
        word(ace,table+4,quint32(block.size())); ace.append(block);
        AceDocument doc; QByteArray viaContainer; int components=0;
        check(candidate(ace,doc,viaContainer,components,error)
            && components==4 && viaContainer==expected,"DXT2-5 ACE envelope and CPU pixel reference");
        check(productionMatches(ace,expected,4,4,4,4,4,"DXT2-5 production"),
            "production loads expanded DXT2-5 CPU pixels for unchanged renderer");
    }
    // Clipped edge blocks must not write outside the supplied output span.
    QByteArray block(8,'\0'); qToLittleEndian<quint16>(0xf800,block.data());
    QByteArray guarded(3*2*3+2,char(0x5a));
    check(DxtCodec::decodeInto(block,3,2,DxtCodec::Format::Dxt1,false,
        reinterpret_cast<unsigned char *>(guarded.data()+1),18,error),"DXT clipped block decodeInto");
    check(guarded.front()==char(0x5a) && guarded.back()==char(0x5a),"DXT output guard bytes");
    for(int i=0;i<6;++i) check(quint8(guarded[1+i*3])==255 && guarded[2+i*3]==0
        && guarded[3+i*3]==0,"DXT clipped pixels");
}
void productionLoadingTests() {
    const bool savedWrite=Game::writeEnabled, savedUnsafe=Game::UnsafeMode;
    const int savedQuality=Game::textureQuality;
    Game::writeEnabled=true; Game::UnsafeMode=false; Game::textureQuality=4;
    QByteArray expected; const QByteArray bytes=planar(8,8,5,expected,17);
    check(productionMatches(bytes,expected,8,8,8,8,4,"editing native resolution"),
        "production editing ignores reduced rendering quality");
    check(productionMatches(compressed(bytes),expected,8,8,8,8,4,"compressed editing"),
        "production compressed ACE retains full editable pixels");
    {
        Texture texture;
        check(runProductionBytes(compressed(bytes),texture,true) && texture.loaded && texture.editable
            && texture.imageData && texture.imageSize==expected.size()
            && QByteArray(reinterpret_cast<const char *>(texture.imageData),texture.imageSize)==expected,
            "existing QThread dispatch produces the same CPU pixels without a GL context");
    }
    Game::writeEnabled=false; Game::textureQuality=3;
    QByteArray reduced;
    for(int y : {0,4}) for(int x : {0,4}) reduced.append(expected.mid((y*8+x)*4,4));
    check(productionMatches(bytes,reduced,2,2,8,8,4,"view-only quality"),
        "production view-only downsampling preserves legacy pixel selection");
    Game::textureQuality=32;
    check(productionMatches(bytes,expected,8,8,8,8,4,"oversized quality divisor"),
        "quality never produces empty textures");
    Game::writeEnabled=true; Game::textureQuality=2;
    {
        Texture texture;
        texture.loaded=true; texture.editable=true; texture.glLoaded=true;
        texture.error=true; texture.missing=true; texture.saveDds24=true;
        texture.imageData=new unsigned char[4]{1,2,3,4}; texture.imageSize=4;
        texture.width=77; texture.height=55;
        auto *prior=texture.imageData;
        check(runProductionBytes(bytes.left(bytes.size()-1),texture),"stage malformed production input");
        check(!texture.loaded && !texture.editable && texture.error && !texture.missing
            && texture.imageData==prior && texture.imageData[3]==4
            && texture.width==77 && texture.height==55 && texture.imageSize==4,
            "production failure leaves pixel storage/dimensions unpublished");
        check(runProductionBytes(bytes,texture) && texture.loaded && texture.editable
            && !texture.error && !texture.missing && !texture.glLoaded && !texture.saveDds24,
            "production retry clears stale failure state and replaces pixels");
        if(texture.loaded) check(QByteArray(reinterpret_cast<const char *>(texture.imageData),texture.imageSize)==expected,
            "retry publishes exact source pixels");
    }
    {
        QImage image(reinterpret_cast<const uchar *>(expected.constData()),8,8,32,QImage::Format_RGBA8888);
        QByteArray mips; QString error; Texture texture;
        check(TerrtexMipmaps::serializeAce(image,true,mips,&error),"prepare production mip contract");
        check(runProductionBytes(mips,texture) && texture.loaded && texture.saveMipmaps
            && !texture.saveDds24 && texture.typk==1 && texture.compressed==14,
            "production preserves existing mip/save and alpha metadata");
    }
    {
        QByteArray oddExpected; const QByteArray odd=planar(3,5,5,oddExpected);
        Texture rejected;
        check(runProductionBytes(odd,rejected) && rejected.error && !rejected.loaded,
            "production keeps existing safe-mode odd-dimension rejection");
        Game::UnsafeMode=true;
        check(productionMatches(odd,oddExpected,3,5,3,5,4,"unsafe-mode odd input"),
            "bounded odd decode honors explicit legacy Unsafe Mode");
        Game::UnsafeMode=false;
        QByteArray tinyExpected; const QByteArray tiny=planar(1,1,3,tinyExpected);
        check(runProductionBytes(tiny,rejected) && rejected.error && !rejected.loaded,
            "production keeps existing minimum editor texture dimensions");
    }
    {
        QTemporaryDir directory; Texture missing;
        check(directory.isValid(),"prepare missing-file fixture");
        missing.pathid=directory.path()+"/absent.ace";
        AceLib worker; worker.texture=&missing; worker.run();
        check(missing.missing && !missing.error && !missing.loaded && !missing.editable,
            "production distinguishes missing files from malformed files");
        AceLib empty; empty.run();
        check(empty.texture==nullptr,"empty production worker safely returns");
    }
    Game::writeEnabled=savedWrite; Game::UnsafeMode=savedUnsafe; Game::textureQuality=savedQuality;
}
QByteArray rawDxtBase(int width, int height) {
    // One raw BC1 level, no mip chain: valid ACE rejected by the old fixed
    // offset heuristic. No candidate writer is used to assemble this fixture.
    QByteArray bytes=header(width,height,3,16,18);
    for(quint64 id : {3,4,5}) descriptor(bytes,8,id);
    const qsizetype table=bytes.size(); bytes.append(QByteArray(8,'\0'));
    const quint32 payload=quint32(((qint64(width)+3)/4)*((qint64(height)+3)/4)*8);
    word(bytes,table,quint32(table+4-16)); word(bytes,table+4,payload);
    bytes.append(QByteArray(payload,'\0'));
    return bytes;
}
void inputPreflightTests() {
    using Profile=AceInputValidation::Profile;
    QByteArray expected; const QByteArray good=planar(8,8,5,expected,17,4);
    const QByteArray baseOnly=rawDxtBase(4,4);
    AceFormatLayout oldLayout; QString error;
    check(!AceFormatValidator::inspect(reinterpret_cast<const unsigned char *>(baseOnly.constData()),
        baseOnly.size(),oldLayout,&error),"fixture exposes obsolete legacy mip-offset precheck");
    for(const auto profile : {Profile::RoutePaint,Profile::TerrainConversion}) {
        check(inputPreflight(good,profile,error),"parser preflight accepts planar alpha/flag 4");
        check(inputPreflight(compressed(good),profile,error),"parser preflight accepts compressed ACE");
        check(inputPreflight(baseOnly,profile,error),"parser preflight accepts valid base-only DXT1");
        QByteArray offset=good; word(offset,248,0xffffffff);
        check(!inputPreflight(offset,profile,error),"parser preflight rejects invalid row offset");
        QByteArray channel=good; word(channel,168,0);
        check(!inputPreflight(channel,profile,error),"parser preflight rejects invalid channel descriptor");
        QByteArray zipped=compressed(good); zipped.chop(4);
        check(!inputPreflight(zipped,profile,error),"parser preflight rejects truncated deflate stream");
        QByteArray dimension=good; word(dimension,24,8193);
        check(!inputPreflight(dimension,profile,error) && error.contains("dimensions/counts"),
            "both profiles preserve 8192 dimension cap before payload work");
        QByteArray tinyExpected;
        check(!inputPreflight(planar(1,1,3,tinyExpected),profile,error),
            "both preflight profiles retain existing minimum editor dimensions");
        const QByteArray unrelated=QByteArray::fromHex("0000058202789ced9cdf6ee43414c66d5cc8a68c")+QByteArray(64,'\0');
        check(!inputPreflight(unrelated,profile,error) && error.contains("envelope"),
            "parser preflight rejects non-ACE input without guessing envelopes");
    }
    {
        // These are small encoded BC1 fixtures: preflight never allocates the
        // editable ~64 MiB image. The RGBA-equivalent batch budget applies to
        // opaque RGB too, exactly as it did before the parser replacement.
        const QByteArray boundary=rawDxtBase(4096,4096);
        check(inputPreflight(boundary,Profile::RoutePaint,error),
            "exact 64 MiB RGBA-equivalent batch boundary accepted");
        const QByteArray above=rawDxtBase(4096,4098);
        check(!inputPreflight(above,Profile::RoutePaint,error) && error.contains("dimensions/counts"),
            "batch rejects RGB beyond conservative 64 MiB budget before pixel allocation");
        check(inputPreflight(above,Profile::TerrainConversion,error),
            "conversion does not inherit the smaller batch-only image budget");
    }
    QByteArray inflated=compressed(good); word(inflated,8,128u*1024u*1024u+1u);
    check(!inputPreflight(inflated,Profile::RoutePaint,error) && error.contains("Inflated ACE size"),
        "batch rejects oversized advertised inflation before allocating its payload");
    {
        QTemporaryDir directory; check(directory.isValid(),"missing preflight fixture directory");
        check(!AceInputValidation::preflight(directory.path()+"/absent.ace",Profile::RoutePaint,error),
            "preflight read failure is propagated");
    }
}
void selfTest() {
    blockTests();
    legacyColorTests();
    productionLoadingTests();
    for(int channels : {3,4,5}) for(int flags : {0,4}) {
        QByteArray expected,result; AceDocument doc; QString error; int c=0;
        const int surface=channels==3?14:channels==4?16:17;
        const QByteArray bytes=planar(8,8,channels,expected,surface,flags);
        check(candidate(bytes,doc,result,c,error) && result==expected,
            "planar surface label and flag-4 independent pixels");
        check(parity(bytes,"planar alpha/mask surface and flag 4"),
            "previously excluded planar profiles are safely comparable");
    }
    for (int channels : {3,4,5}) for (const QSize size : {QSize(2,2),QSize(8,8),QSize(16,8)}) {
        QByteArray expected; const QByteArray plain=planar(size.width(),size.height(),channels,expected);
        for(bool zip : {false,true}) {
            const QByteArray bytes=zip ? compressed(plain) : plain;
            AceDocument doc; QByteArray result; int components=0; QString error;
            const QString label=QString("planar %1 %2x%3 zipped=%4").arg(channels).arg(size.width()).arg(size.height()).arg(zip);
            check(candidate(bytes,doc,result,components,error),label+" decode "+error);
            check(result==expected,label+" independent expected pixels");
            check(parity(bytes,label),label+" exact legacy parity");
            if(channels==5) {
                QByteArray mask; check(doc.decode(0,result,components,error,&mask),label+" independent mask decode");
                check(mask.size()==size.width()*size.height(),label+" mask length");
                if(mask.size()==size.width()*size.height()) for(int y=0;y<size.height();++y)
                    for(int x=0;x<size.width();++x)
                        check(mask[y*size.width()+x]==char((x+y)%2),label+" alpha does not replace independent mask");
            }
        }
    }
    // Actual GenX serializers, not substitute encoders. Only memory output used.
    for(bool alpha : {false,true}) {
        const int c=alpha?4:3; const QByteArray expected=pixels(8,8,c);
        Texture texture; texture.width=8; texture.height=8; texture.bytesPerPixel=c;
        texture.imageData=new unsigned char[expected.size()];
        std::copy(expected.cbegin(),expected.cend(),texture.imageData);
        for(bool mip : {false,true}) {
            if(alpha && !mip) continue; // Existing no-mip writer emits RGB only.
            texture.saveMipmaps=mip;
            QByteArray bytes,result; QString error; AceDocument doc; int components=0;
            check(AceLib::serialize(&texture,bytes,&error),"production serialize "+error);
            check(candidate(bytes,doc,result,components,error),"production output decode "+error);
            check(result==expected,"production output base pixels");
            check(parity(bytes,"production writer"),"production output legacy parity");
            if(!mip) {
                check(!doc.warnings.isEmpty(),"exact historical RGB offset recovery diagnosed");
                AceReadOptions strict=limits(); strict.allowLegacyRgbOffsets=false;
                AceDocument rejected; check(!AceDocument::parse(bytes,rejected,error,strict),"historical stride rejected when recovery disabled");
            } else {
                QImage image(reinterpret_cast<const uchar *>(expected.constData()),8,8,8*c,
                    alpha?QImage::Format_RGBA8888:QImage::Format_RGB888);
                QVector<QImage> levels;
                check(TerrtexMipmaps::generate(image,levels,&error),"expected GenX mip generation");
                check(doc.levels.size()==levels.size(),"authored mip count");
                for(int m=0;m<std::min(doc.levels.size(),levels.size());++m) {
                    check(doc.decode(m,result,components,error),"authored mip decode");
                    const QImage reference=levels[m].convertToFormat(alpha?QImage::Format_RGBA8888:QImage::Format_RGB888);
                    QByteArray tight;
                    for(int y=0;y<reference.height();++y)
                        tight.append(reinterpret_cast<const char *>(reference.constScanLine(y)),reference.width()*c);
                    check(result==tight,"every authored mip matches existing generator");
                }
                word(bytes,32,alpha?17:14); word(bytes,20,5);
                check(parity(bytes,"planar mip flag 5 and alpha surface"),
                    "previously excluded flag-5 mip profiles compare exactly");
            }
        }
    }
    check(parity(dxtFixture(0xf800),"DXT1 red endpoint"),"DXT1 exact representable endpoint parity");
    {
        QByteArray flagged=dxtFixture(0x2000); word(flagged,20,21);
        check(parity(flagged,"DXT1 flag 21"),"flag-4 DXT1 mip variant safely compares");
    }
    {
        const QByteArray bytes=dxtFixture(0x0800); // 5-bit red 1: scaled=8, replicated=8.
        check(parity(bytes,"DXT1 low endpoint"),"DXT1 low endpoint parity");
        const QByteArray divergent=dxtFixture(0x2000); // Compatibility must now match legacy red 33.
        AceDocument doc; QByteArray next,old; QString error; int c=0,w=0,h=0,lc=0;
        const bool loaded=candidate(divergent,doc,next,c,error) && legacy(divergent,doc,old,w,h,lc,error);
        check(loaded,"DXT1 expansion diagnostic decode");
        check(loaded && next==old && quint8(next[0])==33,
              "DXT1 compatibility resolves endpoint rounding without tolerance");
    }
    // Bounded parser failures must not replace a previously valid document.
    QByteArray expected; const QByteArray good=planar(8,8,3,expected);
    {
        // A surface label alone must never admit packed raw pixels to the
        // legacy reader's planar pointer loops, even if its span would fit.
        QByteArray raw=header(4,4,5,16,17);
        for(quint64 id : {3,4,5}) descriptor(raw,8,id);
        descriptor(raw,1,2); descriptor(raw,8,6);
        const qsizetype table=raw.size(); raw.append(QByteArray(8,'\0'));
        word(raw,table,quint32(table+4-16)); word(raw,table+4,32);
        raw.append(QByteArray(32,char(0xff)));
        raw.append(QByteArray(64,'\0')); // Enough trailing bytes that bounds alone cannot reject it.
        AceDocument doc; QByteArray result; QString error; int c=0;
        check(candidate(raw,doc,result,c,error),"packed raw candidate fixture is valid");
        check(productionMatches(raw,QByteArray(64,char(0xff)),4,4,4,4,4,"packed RGBA production"),
            "production expands packed RGBA without changing the renderer");
        check(!legacySafe(raw,doc,error) && error.contains("profile"),
            "packed raw input remains outside legacy comparison despite sufficient span");
        QByteArray unknown=good; word(unknown,20,2);
        check(candidate(unknown,doc,result,c,error),"structurally valid unknown flag fixture");
        check(!legacySafe(unknown,doc,error) && error.contains("profile"),
            "unknown flags still excluded from legacy comparison");
        const QByteArray unrelated=QByteArray::fromHex("0000058202789ced9cdf6ee43414c66d5cc8a68c")+QByteArray(64,'\0');
        check(!AceDocument::parse(unrelated,doc,error,limits()) && error.contains("envelope"),
            "mislabeled non-ACE data stays rejected without envelope guessing");
    }
    {
        QByteArray oddExpected; const QByteArray odd=planar(3,5,5,oddExpected);
        AceDocument doc; QByteArray result; QString error; int c=0;
        check(candidate(odd,doc,result,c,error) && result==oddExpected,
            "odd rectangular alpha/mask input candidate-only; unsafe for legacy");
        check(!legacySafe(odd,doc,error),"legacy odd-dimension input is never executed");
    }
    QVector<QByteArray> bad{good.left(12),good.left(167),good.left(good.size()-1)};
    QByteArray huge=good; word(huge,24,0xffffffff); bad.push_back(huge);
    QByteArray offset=good; word(offset,216,0xffffffff); bad.push_back(offset);
    QByteArray truncatedZ=compressed(good); truncatedZ.chop(4); bad.push_back(truncatedZ);
    QByteArray inflate=compressed(good); word(inflate,8,quint32(InputLimit+1)); bad.push_back(inflate);
    for(const auto &bytes : bad) {
        AceDocument doc; QByteArray result; QString error; int c=0;
        check(candidate(good,doc,result,c,error),"seed failure atomicity");
        check(!AceDocument::parse(bytes,doc,error,limits()),"malformed input rejected");
        check(doc.levels.size()==1 && doc.levels[0].width==8,"failed parse preserves document");
    }
    QByteArray blocks(8,'\0'),unchanged("sentinel"); QString error;
    check(!DxtCodec::decode(blocks.left(7),4,4,DxtCodec::Format::Dxt1,false,unchanged,error)
        && unchanged=="sentinel","truncated DXT decode preserves output");
}
int corpus() {
    const QString root=qEnvironmentVariable("TSRE_ACE_PARITY_CORPUS");
    if(root.isEmpty()) { std::cout << "SKIP: set TSRE_ACE_PARITY_CORPUS to a local representative ACE directory.\n"; return 77; }
    if(!QFileInfo(root).isDir()) { std::cerr << "Corpus directory unavailable\n"; return 1; }
    QDirIterator it(root,QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);
    int count=0,passed=0;
    while(it.hasNext()) {
        const QString path=it.next(); if(QFileInfo(path).suffix().compare("ace",Qt::CaseInsensitive)!=0) continue;
        if(++count>256) { std::cerr << "Corpus exceeds focused 256-file limit; select a smaller corpus\n"; return 1; }
        QFile file(path);
        if(!file.open(QIODevice::ReadOnly) || file.size()>InputLimit) { check(false,"corpus open/size"); continue; }
        const QByteArray bytes=file.read(InputLimit+1);
        if(file.error()!=QFileDevice::NoError || bytes.size()!=file.size()) { check(false,"corpus short read"); continue; }
        // Paths/hashes are local evidence only; never write into the corpus.
        const QString label=QDir(root).relativeFilePath(path);
        const bool ok=parity(bytes,label); check(ok,label); passed+=ok;
        std::cout << (ok?"EXACT ":"GAP ") << label.toStdString() << " sha256="
            << QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex().constData() << '\n';
    }
    check(count>0,"corpus contains ACE inputs");
    std::cout << "Corpus exact=" << passed << " total=" << count << "; every gap blocks parity acceptance.\n";
    return failures?1:0;
}
} // namespace
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    // Keep real loader at full source resolution, even in editing/quality tests.
    AceLib::IsThread=true;
    Game::writeEnabled=true; Game::textureQuality=2;
    if(app.arguments().contains("--corpus")) return corpus();
    const bool preflightOnly=app.arguments().contains("--preflight");
    if(preflightOnly) inputPreflightTests(); else selfTest();
    std::cout << "ACE " << (preflightOnly?"input preflight":"codec") << " checks=" << checks << " failures=" << failures
              << "; synthetic coverage only, corpus parity is a separate gate.\n";
    return failures?1:0;
}
