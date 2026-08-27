/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef ACELIB_H
#define	ACELIB_H

#include <QThread>
#include <QByteArray>
#include "Texture.h"

class AceLib : public QThread
 {
     Q_OBJECT

public:
    static bool IsThread;
    AceLib();
    AceLib(const AceLib& orig) = delete;
    virtual ~AceLib();
    //static bool LoadACE(Texture* texture);
    Texture* texture;
    static bool serialize(Texture *texture, QByteArray &data, QString *error = NULL);
    static bool save(QString path, Texture* t, QString *error = NULL);
    void run();
private:
    
protected:
    
};

#endif	/* ACELIB_H */

