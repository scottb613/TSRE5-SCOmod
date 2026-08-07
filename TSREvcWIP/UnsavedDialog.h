/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef UNSAVEDDIALOG_H
#define	UNSAVEDDIALOG_H

#include <QtWidgets>

class UnsavedDialog : public QDialog {
    Q_OBJECT
public:
    explicit UnsavedDialog(QWidget *parent = NULL);
    UnsavedDialog(QString buttonLayout, QWidget *parent = NULL);
    virtual ~UnsavedDialog();
    QListWidget items;
    void setMsg(QString msg);
    void setSubtitle(QString subtitle);
    void hideButtons();
    int changed = 0;
    
public slots:
    void ok();
    void okt();
    void okw();    
    void cancel();
    void exit();
    
private:
    QLabel warningIcon;
    QLabel subtitleLabel;
    QLabel infoLabel;
    QPushButton* bok = NULL;
    QPushButton* bokt = NULL;
    QPushButton* bokw = NULL;
    QPushButton* bexit = NULL;
    QPushButton* bcancel = NULL;
};

#endif	/* UNSAVEDDIALOG_H */

