/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef SFILEC_H
#define	SFILEC_H

#include "FileBuffer.h"

struct SFileGeometry;

class SFileC {
public:
    struct fvertex {
        unsigned short arg1;
        unsigned int point;
        unsigned int normal;
        unsigned short arg2;
        unsigned short arg3;
        unsigned short material;
        unsigned int uvpoint = 0;
    };
    static void odczytajshaders(FileBuffer* bufor, SFileGeometry* pliks);
    //static void szukajsekcjic(int szukane, FileBuffer* bufor);
    static void odczytajpunktyc(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajuvpunktyc(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajnormalnec(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajmatricesc(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajimagesc(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajtexturesc(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajvtx_statesc(FileBuffer* bufor, SFileGeometry* pliks);
    static void odczytajprim_statesc(FileBuffer* bufor, SFileGeometry* pliks);
    static bool odczytajloddc(FileBuffer* bufor, SFileGeometry* pliks, float alphaTest);
private:

};

#endif	/* SFILEC_H */

