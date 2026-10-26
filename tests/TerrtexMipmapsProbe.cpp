// Uncompressed ACE mip-chain regression coverage. GNU GPL v3 or later.
#include "TerrtexMipmaps.h"
#include "AceFormatValidator.h"
#include "SaveProgress.h"
#include <QtEndian>
#include <QColor>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <iostream>
#include <cmath>

int main() {
    int failures = 0;
    auto check = [&](bool ok, const char *message) {
        if(!ok) { ++failures; std::cerr << message << '\n'; }
    };
    // A second file's rows advance its share of the total, not a fresh 0..100 bar.
    {
        SaveProgress::Range save(0,1);
        { SaveProgress::Range terrain(0.1,0.9);
          { SaveProgress::Range file(0.25,0.5);
            SaveProgress::set(0.5);
            check(std::abs(SaveProgress::fraction()-0.4)<1.e-9,
                "nested rows map into total route progress"); }
          SaveProgress::set(0.75);
          check(std::abs(SaveProgress::fraction()-0.7)<1.e-9,
              "parent range restored after file");
          { SaveProgress::Range failed(0.75,1);
            SaveProgress::set(0.25); }
          check(SaveProgress::fraction()<0.9,"failed work never reports completion"); }
        SaveProgress::set(1);
        check(SaveProgress::fraction()==1,"successful save can report full completion");
    }
    {
        QImage original(2048,2048,QImage::Format_RGB888);
        original.fill(QColor(45,90,135));
        QByteArray encoded;
        check(TerrtexMipmaps::serializeAce(original,false,encoded),"2048 ACE mip save");
        if(encoded.size()>=32) {
            check(qFromLittleEndian<quint32>(encoded.constData()+24)==2048
                && qFromLittleEndian<quint32>(encoded.constData()+28)==2048,
                "2048 base dimensions survive mip encoding");
        } else check(false,"2048 ACE payload exists");
        check(original.width()==2048 && original.height()==2048
            && original.pixelColor(2047,2047)==QColor(45,90,135),
            "2048 source remains untouched");
    }
    QImage base(8,8,QImage::Format_RGBA8888);
    for(int y = 0; y < 8; ++y) for(int x = 0; x < 8; ++x)
        base.setPixelColor(x,y,QColor(x*31,y*29,(x+y)*17, x%2 ? 255 : 0));
    for(bool alpha : {false,true}) {
        QVector<QImage> expectedLevels;
        check(TerrtexMipmaps::generate(base, expectedLevels), "generate expected stored mip pixels");
        QByteArray data;
        QString error;
        check(TerrtexMipmaps::serializeAce(base,alpha,data,&error), "serialize full mip chain");
        if(data.isEmpty()) continue;
        auto word = [&](int offset) { return qFromLittleEndian<quint32>(data.constData()+offset); };
        check(data.left(16) == "SIMISA@@@@@@@@@@", "uncompressed SIMISA wrapper");
        check(word(20) == 1 && word(24) == 8 && word(28) == 8, "mipmap flag and unchanged dimensions");
        check(word(32) == 14 && word(36) == (alpha ? 5u : 3u), "structured uncompressed channels");
        AceFormatLayout layout;
        check(AceFormatValidator::inspect(reinterpret_cast<const unsigned char*>(data.constData()),
            data.size(),layout,&error), "existing ACE base-level validator accepts mip chain");
        const int table = 168 + (alpha ? 5 : 3)*16;
        int tableRow = 0, expectedOffset = table + 15*4;
        for(const QImage &expected : expectedLevels) {
            const int size = expected.width();
            const int rowBytes = size*(alpha ? 4 : 3)+(alpha ? (size+7)/8 : 0);
            for(int y = 0; y < size; ++y) {
                const int rowStart = int(word(table+4*tableRow++))+16;
                check(rowStart == expectedOffset && rowStart+rowBytes <= data.size(), "every mip scanline offset and length");
                {
                    for(int x = 0; x < size; ++x) {
                        const QColor pixel = expected.pixelColor(x,y);
                        check(uchar(data[rowStart+x]) == pixel.red()
                            && uchar(data[rowStart+size+x]) == pixel.green()
                            && uchar(data[rowStart+size*2+x]) == pixel.blue(), "all stored mip RGB pixels preserved exactly");
                        if(alpha) {
                            check(uchar(data[rowStart+size*3+(size+7)/8+x]) == pixel.alpha(), "all stored mip alpha preserved exactly");
                            const uchar mask = uchar(data[rowStart+size*3+x/8]);
                            check(bool(mask & (1 << (7-x%8))) == (pixel.alpha() >= 128), "all mip mask bits preserved");
                        }
                    }
                }
                expectedOffset += rowBytes;
            }
        }
        check(expectedOffset == data.size(), "complete chain through 1x1 without trailing or missing bytes");
    }
    QImage checker(8,8,QImage::Format_RGBA8888);
    for(int y=0;y<8;++y) for(int x=0;x<8;++x)
        checker.setPixelColor(x,y, (x+y)%2 ? Qt::white : Qt::black);
    QVector<QImage> levels;
    check(TerrtexMipmaps::generate(checker,levels), "generate checker chain");
    if(!levels.isEmpty()) {
        const int gray = levels.back().pixelColor(0,0).red();
        check(gray >= 186 && gray <= 189, "linear-light averaging avoids dark sRGB mips");
        check(levels.first() == checker, "level zero never filtered");
    }
    for(int y=0;y<8;++y) for(int x=0;x<8;++x)
        checker.setPixelColor(x,y, x%2 ? QColor(255,255,255,255) : QColor(0,0,0,0));
    check(TerrtexMipmaps::generate(checker,levels), "generate alpha chain");
    if(!levels.isEmpty()) {
        const auto pixel = levels.back().pixelColor(0,0);
        check(pixel.red() >= 254 && pixel.alpha() >= 126 && pixel.alpha() <= 129,
            "premultiplied filtering avoids dark transparent fringes");
    }
    checker.fill(QColor(80,120,160,255));
    check(TerrtexMipmaps::generate(checker,levels), "generate constant chain");
    for(const QImage &level : levels) for(int y=0;y<level.height();++y) for(int x=0;x<level.width();++x)
        check(level.pixelColor(x,y)==QColor(80,120,160,255), "wrapped edges preserve constant color");
    QString error;
    check(!TerrtexMipmaps::generate(QImage(8,4,QImage::Format_RGB888),levels,&error)
        && !error.isEmpty() && levels.isEmpty(), "reject rectangular ACE mip chain without resizing");
    check(!TerrtexMipmaps::generate(QImage(6,6,QImage::Format_RGB888),levels), "reject non-power-of-two ACE mip chain");
    int reports = 0;
    { SaveProgress::Range work(0.2,0.8);
      double lastProgress = SaveProgress::fraction();
      SaveProgress::Scope reporting([&](const QString &){
          const double current = SaveProgress::fraction();
          check(current>=lastProgress && current>=0.2 && current<=0.8,
              "real mip rows advance monotonically within whole-save range");
          lastProgress = current; ++reports;
      });
      check(TerrtexMipmaps::generate(checker,levels), "progress-enabled mip generation"); }
    const int reported = reports;
    SaveProgress::report("headless save after scope");
    check(reported > 0 && reports == reported, "progress sink active during work and restored afterward");
    // Opt-in timing through the registered CTest; no route access or writes.
    // Default correctness runs stay small and do not time performance.
    if(qEnvironmentVariableIntValue("TSRE_BENCHMARK_TERRTEX_SAVE") != 0) {
        for(int size : {1024,2048}) {
            QImage sample(size,size,QImage::Format_RGBA8888);
            for(int y=0;y<size;++y) {
                uchar *row = sample.scanLine(y);
                for(int x=0;x<size;++x) {
                    row[x*4] = uchar((x*17+y*7)%256);
                    row[x*4+1] = uchar((x*3+y*13)%256);
                    row[x*4+2] = uchar((x*11+y*5)%256);
                    row[x*4+3] = 255;
                }
            }
            QElapsedTimer timer; timer.start();
            QVector<QImage> chain;
            check(TerrtexMipmaps::generate(sample,chain), "benchmark chain generation");
            const qint64 generationMs = timer.elapsed();
            timer.restart();
            QByteArray encoded;
            check(TerrtexMipmaps::serializeAce(sample,false,encoded), "benchmark complete serialization");
            std::cout << "TERRTEX benchmark " << size << "x" << size
                << " generate_only_ms=" << generationMs
                << " generate_and_encode_ms=" << timer.elapsed()
                << " bytes=" << encoded.size()
                << " sha256=" << QCryptographicHash::hash(encoded,QCryptographicHash::Sha256).toHex().constData()
                << '\n';
        }
    }
    return failures ? 1 : 0;
}
