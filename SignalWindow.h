/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef SIGNALWINDOW_H
#define	SIGNALWINDOW_H

#include <QtWidgets>
#include "GuiFunct.h"

class SignalObj;

class SignalWindow : public EditorPopupWindow {
    Q_OBJECT

public:
    SignalWindow(QWidget *parent);
    virtual ~SignalWindow();
    void showObj(SignalObj* obj);
    void updateObj(SignalObj* obj);
    void showForOwner();
    
public slots:
    void exitNow();
    void setLink();
    void chSubEnabled(int i);
    void bLinkEnabled(int i);
    
signals:
    void sendMsg(QString name, QString val);
    void helperClosed();
    void userButtonPressed();

protected:
    void closeEvent(QCloseEvent *event) override;
    void popupPinClicked() override;
    
private:
    static const int maxSubObj = 32;
    int currentSubObjLinkInfo = 0;
    QLineEdit name;
    QLineEdit description;
    QCheckBox chSub[maxSubObj];
    QPushButton bSub[maxSubObj];
    QLineEdit dSub[maxSubObj];
    QGridLayout* vSub[maxSubObj];
    QWidget* wSub[maxSubObj];
    QSignalMapper signalsChSect;
    QSignalMapper signalsLinkButton;
    SignalObj* sobj;
    QPushButton* setLinkButton;
    QLineEdit eLink1;
    QLineEdit eLink2;
    QLineEdit eLink3;
    
    void setLinkInfo(int i);
};

#endif	/* SIGNALWINDOW_H */

