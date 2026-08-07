/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TrackItemObj.h"
#include "Game.h"

TrackItemObj::TrackItemObj(int type) : OglObj() { 
    if(type == 0){
        float punkty[18*3]{
            -0.5,-0.5,0,
            -0.5,0.5,0,
            0.5,0.5,0,
            0.5,0.5,0,
            0.5,-0.5,0,
            -0.5,-0.5,0,
            -0.5,0.5,0,
            -0.5,-0.5,0,
            0,0,1,
            0.5,0.5,0,
            0,0,1,
            0.5,-0.5,0,
            0.5,-0.5,0,
            0,0,1,
            -0.5,-0.5,0,
            0.5,0.5,0,
            -0.5,0.5,0,
            0,0,1
            };        int ptr = 18*3;    /// was 18*3
        this->setMaterial(0.0, 1.0, 0.0);
        initLitTriangles(punkty, ptr);
        //delete[] punkty;
    }    else if(type == 2) {   // classic pyramid
        float punkty[18*3]{
            -0.5,-0.5,0,
            -0.5,0.5,0,
            0.5,0.5,0,
            0.5,0.5,0,
            0.5,-0.5,0,
            -0.5,-0.5,0,
            -0.5,0.5,0,
            -0.5,-0.5,0,
            0,0,1,
            0.5,0.5,0,
            0,0,1,
            0.5,-0.5,0,
            0.5,-0.5,0,
            0,0,1,
            -0.5,-0.5,0,
            0.5,0.5,0,
            -0.5,0.5,0,
            0,0,1
            };
        int ptr = 18*3;
        this->setMaterial(0.0, 1.0, 0.0);
        initLitTriangles(punkty, ptr);
        //delete[] punkty;

    } else if(type == 3) {
        //// EFO  Arrow pointing into protection for speedzones and signals, value 3 is negative
        float punkty[30*3]{
             -0.8,-0.6, 0,    // base-1 1
              0.8,-0.6, 0,    // base-1 2
              0.8,-0.6,-0.4,  // base-1 3 GOOD
              0.8,-0.6,-0.4,  // base-2 1
             -0.8,-0.6,-0.4,  // base-2 2
             -0.8,-0.6, 0,    // base-2 3 GOOD        
             0.15,-0.6,-1,    // stem-1 1
            -0.15,-0.6,-1,    // stem-1 2
            -0.15,-0.6,-0.4,  // stem-1 3 gOOD
            -0.15,-0.6,-0.4,  // stem-2 1
             0.15,-0.6,-0.4,  // stem-2 2                         
             0.15,-0.6,-1,    // stem-2 3          
             0.5 ,-0.6,-1,    // tip 1
             0   ,-0.6,-1.5,    // tip 2
            -0.5 ,-0.6,-1,    // tip 3

             /// invert the bottom  231
              0.8,-0.6,-0.4,  // base-1 3 GOOD
              0.8,-0.6, 0,    // base-1 2
             -0.8,-0.6, 0,    // base-1 1
             -0.8,-0.6, 0,    // base-2 3 GOOD        
             -0.8,-0.6,-0.4,  // base-2 2
              0.8,-0.6,-0.4,  // base-2 1
            -0.15,-0.6,-0.4,  // stem-1 3 gOOD
            -0.15,-0.6,-1,    // stem-1 2
             0.15,-0.6,-1,    // stem-1 1
             0.15,-0.6,-1,    // stem-2 3          
             0.15,-0.6,-0.4,  // stem-2 2                         
            -0.15,-0.6,-0.4,  // stem-2 1
            -0.5 ,-0.6,-1,    // tip 3
             0   ,-0.6,-1.5,    // tip 2
             0.5 ,-0.6,-1,    // tip 1
             
            };
        int ptr = 30*3;
        this->setMaterial(1.0, 0.0, 0.0);
        this->init(punkty, ptr, RenderItem::V, GL_TRIANGLES);
        
        //delete[] punkty;
    } else if(type == 4) {
        //// EFO  Arrow pointing into object for platforms and sidings, value 3 is positive  
        float punkty[49*3]{
              -0.8, -0.6, 0,    // base-1 1
              0.8, -0.6, 0.4,   // base-1 2 GOOD
              0.8, -0.6, 0,    // base-1 3
              
              0.8, -0.6, 0.4,  // base-2 1
              -0.8, -0.6, 0,     // base-2 2 GOOD        
             -0.8, -0.6, 0.4,  // base-2 3
              
             0.15, -0.6, 1,    // stem-1 1            
            -0.15, -0.6, 0.4,  // stem-1 2 gOOD
              -0.15, -0.6, 1,    // stem-1 3

              -0.15, -0.6, 0.4,  // stem-2 1                                      
             0.15, -0.6, 1,    // stem-2 2          
              0.15, -0.6, 0.4,  // stem-2 3

              0   , -0.6, 1.5,    // tip 1
              0.5 , -0.6, 1,    // tip 2
             -0.5 , -0.6, 1,    // tip 3
              
             -0.05 ,-0.6 ,0,     /// tip 4 1
              0.05 ,-0.6 ,0,      /// tip 4 2
              0   ,  0,0,     /// tip 4 3
              
              //// invert for the bottom to 321
              
              0.8, -0.6, 0,    // base-1 3
              0.8, -0.6, 0.4,   // base-1 2 
              -0.8, -0.6, 0,    // base-1 1
              
             -0.8, -0.6, 0.4,  // base-2 3
              -0.8, -0.6, 0,     // base-2 2         
              0.8, -0.6, 0.4,  // base-2 1
              
              -0.15, -0.6, 1,    // stem-1 3
            -0.15, -0.6, 0.4,  // stem-1 2 
             0.15, -0.6, 1,    // stem-1 1            

              0.15, -0.6, 0.4,  // stem-2 3
             0.15, -0.6, 1,    // stem-2 2          
              -0.15, -0.6, 0.4,  // stem-2 1                                      

             -0.5 , -0.6, 1,    // tip 3
              0.5 , -0.6, 1,    // tip 2
              0   , -0.6, 1.5,    // tip 1
              
              0   ,  0,0,     /// tip 4 3
              0.05 ,-0.6 ,0,      /// tip 4 2
             -0.05 ,-0.6 ,0,     /// tip 4 1
              
              
              // wire hangar
              0.05 ,-0.6,0,      /// tip 4 2              
              -0.05 ,-0.6,0,     /// tip 4 1
              0   ,   0,0,     /// tip 4 3                  
            };
        int ptr = 49*3;
        this->setMaterial(1.0, 0.0, 0.0);
        this->init(punkty, ptr, RenderItem::V, GL_TRIANGLES);
        //delete[] punkty;
    }     
    else if(type == 5){   /// updated pyramid
        float punkty[18*3]{    /// was 18*3
            -0.40,-0.40,-1,    // flat face 5
            -0.40,0.40,-1,     // flat face 5
            0.40,0.40,-1,      // flat face 5
            0.40,0.40,-1,      // flat face 6
            0.40,-0.40,-1,     // flat face 6
            -0.40,-0.40,-1,    // flat face 6
            -0.5,0.4,-1,     // left face 1
            -0.5,-0.4,-1,    // left face 1
            0,0,0,           // left face 1
            0.5,0.4,-1,      // right face 2
            0,0,0,          // right face 2
            0.5,-0.4,-1,     // right face 2
            0.4,-0.5,-1,     // bottom face 3
            0,0,0,            // bottom face 3
            -0.4,-0.5,-1,    // bottom face 3
            0.4,0.5,-1,      // top face 4 
            -0.4,0.5,-1,     // top face 4
            0,0,0            // top face 4
            };
        int ptr = 18*3;    /// was 18*3
        this->setMaterial(0.0, 1.0, 0.0);
        initLitTriangles(punkty, ptr);
        //delete[] punkty;
    }
     else if(type == 6) {
        //// EFO  Arrow pointing down for platforms and sidings, value 3 is positive  
        float punkty[30*3]{
              -0.40, -0.25, 0,    // base-1 1
               0.40, -0.25, 0,   // base-1 2 GOOD
               0.40, -0.5, 0,    // base-1 3
              
               0.40, -0.5, 0,  // base-2 1
              -0.40, -0.5, 0,     // base-2 2 GOOD        
              -0.40, -0.25, 0,  // base-2 3
              
              -0.08, -0.5, 0,    // stem-1 1
               0.08, -0.5, 0,   // stem-1 2 GOOD
               0.08, -0.75, 0,    // stem-1 3
              
               0.08, -0.75, 0,  // stem-2 1
              -0.08, -0.75, 0,     // stem-2 2 GOOD        
              -0.08, -0.5, 0,  // stem-2 3

              -0.25, -0.75, 0,    // tip 1
                 0 , -1.25, 0,    // tip 2
               0.25 , -0.75, 0,    // tip 3
              
              
              //// invert for the bottom to 321

               0.40, -0.5, 0,    // base-1 3
               0.40, -0.25, 0,   // base-1 2 GOOD
              -0.40, -0.25, 0,    // base-1 1
              
              -0.40, -0.25, 0,  // base-2 3
              -0.40, -0.5, 0,     // base-2 2 GOOD        
               0.40, -0.5, 0,  // base-2 1
              
               0.08, -0.75, 0,    // stem-1 3
               0.08, -0.5, 0,   // stem-1 2 GOOD
              -0.08, -0.5, 0,    // stem-1 1
              
              -0.08, -0.5, 0,  // stem-2 3
              -0.08, -0.75, 0,     // stem-2 2 GOOD        
               0.08, -0.75, 0,  // stem-2 1

               0.25 , -0.75, 0,    // tip 3
                 0 , -1.25, 0,    // tip 2
              -0.25, -0.75, 0    // tip 1
                      
            };
        int ptr = 30*3;
        this->setMaterial(0.0, 0.0, 0.0);
        this->init(punkty, ptr, RenderItem::V, GL_TRIANGLES);
        //delete[] punkty;
    }     
    
    
    else {   // classic box for level crossings and TrItem display
        float punkty[36*3]{
            -0.5f,-0.5f,-0.5f, 
            -0.5f,-0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f, 
            0.5f, 0.5f,-0.5f, 
            -0.5f,-0.5f,-0.5f,
            -0.5f, 0.5f,-0.5f,
            0.5f,-0.5f, 0.5f,
            -0.5f,-0.5f,-0.5f,
            0.5f,-0.5f,-0.5f,
            0.5f, 0.5f,-0.5f,
            0.5f,-0.5f,-0.5f,
            -0.5f,-0.5f,-0.5f,
            -0.5f,-0.5f,-0.5f,
            -0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f,-0.5f,
            0.5f,-0.5f, 0.5f,
            -0.5f,-0.5f, 0.5f,
            -0.5f,-0.5f,-0.5f,
            -0.5f, 0.5f, 0.5f,
            -0.5f,-0.5f, 0.5f,
            0.5f,-0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f,-0.5f,-0.5f,
            0.5f, 0.5f,-0.5f,
            0.5f,-0.5f,-0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f,-0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
            0.5f, 0.5f,-0.5f,
            -0.5f, 0.5f,-0.5f,
            0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f,-0.5f,
            -0.5f, 0.5f, 0.5f,
            0.5f, 0.5f, 0.5f,
            -0.5f, 0.5f, 0.5f,
            0.5f,-0.5f, 0.5f
        };
        int ptr = 36*3;
        this->setMaterial(0.0, 1.0, 0.0);
        initLitTriangles(punkty, ptr);
        //delete[] punkty;
    }
}

