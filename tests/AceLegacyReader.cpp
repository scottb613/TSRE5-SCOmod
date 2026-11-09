/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

// Frozen pre-integration reader, test-only; never linked into TSRE5.
// Source: AceLib.cpp Git blob 8338f4d0d2bb790d9201e7fadb3bd4f3c56a6a4f.
#include "AceLegacyReader.h"
#include "Texture.h"
#include "ReadFile.h"
#include "FileBuffer.h"
#include "Game.h"
#include <QFile>
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <limits>
#include <memory>

void runLegacyAceReader(Texture *texture) {
    constexpr bool IsThread = true;

    QFile file(texture->pathid);
    if (!file.open(QIODevice::ReadOnly)){
        texture->missing = true;
        if(!IsThread)
            if(Game::debugOutput) qDebug() << "ACE: not exist "<<texture->pathid;
        //return false;
        return;
    }
    //if(!IsThread)
    //    qDebug() << "ACE: "<<texture->pathid;
    std::unique_ptr<FileBuffer> data(ReadFile::read(&file));
    if (!data) {
        texture->missing = true;
        return;
    }
    //qDebug() << "Date:" << data->length;
    if(data->length < 37){
        texture->error = true;
        qWarning() << "ACE rejected:" << texture->pathid << "ACE header is truncated.";
        return;
    }
    unsigned char* bufor = data->data;
    int typ = 0;
    int dane = bufor[20];
    texture->saveMipmaps = (dane & 1) != 0;
    texture->saveDds24 = false;
    unsigned char tempt;
    texture->compressed = bufor[32];
    texture->width = bufor[25] * 256 + bufor[24];
    texture->height = bufor[29] * 256 + bufor[28];
    texture->sourceWidth = texture->width;
    texture->sourceHeight = texture->height;
    if (bufor[36] == 3) {
        texture->bpp = 24;
        texture->typk = typ = 0;
    }
    if (bufor[36] == 4) {
        texture->bpp = 32;
        texture->typk = typ = 2;
    }
    if (bufor[36] == 5) {
        texture->bpp = 32;
        texture->typk = typ = 1;
    }

    if(!IsThread)
        qDebug() << "--"<<texture->width<<":"<<texture->height<<" "<<texture->bpp;

    if ((texture->width <= 1) || (texture->height <= 1)
            || ((texture->bpp != 24) && (texture->bpp != 32))) {
        texture->error = true;
        return;
    }
    if (texture->width > 8192 || texture->height > 8192) {
        texture->error = true;
        return;
    }

    //// EFO MOD Check to see if texture is square....

    if ( texture->width % 2 != 0 || texture->height % 2 != 0 ) {
            // || (texture->width != texture->height) ) {  //  Now allowing rectangles that are squareable

        if(Game::UnsafeMode == true)
        {
            qWarning() << "Check Texture Dimensions (may cause crash): " << texture->pathid << " = " << texture->width << " x " << texture->height;
        }
        else   /// safe path
        {
            texture->error = true;

            if(!IsThread)
                qWarning() << "Non-squareable texture failed to load: " << texture->pathid
                    << " " << texture->width
                    << " " << texture->height
                    << " " << texture->bpp
                    ;
            return;
        }
    }


    texture->bytesPerPixel = (texture->bpp / 8);
    constexpr qint64 maximumDecodedBytes = 256ll * 1024ll * 1024ll;
    const qint64 decodedBytes = qint64(texture->bytesPerPixel)
        * qint64(texture->width) * qint64(texture->height);
    if(decodedBytes <= 0 || decodedBytes > maximumDecodedBytes
            || decodedBytes > std::numeric_limits<int>::max()) {
        texture->error = true;
        qWarning() << "ACE rejected:" << texture->pathid
                   << "decoded image size exceeds the supported limit.";
        return;
    }
    texture->imageSize = static_cast<int>(decodedBytes);
    texture->imageData = new unsigned char[texture->imageSize];
    if (texture->bpp == 24) {
        texture->type = GL_RGB;
    } else {
        texture->type = GL_RGBA;
    }

    int ptr = 0;
    if (texture->compressed != 18) {
        int iw = 0, ite;
        if (typ == 0) ptr = 216;
        if (typ == 1) ptr = 248;
        if (typ == 2) ptr = 232;
        if (dane == 0) ptr += texture->height * 4;
        else if (dane == 1) ptr += texture->height * 8 - 4;
        else if (dane == 4) ptr += texture->height * 4;
        else if (dane == 5) ptr += texture->height * 8 - 4;
        //if (dane == 5) {
        //    qDebug() << "ace dane: " << dane << texture->pathid;
        //}

        //if(!IsThread)
        //    qDebug() << "tekstura wtyp" << typ;

        if (typ == 0) {
            for (int ih = 0; ih<texture->height; ih++) {
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 1] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 2] = bufor[ptr++];
                }
            }
        }
        if (typ == 1) {
            for (int ih = 0; ih<texture->height; ih++) {
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 1] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 2] = bufor[ptr++];
                }
                ptr += (texture->width + 7) / 8;
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 3] = bufor[ptr++];
                }
            }
        }
        if (typ == 2) {
            for (int ih = 0; ih<texture->height; ih++) {
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 1] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; iw = iw + 1) {
                    texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 2] = bufor[ptr++];
                }
                for (iw = 0; iw<texture->width; ptr++) {
                    for (ite = 0; ite < 8; ite++) {
                        tempt = (unsigned char)(bufor[ptr] << ite);
                        texture->imageData[texture->bytesPerPixel * texture->width * ih + iw * texture->bytesPerPixel + 3] = (unsigned char)(tempt >> 7)*255;
                        iw = iw + 1;
                        if(iw == texture->width)
                            break;
                    }
                }
            }
        }
        //texture->loaded = true;
        //texture->editable = true;
    } else {
        //var start = new Date().getTime();
        ptr = 216;
        int tempp = texture->height;
        if (texture->bpp == 24) ptr += 4;
        else ptr += 20;
        while (tempp >= 1) {
            ptr += 4;
            tempp = tempp / 2;
        }
        unsigned short c[5] = {0,0,0,0,0};
        unsigned char r[4] = {0,0,0,0};
        unsigned char g[4] = {0,0,0,0};
        unsigned char b[4] = {0,0,0,0};
        unsigned char a[5] = {255, 255, 255, 255, 255};
        unsigned char bits[4];

        //for(var u = 0, ppp = ptr; u<100; u++, ptr = ppp)
        for (int ih = 0; ih<texture->height; ih += 4) {
            for (int iw = 0; iw<texture->width; iw += 4) {

                //May be undefined
                c[0] = bufor[ptr] + bufor[ptr+1]*256;
                ptr+=2;
                c[1] = bufor[ptr] + bufor[ptr+1]*256;
                ptr+=2;
                bits[0] = bufor[ptr++];
                bits[1] = bufor[ptr++];
                bits[2] = bufor[ptr++];
                bits[3] = bufor[ptr++];

                c[4] = c[0] & 0xf800;
                r[0] = ((c[4] >> 11) << 3) + (c[4] >> 13);
                c[4] = c[0] & 0x07e0;
                g[0] = ((c[4] >> 5) << 2) + (c[4] >> 9);
                c[4] = c[0] & 0x1f;
                b[0] = ((c[4] >> 0) << 3) + (c[4] >> 2);

                c[4] = c[1] & 0xf800;
                r[1] = ((c[4] >> 11) << 3) + (c[4] >> 13);
                c[4] = c[1] & 0x07e0;
                g[1] = ((c[4] >> 5) << 2) + (c[4] >> 9);
                c[4] = c[1] & 0x1f;
                b[1] = ((c[4] >> 0) << 3) + (c[4] >> 2);

                if (c[0] <= c[1]) {
                    r[2] = (r[0] + r[1]) / 2;
                    r[3] = 0;
                    g[2] = (g[0] + g[1]) / 2;
                    g[3] = 0;
                    b[2] = (b[0] + b[1]) / 2;
                    b[3] = 0;
                    a[3] = 0;
                } else {
                    r[2] = ((2 * r[0] + r[1]) / 3);
                    r[3] = ((r[0] + 2 * r[1]) / 3);
                    g[2] = ((2 * g[0] + g[1]) / 3);
                    g[3] = ((g[0] + 2 * g[1]) / 3);
                    b[2] = ((2 * b[0] + b[1]) / 3);
                    b[3] = ((b[0] + 2 * b[1]) / 3);
                    a[3] = 255;
                }

                for (int ii = 0; ii < 4; ii++) {
                    for (int jj = 0; jj < 4; jj++) {
                        int o = (bits[ii] >> (jj * 2)) & 0x3;
                        if(ih + ii >= texture->height || iw + jj >= texture->width)
                            continue;
                        int p = texture->bytesPerPixel * texture->width * (ih + ii) + (iw + jj) * texture->bytesPerPixel;
                        texture->imageData[p] = r[o];
                        texture->imageData[p + 1] = g[o];
                        texture->imageData[p + 2] = b[o];
                        if (texture->bpp == 32)
                            texture->imageData[p + 3] = a[o];
                    }
                }
            }
        }
        //texture->loaded = true;
        //texture->editable = true;
    }
    // Do not downsample textures while route editing. Terrain paint saves the
    // editable image back to ACE, so reducing it here can permanently turn a
    // 1024x1024 terrtex patch into 512x512 after painting.
    if(Game::textureQuality > 1 && !Game::writeEnabled
            && texture->width >= Game::textureQuality
            && texture->height >= Game::textureQuality){
        int nw = texture->width/Game::textureQuality;
        int nh = texture->height/Game::textureQuality;
        float scalew = (float)texture->width/nw;
        float scaleh = (float)texture->height/nh;
        texture->imageSize = (texture->bytesPerPixel * nw * nh );
        //qDebug() << texture->width << texture->height << nw << nh;
        unsigned char * nd = new unsigned char[texture->imageSize];

        for (int ii = 0; ii < nh; ii++) {
            for (int jj = 0; jj < nw; jj++) {
                int wsi = scaleh*ii;
                int hsi = scalew*jj;
                nd[ii*nw*texture->bytesPerPixel + jj*texture->bytesPerPixel+0] = texture->imageData[wsi*texture->width*texture->bytesPerPixel + hsi*texture->bytesPerPixel+0];
                nd[ii*nw*texture->bytesPerPixel + jj*texture->bytesPerPixel+1] = texture->imageData[wsi*texture->width*texture->bytesPerPixel + hsi*texture->bytesPerPixel+1];
                nd[ii*nw*texture->bytesPerPixel + jj*texture->bytesPerPixel+2] = texture->imageData[wsi*texture->width*texture->bytesPerPixel + hsi*texture->bytesPerPixel+2];
                if(texture->bytesPerPixel == 4)
                    nd[ii*nw*texture->bytesPerPixel + jj*texture->bytesPerPixel+3] = texture->imageData[wsi*texture->width*texture->bytesPerPixel + hsi*texture->bytesPerPixel+3];
            }
        }
        delete [] texture->imageData;
        texture->imageData = nd;
        texture->width = nw;
        texture->height = nh;
    }
    texture->loaded = true;
    texture->editable = true;
    //qDebug() << "--";
    //qDebug() << "2";
    return;
}
