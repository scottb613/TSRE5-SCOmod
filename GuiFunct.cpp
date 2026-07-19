/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "GuiFunct.h"
#include <QtWidgets>
#include "Game.h"

QLabel* GuiFunct::newQLabel(QString text, int width){
    QLabel* label = new QLabel(text);
    label->setFixedWidth(width);
    return label;
}

QLabel* GuiFunct::newTQLabel(QString text, int width){
    QLabel *l = new QLabel(text);
    l->setContentsMargins(3,0,0,0);
    l->setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; }");
    if(width >=0)
        l->setMinimumWidth(width);
    return l;
}

QLineEdit* GuiFunct::newQLineEdit(int width, int length){
    QLineEdit* edit = new QLineEdit;
    edit->setFixedWidth(width);
    edit->setMaxLength(length);
    return edit;
}

QAction* GuiFunct::newMenuCheckAction(QString desc, QWidget* window, bool checked){
    QAction *action = new QAction(desc, window);
    action->setCheckable(true);
    action->setChecked(checked);
    return action;
}

QString GuiFunct::scoPanelStyle(){
    return QString(
        "QWidget { background-color: #303030; color: #f2f2f2; }"
        "QPushButton {"
        " color: white;"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #606060, stop:0.48 #535353, stop:1 #414141);"
        " border: 1px solid #707070; border-bottom-color: #292929; border-radius: 2px; padding: 2px 5px;"
        "}"
        "QPushButton:hover {"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #707070, stop:1 #505050);"
        " border-color: #888888;"
        "}"
        "QPushButton:pressed {"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #383838, stop:1 #505050);"
        " padding-top: 3px; padding-bottom: 1px;"
        "}"
        "QPushButton:checked {"
        " color: #171717;"
        " background: qlineargradient(x1:0,y1:0,x2:0,y2:1, stop:0 #f7a32d, stop:1 #d46f00);"
        " border-color: #ffad3b; border-bottom-color: #713900;"
        "}"
        "QPushButton:disabled { color: #858585; background: #3b3b3b; border-color: #4b4b4b; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QTimeEdit, QPlainTextEdit, QComboBox {"
        " background-color: #202020; color: white; border: 1px solid #555555;"
        " border-top-color: #151515; border-radius: 1px; padding: 1px 3px; selection-background-color: #f08200; selection-color: black;"
        "}"
        "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled {"
        " background-color: #252525; color: #b0b0b0; border-color: #444444;"
        "}"
        "QListWidget, QListView, QTreeWidget, QTableWidget {"
        " background-color: #191919; color: white; border: 1px solid #4c4c4c; outline: none;"
        "}"
        "QAbstractItemView::item:selected { background-color: #f08200; color: black; }"
        "QCheckBox, QRadioButton { color: white; spacing: 5px; }"
        "QCheckBox::indicator {"
        " width: 13px; height: 13px; background-color: #202020; border: 1px solid #9a9a9a;"
        "}"
        "QCheckBox::indicator:hover { border-color: #f08200; }"
        "QCheckBox::indicator:checked {"
        " background-color: #f08200; border-color: #ffad3b;"
        "}"
        "QRadioButton::indicator {"
        " width: 13px; height: 13px; background-color: #202020; border: 1px solid #9a9a9a; border-radius: 7px;"
        "}"
        "QRadioButton::indicator:hover { border-color: #f08200; }"
        "QRadioButton::indicator:checked { background-color: #f08200; border-color: #ffad3b; border-radius: 7px; }"
        "QSlider::groove:horizontal { height: 4px; background: #1d1d1d; border: 1px solid #474747; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #b76512; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 10px; margin: -4px 0; background: #747474; border: 1px solid #989898; border-radius: 2px; }"
        "QSlider::handle:horizontal:hover { background: #8a8a8a; border-color: #f08200; }"
        "QToolTip { color: white; background-color: #252525; border: 1px solid #777777; padding: 3px; }"
    );
}