TrackItemObj::TrackItemObj(const TrackItemObj& orig) {
}

TrackItemObj::~TrackItemObj() {
}

/// EFO dangerous ground here  -- adding a move signalObj TRITEM

void TrackItemObj::translate(float px, float py, float pz){
//    if(pz == 0) 
//        return;
//    TDB* tdb = Game::trackDB;
  
    qDebug() << "TRItem Translate: " << px << "/" << py << "/" << pz;

}
    /*
     
//    int id = tdb->findTrItemNodeId(this->getTrItemId[this->selectionValue]);
//    if (id < 0) {
//        qDebug() << "fail id";
//        return;
    }
    
    float dlugosc = tdb->getVectorSectionLength(id);
    TRitem* trit = tdb->trackItems[this->getTrItemId[this->selectionValue]];
    if(trit == NULL) 
        return;
    if(pz < 0){
        trit->trackPositionAdd(-1);
        if(trit->getTrackPosition() < 0)
            trit->setTrackPosition(0);
        tdb->updateTrItemRData(trit);
    } else if(pz > 0){
        trit->trackPositionAdd(1);
        if(trit->getTrackPosition() > dlugosc)
            trit->setTrackPosition(dlugosc);
        tdb->updateTrItemRData(trit);
    }
    
     // this is from the platform obj,only relevent with two objs tied togeth by a line
    if(this->selectionValue == 1){
        delete[] drawPositionB;
        drawPositionB = NULL;   
    } else if(this->selectionValue == 3){
        delete[] drawPositionE;
        drawPositionE = NULL;
    }
    delete line;      
    line = NULL;      
     */
    
//    delete[] drawPositions;
//    drawPositions = NULL;
//    setModified();   /// legit -- keep this
//    setMartix();     /// legit -- keep this}
