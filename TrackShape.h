/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef TRACKSHAPE_H
#define	TRACKSHAPE_H

#include <QString>
#include "RouteSectionPolicy.h"

class QTextStream;
class FileBuffer;

class TrackShape {
public:
    struct SectionIdx{
        int n = 0;
        float pos[3]{};
        float rotDeg = 0;
        unsigned int sect[RouteSectionPolicy::trackPathCapacity]{};
    };
    
    QString filename;
    int id = -1;
    int numpaths = 0;
    int mainroute = -1;
    int clearancedist = -1;
    bool crossovershape = false;
    bool tunnelshape = false;
    bool roadshape = false;
    bool manualjunctionshape = false;
    bool dyntrack = false;
    SectionIdx *path = nullptr;
    int xoverpts = 0;
    float *xoverpt = NULL;
    
 //      } else if (sh == ("xoverpts")) {    
        
    TrackShape();
    TrackShape(int t);
    virtual ~TrackShape();
    QString getHashString();
    //void getShapePath();
    void saveToStream(QTextStream &out);
    bool loadUtf16Data(FileBuffer *data);
    static bool readIndex(FileBuffer *data, int &index);
    static bool atBlockEnd(FileBuffer *data);
    static bool readPath(FileBuffer *data, SectionIdx &result, bool positioned);
private:

};

#endif	/* TRACKSHAPE_H */

