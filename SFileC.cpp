/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "SFileC.h"
#include "SFile.h"
#include <QDebug>
#include "TS.h"

    void SFileC::odczytajshaders(FileBuffer* bufor, SFileGeometry* pliks) {
        int temp;

        bufor->off += 1;
        pliks->ishaders = bufor->getInt();

        pliks->shader.reset(new SFileGeometry::fshader[pliks->ishaders]);
        for (int i = 0; i < pliks->ishaders; i++) {
            bufor->off += 4;
            temp = bufor->get();
            bufor->off += 6;
            //pliks->image[i] = new SFileGeometry::imgs();
            pliks->shader[i].name = "";
            for (int j = 0; j < (temp - 3) / 2; j++) {
                pliks->shader[i].name += QChar(bufor->get());
                bufor->get();
            }
            pliks->shader[i].name = pliks->shader[i].name.toLower();
            if(pliks->shader[i].name == "texdiff") 
                pliks->shader[i].alpha = 1;
            else 
                pliks->shader[i].alpha = 0;
            //qDebug() << pliks->shaderName[i];
        }
        return;
    }

    //-----------------------------------
    //Szukanie sekcji
    //-----------------------------------
    /*void SFileC::szukajsekcjic(int szukane, FileBuffer* bufor) {
        int s;
        while (bufor->length>bufor->off) {
            s = (int)bufor->getInt();
            if (s == szukane) {
                return;
            }

            s = bufor->getInt();
            bufor->off += s;
        }
        //Console.WriteLine("nie znaleziono 7");
        return;
    }*/
    //-----------------------------------
    //Oczytanie i zapisanie sekcji points
    //-----------------------------------

    void SFileC::odczytajpunktyc(FileBuffer* bufor, SFileGeometry* pliks) {
        bufor->off += 1;
        pliks->tpoints.ipoints = bufor->getInt();

        pliks->tpoints.points.reset(new SFileGeometry::fpoint[ pliks->tpoints.ipoints + 1]);
        for (int i = 0; i <  pliks->tpoints.ipoints; i++) {
            bufor->off += 9;
            //pliks->tpoints.points[i] = new SFileGeometry::fpoint();
            pliks->tpoints.points[i].x = bufor->getFloat();
            pliks->tpoints.points[i].y = bufor->getFloat();
            pliks->tpoints.points[i].z = bufor->getFloat();
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji uv_points
    //-----------------------------------

    void SFileC::odczytajuvpunktyc(FileBuffer* bufor, SFileGeometry* pliks) {
        bufor->off += 1;
        pliks->tpoints.iuv_points = bufor->getInt();
        
        pliks->tpoints.uv_points.reset(new SFileGeometry::fpoint[pliks->tpoints.iuv_points + 1]);
        for (int i = 0; i < pliks->tpoints.iuv_points; i++) {
            bufor->off += 9;
            //pliks->tpoints.uv_points[i] = new SFileGeometry::fpoint();
            pliks->tpoints.uv_points[i].x = bufor->getFloat();
            pliks->tpoints.uv_points[i].y = bufor->getFloat();
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji normals
    //-----------------------------------

    void SFileC::odczytajnormalnec(FileBuffer* bufor, SFileGeometry* pliks) {
        bufor->off += 1;
        pliks->tpoints.inormals = bufor->getInt();
        pliks->tpoints.normals.reset(new SFileGeometry::fpoint[pliks->tpoints.inormals + 1]);
        for (int i = 0; i < pliks->tpoints.inormals; i++) {
            bufor->off += 9;
            //pliks->tpoints.normals[i] = new SFileGeometry::fpoint();
            pliks->tpoints.normals[i].x = bufor->getFloat();
            pliks->tpoints.normals[i].y = bufor->getFloat();
            pliks->tpoints.normals[i].z = bufor->getFloat();
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji matrices
    //-----------------------------------

    void SFileC::odczytajmatricesc(FileBuffer* bufor, SFileGeometry* pliks) {
        int temp;

        bufor->off += 1;
        pliks->iloscm = bufor->getInt();

        pliks->macierz.reset(new SFileGeometry::matrt[pliks->iloscm + 1]);
        for (int i = 0; i < pliks->iloscm; i++) {
            bufor->off += 8;
            temp = bufor->get();
            const std::unique_ptr<QString> name(bufor->getString(bufor->off, bufor->off + temp*2));
            pliks->macierz[i].name = *name;
            //qDebug() << pliks->macierz[i].name;
            bufor->off += temp*2;
            //pliks->macierz[i] = new SFileGeometry::matrt();
            for (int j = 0; j < 16; j++) {
                if(j==3 || j==7 || j==11) {
                    pliks->macierz[i].param[j] = 0;
                    continue;
                }
                if(j==15) {
                    pliks->macierz[i].param[j] = 1;
                    continue;
                }
                pliks->macierz[i].param[j] = bufor->getFloat();
            }
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji images
    //-----------------------------------

    void SFileC::odczytajimagesc(FileBuffer* bufor, SFileGeometry* pliks) {
        int temp;

        bufor->off += 1;
        pliks->ilosci = bufor->getInt();

        pliks->image.reset(new SFileGeometry::imgs[pliks->ilosci + 1]);
        for (int i = 0; i < pliks->ilosci; i++) {
            bufor->off += 4;
            temp = bufor->get();
            bufor->off += 6;
            //pliks->image[i] = new SFileGeometry::imgs();
            pliks->image[i].name = "";
            for (int j = 0; j < (temp - 3) / 2; j++) {
                pliks->image[i].name += QChar(bufor->get());
                bufor->get();
            }
            pliks->image[i].tex = -1;
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji textures
    //-----------------------------------

    void SFileC::odczytajtexturesc(FileBuffer* bufor, SFileGeometry* pliks) {

        bufor->off += 1;

        pliks->ilosct = bufor->getInt();

        pliks->texture.reset(new SFileGeometry::text[pliks->ilosct]);

        for (int i = 0; i < pliks->ilosct; i++) {
            //pliks->texture[i] = new SFileGeometry::text();
            bufor->off += 9;
            pliks->texture[i].image = bufor->getInt();
            pliks->texture[i].arg1 = bufor->getInt();
            pliks->texture[i].arg2 = bufor->getInt();
            pliks->texture[i].arg3 = bufor->getInt();
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji vtx_states
    //-----------------------------------

    void SFileC::odczytajvtx_statesc(FileBuffer* bufor, SFileGeometry* pliks) {

        bufor->off += 1;
        pliks->iloscv = bufor->getInt();

        pliks->vtxstate.reset(new SFileGeometry::vtxs[pliks->iloscv]);

        for (int i = 0; i < pliks->iloscv; i++) {
            bufor->off += 9;
            //pliks->vtxstate[i] = new SFileGeometry::vtxs();
            pliks->vtxstate[i].arg1 = bufor->getInt();
            pliks->vtxstate[i].matrix = bufor->getInt();
            pliks->vtxstate[i].arg2 = bufor->getInt();
            pliks->vtxstate[i].arg3 = bufor->getInt();
            pliks->vtxstate[i].arg4 = bufor->getInt();
        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji prim_states
    //-----------------------------------

    void SFileC::odczytajprim_statesc(FileBuffer* bufor, SFileGeometry* pliks) {
        int temp;

        bufor->off += 1;
        pliks->iloscps = bufor->getInt();

        pliks->primstate.reset(new SFileGeometry::primst[pliks->iloscps]);
        for (int i = 0; i < pliks->iloscps; i++) {
            bufor->off += 8;
            temp = bufor->get();
            bufor->off += temp*2;

            //pliks->primstate[i] = new SFileGeometry::primst();

            pliks->primstate[i].arg1 = bufor->getInt();
            pliks->primstate[i].arg2 = bufor->getInt();

            bufor->off += 9;

            pliks->primstate[i].arg3 = bufor->getInt();
            pliks->primstate[i].arg4 = -1;
            for(int j = 0; j<pliks->primstate[i].arg3; j++){
                if(j == 0) pliks->primstate[i].arg4 = bufor->getInt();
                else bufor->getInt();
            }
            
            pliks->primstate[i].arg5 = bufor->getInt();

            pliks->primstate[i].vtx_state = bufor->getInt();

            pliks->primstate[i].arg6 = bufor->getInt();
            pliks->primstate[i].arg7 = bufor->getInt();
            pliks->primstate[i].arg8 = bufor->getInt();

        }
        return;
    }
    //-----------------------------------
    //Oczytanie i zapisanie sekcji loddetail
    //-----------------------------------

    bool SFileC::odczytajloddc(FileBuffer* bufor, SFileGeometry* pliks, float alphaTest) {
        int temp;
        int i;

        std::unique_ptr<fvertex[]> vert;
        int vertexCapacity = 0;

        bufor->off += 5; // pominiecie ilosci lodcontrols;

        //Odczytanie jednego LOD_Control=0;
        i = 0;

        bufor->off += 13;
        // pominiecie sekcji;
        temp = bufor->getInt();

        bufor->off += temp+9;

        // odczytanie ilosci distance levels
        pliks->iloscd = bufor->getInt();

        // przydzielenie dla nich pamieci
        pliks->distancelevel.reset(new SFileGeometry::dist[pliks->iloscd]);

        for (int j = 0; j < pliks->iloscd; j++) {
            //pliks.distancelevel[j] = new Sfile.dist();

            bufor->off += 27;
            // pomieniecie sekcji dlevel selection
            //bufor->off += 4;
            pliks->distancelevel[j].levelSelection = bufor->getFloat();
            bufor->off += 9;
            // odczytanie dlugosci hierarchii obiektow
            pliks->distancelevel[j].ilosch = bufor->getInt();

            // przydzielenie dla niej pamieci
            pliks->distancelevel[j].hierarchia.reset(new int[pliks->distancelevel[j].ilosch + 1]);
            // wczytanie hierarchii
            for (int ii = 0; ii < pliks->distancelevel[j].ilosch; ii++) {
                pliks->distancelevel[j].hierarchia[ii] = bufor->getInt();
            }

            // odczytanie ilosci subobjektow
            bufor->off += 9;
            pliks->distancelevel[j].iloscs = bufor->getInt();
            //qDebug () << "--iloscs " << pliks->distancelevel[j].iloscs;

            // przydzielenie im pamieci
            pliks->distancelevel[j].subobiekty.reset(new SFileGeometry::sub[pliks->distancelevel[j].iloscs + 1]);
            // wczytanie subobjektow
            for (int ii = 0; ii < pliks->distancelevel[j].iloscs; ii++) {
                //pliks->distancelevel[j].subobiekty[ii] = new SFileGeometry::sub();

                
                bufor->off += 9;
                bufor->findToken(TS::sub_object_header);
                //qDebug() << "sub_object_header";
                int offset1 = bufor->getInt();
                int akto1 = bufor->off;
                bufor->off++;
                bufor->off += 20;
                for (;;) {
                    int pozycja2,offset2,akto2;
                    pozycja2 = bufor->getInt();
                    offset2 = bufor->getInt();
                    akto2 = bufor->off;
                    switch (pozycja2) {
                        case TS::subobject_shaders:
                            break;
                        case TS::subobject_light_cfgs:
                            break;
                        case TS::geometry_info:
                            int pozycja3,offset3,akto3;
                            bufor->off++;
                            bufor->off+=40;
                            for (;;) {
                                pozycja3 = bufor->getInt();
                                offset3 = bufor->getInt();
                                akto3 = bufor->off;
                                int count;
                                switch (pozycja3) {
                                    case TS::geometry_nodes:
                                        break;
                                    case TS::geometry_node_map:
                                        bufor->off++;
                                        count = bufor->getInt();
                                        for(int ignm = 0; ignm < count; ignm++ ){
                                            pliks->distancelevel[j].subobiekty[ii].header.geometryNodeMap.push_back(bufor->getInt());
                                        }
                                        break;
                                    default:
                                        qDebug() << "#SFile geometry_info - unknown token: "<< pozycja3;
                                        break;
                                }
                                bufor->off = akto3 + offset3;
                                if(bufor->off >= akto2 + offset2) break;
                            }
                            break;
                        default:
                            //qDebug() << "#SFile sub_object_header - unknown token: "<< pozycja2 << TS::IdName[pozycja2];
                            break;
                    }
                    bufor->off = akto2 + offset2;
                    if(bufor->off >= akto1 + offset1) break;
                }
                bufor->off = akto1 + offset1;
                
                // Wczytanie wierzcholkow
                bufor->findToken(TS::vertices);
                bufor->off += 5;
                const int v_ilosc = bufor->getInt();
                if (v_ilosc < 0)
                    return false;
                if (v_ilosc > vertexCapacity) {
                    vert.reset(new fvertex[v_ilosc]);
                    vertexCapacity = v_ilosc;
                }
                
                //qDebug () << "iloscv " << v_ilosc;
                
                for (int jj = 0; jj < v_ilosc; jj++) {
                    bufor->off += 9;
                    //vert[jj] = new fvertex();
                    vert[jj].arg1 = (short) bufor->getInt();
                    vert[jj].point = (unsigned int) bufor->getInt();
                    vert[jj].normal = (unsigned int) bufor->getInt();
                    vert[jj].arg2 = (short) bufor->getInt();
                    vert[jj].arg3 = (short) bufor->getInt();

                    bufor->off += 9;
                    vert[jj].material = (short) bufor->getInt();
                    
                    vert[jj].uvpoint = 0;
                    for(int jjjj = 0; jjjj<vert[jj].material; jjjj++){
                        if(jjjj == 0) vert[jj].uvpoint = (unsigned int) bufor->getInt();
                        else bufor->getInt();
                    }
                    
                }

                // odczytanie primitives-'czesci'
                bufor->findToken(53);
                bufor->off += 5;
                // odczytanie ich ilosci
                pliks->distancelevel[j].subobiekty[ii].iloscc = bufor->getInt();
                //System.out.println("iloscc "+pliks.distancelevel[j].subobiekty[ii].iloscc);
                int czilosc = 0, aktidx = 0;
                // przydzielenie im pamieci   
                pliks->distancelevel[j].subobiekty[ii].czesci.reset(new SFileGeometry::czes[pliks->distancelevel[j].subobiekty[ii].iloscc + 1]);

                // wczytanie czesci  
                for (int jj = 0; jj < pliks->distancelevel[j].subobiekty[ii].iloscc; jj++) {
                    //pliks->distancelevel[j]->subobiekty[ii].czesci[czilosc] = new SFileGeometry::czes();
                    //qDebug() << "cc " << pliks->distancelevel[j].subobiekty[ii].iloscc;
                    temp = bufor->getInt();

                    if (temp == 56) {
                        bufor->off += 5;
                        aktidx = bufor->getInt();

                    } else {
                        bufor->off += 5 + 9;
                        pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].prim_state_idx = aktidx;
                        pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv = bufor->getInt();

                        //float *wierzcholki = new float[pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv*8];

                        // wczytanie listy wierzcholkow
                        //int directxSmierdzi = 0;
                        //for (int iii = 0; iii < pliks.distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv; iii++) {
                        
                        pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].idx.reset(new int[pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv]);
                        for (int iii = pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].iloscv - 1; iii >= 0; iii--) {
                            //pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].wierzcholki[iii] = new SFileGeometry::wie();
                            pliks->distancelevel[j].subobiekty[ii].czesci[czilosc].idx[iii] = bufor->getInt();
                            //w = bufor->getInt();
                            //System.out.println("----v "+w);

                            //directxSmierdzi-=2;
                            //if(directxSmierdzi<-2) directxSmierdzi = 2;
                        }

                        
                        bufor->off += 4;
                        temp = bufor->getInt();
                        bufor->off += temp;

                        bufor->off += 4;
                        temp = bufor->getInt();
                        bufor->off += temp;

                        czilosc++;
                        
                    }
                }
                pliks->distancelevel[j].subobiekty[ii].iloscc = czilosc;
                /////////////////////////
                if (!pliks->packVertices(pliks->distancelevel[j].subobiekty[ii], vert.get(),
                                         v_ilosc, alphaTest))
                    return false;
            }
        }

        pliks->tpoints.normals.reset();
        pliks->tpoints.points.reset();
        pliks->tpoints.uv_points.reset();
        return true;
    }
