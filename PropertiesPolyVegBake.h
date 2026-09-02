/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors.
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later.
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef PROPERTIESPOLYVEGBAKE_H
#define PROPERTIESPOLYVEGBAKE_H

#include "PropertiesAbstract.h"

class PropertiesPolyVegBake : public PropertiesAbstract {
    Q_OBJECT
public:
    PropertiesPolyVegBake();
    virtual ~PropertiesPolyVegBake();
    bool support(GameObj *obj);
    void showObj(GameObj *obj);
    void updateObj(GameObj *obj);
    QPushButton *hacksButton();

signals:
    void hacksToggled(GameObj *selection, QPushButton *button, bool checked);

private:
    void refreshValues();

    QLabel *managedText = NULL;
    QLabel *uidFieldLabel = NULL;
    QLabel *tileXFieldLabel = NULL;
    QLabel *tileZFieldLabel = NULL;
    QLabel *shapeSectionLabel = NULL;
    QLabel *fileFieldLabel = NULL;
    QLabel *locationSectionLabel = NULL;
    QLabel *positionXFieldLabel = NULL;
    QLabel *positionYFieldLabel = NULL;
    QLabel *positionZFieldLabel = NULL;
    QPushButton hacks;
};

#endif /* PROPERTIESPOLYVEGBAKE_H */
