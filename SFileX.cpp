/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "SFileX.h"
#include "SFile.h"
#include <QDebug>
#include <QtCore>
#include "ParserX.h"

void SFileX::odczytajshaders(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->ishaders = ParserX::GetNumber(bufor);

    pliks->shader.reset(new SFileGeometry::fshader[pliks->ishaders]);
    for (int i = 0; i < pliks->ishaders; i++) {
        while (bufor->get() != 40) {
            bufor->off++;
        }
        bufor->off++;
        bufor->off += 2;
        pliks->shader[i].name = ParserX::GetString(bufor).toLower();
        if(pliks->shader[i].name == "texdiff") 
            pliks->shader[i].alpha = 1;
        else 
            pliks->shader[i].alpha = 0;
        //qDebug() << pliks->shaderName[i];
        //console.log(pliks->image[i].name);
    }
    ParserX::SkipToken(bufor);
}

//-----------------------------------
//Oczytanie i zapisanie sekcji points
//-----------------------------------

void SFileX::odczytajpunkty(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->tpoints.ipoints = ParserX::GetNumber(bufor);
    //qDebug() << ilosc;
    pliks->tpoints.points.reset(new SFileGeometry::fpoint[pliks->tpoints.ipoints + 1]);
    for (int i = 0; i < pliks->tpoints.ipoints; i++) {
        pliks->tpoints.points[i].x = ParserX::GetNumber(bufor);
        pliks->tpoints.points[i].y = ParserX::GetNumber(bufor);
        pliks->tpoints.points[i].z = ParserX::GetNumber(bufor);
    }
    ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji uv_points
//-----------------------------------

void SFileX::odczytajuvpunkty(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->tpoints.iuv_points = ParserX::GetNumber(bufor);
    pliks->tpoints.uv_points.reset(new SFileGeometry::fpoint[ pliks->tpoints.iuv_points + 1]);
    for (int i = 0; i < pliks->tpoints.iuv_points; i++) {
        pliks->tpoints.uv_points[i].x = ParserX::GetNumber(bufor);
        pliks->tpoints.uv_points[i].y = ParserX::GetNumber(bufor);
    }
    ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji normals
//-----------------------------------

void SFileX::odczytajnormalne(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->tpoints.inormals = ParserX::GetNumber(bufor);
    pliks->tpoints.normals.reset(new SFileGeometry::fpoint[pliks->tpoints.inormals + 1]);
    for (int i = 0; i < pliks->tpoints.inormals; i++) {
        pliks->tpoints.normals[i].x = ParserX::GetNumber(bufor);
        pliks->tpoints.normals[i].y = ParserX::GetNumber(bufor);
        pliks->tpoints.normals[i].z = ParserX::GetNumber(bufor);
    }
    ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji matrices
//-----------------------------------

void SFileX::odczytajmatrices(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->iloscm = ParserX::GetNumber(bufor);
    pliks->macierz.reset(new SFileGeometry::matrt[pliks->iloscm + 1]);

    //System.out.println("iloscm: "+pliks->iloscm);
    for (int i = 0; i < pliks->iloscm; i++) {
        ParserX::NextTokenInside(bufor);
        pliks->macierz[i].name = ParserX::GetString(bufor);
        //qDebug() << pliks->macierz[i].name;
        while (bufor->get() != 40) {
            bufor->off++;
        }
        bufor->off++;
        //pliks->macierz[i] = new float[16];
        for (int j = 0; j < 16; j++) {
            if (j == 3 || j == 7 || j == 11) {
                pliks->macierz[i].param[j] = 0;
                continue;
            }
            if (j == 15) {
                pliks->macierz[i].param[j] = 1;
                continue;
            }
            pliks->macierz[i].param[j] = ParserX::GetNumber(bufor);
        }
        ParserX::SkipToken(bufor);
    }
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji images
//-----------------------------------

void SFileX::odczytajimages(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->ilosci = ParserX::GetNumber(bufor);
    pliks->image.reset(new SFileGeometry::imgs[pliks->ilosci + 1]);
    for (int i = 0; i < pliks->ilosci; i++) {
        while (bufor->get() != 40) {
            bufor->off++;
        }
        bufor->off++;
        bufor->off += 2;
        int j = 0;
        pliks->image[i].name = ParserX::GetString(bufor);
        pliks->image[i].tex = -1;
        //console.log(pliks->image[i].name);
    }
    ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji textures
//-----------------------------------

void SFileX::odczytajtextures(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->ilosct = ParserX::GetNumber(bufor);
    pliks->texture.reset(new SFileGeometry::text[pliks->ilosct]);
    for (int i = 0; i < pliks->ilosct; i++) {
        while (bufor->get() != 40) {
            bufor->off++;
        }
        bufor->off++;
        //pliks->texture[i] = new pliks->Texture();
        pliks->texture[i].image = ParserX::GetNumber(bufor);
        pliks->texture[i].arg1 = ParserX::GetNumber(bufor);
        pliks->texture[i].arg2 = ParserX::GetNumber(bufor);
        pliks->texture[i].arg3 = ParserX::GetNumber(bufor);
    }
    ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji vtx_states
//-----------------------------------

void SFileX::odczytajvtx_states(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->iloscv = ParserX::GetNumber(bufor);
    pliks->vtxstate.reset(new SFileGeometry::vtxs[pliks->iloscv]);
    for (int i = 0; i < pliks->iloscv; i++) {
        while (bufor->get() != 40) {
            bufor->off++;
        }
        bufor->off++;
        //pliks->vtxstate[i] = new pliks->Vtxs();
        pliks->vtxstate[i].arg1 = ParserX::GetHex(bufor);
        pliks->vtxstate[i].matrix = ParserX::GetNumber(bufor);
        pliks->vtxstate[i].arg2 = ParserX::GetNumber(bufor);
        pliks->vtxstate[i].arg3 = ParserX::GetNumber(bufor);

        while (bufor->get() != 41) {
            bufor->off++;
        }
        bufor->off++;
        pliks->vtxstate[i].arg4 = 0;
    }
    //ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji prim_states
//-----------------------------------

void SFileX::odczytajprim_states(FileBuffer* bufor, SFileGeometry* pliks) {
    pliks->iloscps = ParserX::GetNumber(bufor);
    pliks->primstate.reset(new SFileGeometry::primst[pliks->iloscps]);
    for (int i = 0; i < pliks->iloscps; i++) {
        while (bufor->get() != 40) {
            bufor->off++;
        }
        bufor->off++;
        //pliks->primstate[i] = new pliks->Primst();
        pliks->primstate[i].arg1 = ParserX::GetHex(bufor);
        pliks->primstate[i].arg2 = ParserX::GetNumber(bufor);
        pliks->primstate[i].arg3 = ParserX::GetNumber(bufor);
        pliks->primstate[i].arg4 = -1;
        for (int j = 0; j < pliks->primstate[i].arg3; j++) {
            if (j == 0) pliks->primstate[i].arg4 = ParserX::GetNumber(bufor);
            else ParserX::GetNumber(bufor);
        }
        pliks->primstate[i].arg5 = ParserX::GetNumber(bufor);
        pliks->primstate[i].vtx_state = ParserX::GetNumber(bufor);
        pliks->primstate[i].arg6 = ParserX::GetNumber(bufor);
        pliks->primstate[i].arg7 = ParserX::GetNumber(bufor);
        pliks->primstate[i].arg8 = ParserX::GetNumber(bufor);
    }
    ParserX::SkipToken(bufor);
    return;
};
//-----------------------------------
//Oczytanie i zapisanie sekcji loddetail
//-----------------------------------

bool SFileX::odczytajlodd(FileBuffer* bufor, SFileGeometry* pliks, float alphaTest) {
    QString sh = "";
    int i, j, ii, jj;
    int iloscs;
    int aktidx, czilosc = 0;

    std::unique_ptr<fvertex[]> vert;
    int vertexCapacity = 0;

    i = 0; // odczytujemy jeden lodcontrol=0;
    sh = "lod_control";
    ParserX::FindTokenDomIgnore(sh, bufor);
    //console.log("znaleziono sekcje " + sh + " na " + bufor.p);
    //szukamy sekcji distance levels
    sh = "distance_levels";
    ParserX::FindTokenDomIgnore(sh, bufor);
    //console.log("znaleziono sekcje " + sh + " na " + bufor.p);
    // wczytujemy ilosc distancelevels
    pliks->iloscd = ParserX::GetNumber(bufor);
    //qDebug() << "#SFile distance levels" << pliks->iloscd;
    pliks->distancelevel.reset(new SFileGeometry::dist[pliks->iloscd]);

    // wczytujemy distancelevls
    for (j = 0; j < pliks->iloscd; j++) {
        //pliks->distancelevel[j] = new SFile->Dist();

        sh = "distance_level";
        ParserX::FindTokenDomIgnore(sh, bufor);
        //qDebug() << QString("znaleziono sekcje " + sh + " na " + bufor->off);
        //wczytanie hierarchii
        sh = "distance_level_header";
        ParserX::FindTokenDomIgnore(sh, bufor);
        pliks->distancelevel[j].levelSelection = ParserX::GetNumber(bufor); //dlevel_selection//
        pliks->distancelevel[j].ilosch = ParserX::GetNumber(bufor);
        pliks->distancelevel[j].hierarchia.reset(new int[pliks->distancelevel[j].ilosch + 1]);
        for (ii = 0; ii < pliks->distancelevel[j].ilosch; ii++) {
            pliks->distancelevel[j].hierarchia[ii] = ParserX::GetNumber(bufor);
        }

        //szukamy subobjektow
        sh = "sub_objects";
        ParserX::FindTokenDomIgnore(sh, bufor);
        //console.log("znaleziono sekcje " + sh + " na " + bufor.p);
        //ilosc subobjektow
        pliks->distancelevel[j].iloscs = ParserX::GetNumber(bufor);
        // przydzielenie im pamieci
        pliks->distancelevel[j].subobiekty.reset(new SFileGeometry::sub[pliks->distancelevel[j].iloscs + 1]);

        //wczytujemy subobjekty
        iloscs = pliks->distancelevel[j].iloscs;
        for (ii = 0; ii < iloscs; ii++) {
            int vertexCount = 0;
            sh = "sub_object";
            ParserX::FindTokenDomIgnore(sh, bufor);

            while (!((sh = ParserX::NextTokenInside(bufor).toLower()) == "")) {
                if(sh == "sub_object_header"){
                    ParserX::GetHex(bufor);
                    ParserX::GetNumber(bufor);
                    ParserX::GetNumber(bufor);
                    ParserX::GetHex(bufor);
                    ParserX::GetHex(bufor);
                    while (!((sh = ParserX::NextTokenInside(bufor).toLower()) == "")) {
                        if(sh == "geometry_info"){
                            while (!((sh = ParserX::NextTokenInside(bufor).toLower()) == "")) {
                                if(sh == "geometry_nodes"){
                                    ParserX::SkipToken(bufor);
                                    continue;
                                }
                                if(sh == "geometry_node_map"){
                                    int count = ParserX::GetNumber(bufor);
                                    for(int ignm = 0; ignm < count; ignm++ ){
                                        pliks->distancelevel[j].subobiekty[ii].header.geometryNodeMap.push_back(ParserX::GetNumber(bufor));
                                    }
                                    ParserX::SkipToken(bufor);
                                    continue;
                                }
                                qDebug() << "#SFile geometry_info - undefined token: " << sh;
                                ParserX::SkipToken(bufor);
                            }
                            ParserX::SkipToken(bufor);
                            continue;
                        }
                        if(sh == "subobject_shaders"){
                            ParserX::SkipToken(bufor);
                            continue;
                        }
                        if(sh == "subobject_light_cfgs"){
                            ParserX::SkipToken(bufor);
                            continue;
                        }
                        qDebug() << "#SFile sub_object_header - undefined token: " << sh;
                        ParserX::SkipToken(bufor);
                    }
                    ParserX::SkipToken(bufor);
                    continue;
                }
                if(sh == "vertices"){
                    vertexCount = ParserX::GetNumber(bufor);
                    if (vertexCount < 0)
                        return false;
                    if (vertexCount > vertexCapacity) {
                        vert.reset(new fvertex[vertexCount]);
                        vertexCapacity = vertexCount;
                    }
                    for (jj = 0; jj < vertexCount; jj++) {
                        vert[jj].arg1 = (short) ParserX::GetHex(bufor);
                        vert[jj].point = (unsigned int) ParserX::GetNumber(bufor);
                        vert[jj].normal = (unsigned int) ParserX::GetNumber(bufor);
                        vert[jj].arg2 = (short) ParserX::GetHex(bufor);
                        vert[jj].arg3 = (short) ParserX::GetHex(bufor);

                        //if (jj < 20) Console.WriteLine("tu jest :" + bufor[ibufor]);
                        //if (jj < 20) Console.WriteLine("tu jest :" + bufor[ibufor]);
                        //vert[jj].arg2 = 
                        //if (jj < 20) Console.WriteLine("tu jest :" + bufor[ibufor]);
                        //vert[jj].arg3 = 
                        vert[jj].material = (short) ParserX::GetNumber(bufor);

                        vert[jj].uvpoint = 0;
                        for (int jjjj = 0; jjjj < vert[jj].material; jjjj++) {
                            if (jjjj == 0) vert[jj].uvpoint = (unsigned int) ParserX::GetNumber(bufor);
                            else ParserX::GetNumber(bufor);
                        }
                    }
                    ParserX::SkipToken(bufor);
                    ParserX::SkipToken(bufor);
                    ParserX::SkipToken(bufor);
                    continue;
                }
                if(sh == "vertex_sets"){
                    ParserX::SkipToken(bufor);
                    continue;
                }
                if(sh == "primitives"){
                    pliks->distancelevel[j].subobiekty[ii].iloscc = ParserX::GetNumber(bufor);
                    // przydzielenie im pamieci
                    pliks->distancelevel[j].subobiekty[ii].czesci.reset(new SFileGeometry::czes[pliks->distancelevel[j].subobiekty[ii].iloscc + 1]);

                    //wczytaj czesci
                    czilosc = 0;
                    aktidx = 0;

                    for (jj = 0; jj < pliks->distancelevel[j].subobiekty[ii].iloscc; jj++) {
                        //pliks->distancelevel[j].subobiekty[ii][czilosc] = new pliks->Czesc();
                        //qDebug() << "cc " << pliks->distancelevel[j].subobiekty[ii].iloscc;
                        //wybor sekcji lista czy indeks
                        //w = ParserX::sekcjap(bufor);
                        sh = ParserX::NextTokenDomIgnore(bufor).toLower();
                        //w = 
                        //console.log("ww " + w);
                        if (sh == "indexed_trilist") {
                            //jesli lista
                            pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].prim_state_idx = aktidx;
                            //wczytanie indeksow wierzcholkow
                            sh = "vertex_idxs";
                            ParserX::FindTokenDomIgnore(sh, bufor);
                            //console.log("znaleziono sekcje " + sh + " na " + bufor.p);
                            pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv = ParserX::GetNumber(bufor);

                            //pliks->distancelevel[j].subobiekty[ii][czilosc].pwierzcholki = new Float32Array(pliks->distancelevel[j].subobiekty[ii][czilosc].iloscv*8);
                            //float *wierzcholki = new float[pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv * 8];
                            //var nwierzcholki = new Float32Array(pliks->distancelevel[j].subobiekty[ii][czilosc].iloscv*3);
                            //var twierzcholki = new Float32Array(pliks->distancelevel[j].subobiekty[ii][czilosc].iloscv*2);
                            pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].idx.reset(new int[pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv]);
                            //lista wierzcholkow
                            //pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].wierzcholki = new Sfile.wie[pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv];
                            for (int iii = pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv - 1; iii >= 0; iii--) {
                                //pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].wierzcholki[iii] = new Sfile.wie();
                                //qDebug() << "cc " << pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv;
                                //w = ParserX::GetNumber(bufor);
                                pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].idx[iii] = ParserX::GetNumber(bufor);

                            }

                            //nwierzcholki = null;
                            //twierzcholki = null;
                            //pliks->distancelevel[j].subobiekty[ii][czilosc].pwierzcholki = pwierzcholki;
                            //pominiecie normals i flags 
                            sh = "normal_idxs";
                            ParserX::FindTokenDomIgnore(sh, bufor);
                            sh = "flags";
                            ParserX::FindTokenDomIgnore(sh, bufor);
                            ParserX::SkipToken(bufor);
                            czilosc++;
                        } else {
                            aktidx = ParserX::GetNumber(bufor);
                        }
                    }
                    pliks->distancelevel[j].subobiekty[ii].iloscc = czilosc;
                    //Console.WriteLine("wczytano cz " + czilosc);
                    /////////////////////////
                    if (!pliks->packVertices(pliks->distancelevel[j].subobiekty[ii], vert.get(),
                                             vertexCount, alphaTest))
                        return false;
                
                    ParserX::SkipToken(bufor);
                    ParserX::SkipToken(bufor);
                    continue;
                }
                qDebug() << "#SFile subobject - undefined token: " << sh;
                ParserX::SkipToken(bufor);
            }
        }
    }
    ParserX::SkipToken(bufor);
    ParserX::SkipToken(bufor);
    ParserX::SkipToken(bufor);
    ParserX::SkipToken(bufor);
    ParserX::SkipToken(bufor);
    //ParserX::SkipToken(bufor);
    //ParserX::SkipToken(bufor);

    pliks->tpoints.normals.reset();
    pliks->tpoints.points.reset();
    pliks->tpoints.uv_points.reset();
    return true;
};
