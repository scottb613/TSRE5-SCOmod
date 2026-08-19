/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "SignalWindow.h"
#include <QDebug>
#include "SignalObj.h"
#include "Game.h"
#include "TDB.h"
#include "SigCfg.h"
#include "SignalShape.h"
#include "SignalWindowLink.h"
#include "Undo.h"
#include "GuiFunct.h"

SignalWindow::SignalWindow(QWidget *parent)
    : EditorPopupWindow(parent, "SUBOBJECTS", "signalSubObjects", 300),
      sobj(NULL) {
    QVBoxLayout *vbox = popupLayout();
    setPopupPinToolTips(
        "Save the current Subobjects position between sessions.",
        "The Subobjects position is saved between sessions. Click to return to default placement.");

    addPopupSubtitle(QString::fromUtf8("• Signal Parts"));
    QFrame *partsCard = new QFrame;
    GuiFunct::styleEditorPanelCard(partsCard);
    QVBoxLayout *partsLayout = new QVBoxLayout(partsCard);
    partsLayout->setContentsMargins(4,3,4,3);
    partsLayout->setSpacing(2);
    QScrollArea *partsScroll = new QScrollArea;
    partsScroll->setWidgetResizable(true);
    partsScroll->setFrameShape(QFrame::NoFrame);
    partsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    partsScroll->setFixedHeight(qRound(210.0f * qBound(0.75f, Game::uiScale, 1.25f)));
    QWidget *partsContent = new QWidget;
    QVBoxLayout *partsRows = new QVBoxLayout(partsContent);
    partsRows->setContentsMargins(0,0,0,0);
    partsRows->setSpacing(2);

    for (int i = 0; i < maxSubObj; i++) {
        wSub[i] = new QWidget(partsContent);
        vSub[i] = new QGridLayout(wSub[i]);
        this->chSub[i].setText("");
        vSub[i]->setSpacing(2);
        vSub[i]->setContentsMargins(3, 0, 1, 0);
        dSub[i].setEnabled(false);
        dSub[i].setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        GuiFunct::styleEditorActionButton(&bSub[i]);
        vSub[i]->addWidget(&this->chSub[i], 0, 0);
        vSub[i]->addWidget(&this->bSub[i], 0, 2);
        vSub[i]->addWidget(&this->dSub[i], 0, 1);
        vSub[i]->setColumnStretch(1, 1);
        partsRows->addWidget(wSub[i]);
        wSub[i]->hide();

        signalsChSect.setMapping(&chSub[i], i);
        connect(&chSub[i], SIGNAL(clicked()), &signalsChSect, SLOT(map()));

        signalsLinkButton.setMapping(&bSub[i], i);
        connect(&bSub[i], SIGNAL(clicked()), &signalsLinkButton, SLOT(map()));

    }
    partsRows->addStretch(1);
    partsScroll->setWidget(partsContent);
    partsLayout->addWidget(partsScroll);
    vbox->addWidget(partsCard);

    connect(&signalsChSect, SIGNAL(mappedInt(int)), this, SLOT(chSubEnabled(int)));
    connect(&signalsLinkButton, SIGNAL(mappedInt(int)), this, SLOT(bLinkEnabled(int)));

    addPopupSubtitle(QString::fromUtf8("• Link"));
    QFrame *linkCard = new QFrame;
    GuiFunct::styleEditorPanelCard(linkCard);
    QVBoxLayout *linkLayout = new QVBoxLayout(linkCard);
    linkLayout->setContentsMargins(4,3,4,3);
    linkLayout->setSpacing(2);
    QLabel *label = new QLabel("Select a Link button above to view its junction.");
    label->setWordWrap(true);
    linkLayout->addWidget(label);
    QGridLayout *vlist = new QGridLayout;
    vlist->setSpacing(2);
    vlist->setContentsMargins(0,0,0,0);
    setLinkButton = new QPushButton("Set Link");
    GuiFunct::styleEditorActionButton(setLinkButton);
    connect(setLinkButton, SIGNAL(released()), this, SLOT(setLink()));
    connect(setLinkButton, &QPushButton::clicked,
            this, &SignalWindow::userButtonPressed);
    label = new QLabel("From:");
    vlist->addWidget(label,0,0);
    vlist->addWidget(&eLink1,0,1);
    eLink1.setDisabled(true);
    vlist->addWidget(new QLabel("To:"),0,2);
    vlist->addWidget(&eLink2,0,3);
    eLink2.setDisabled(true);
    vlist->addWidget(&eLink3,1,1,1,3);
    eLink3.setDisabled(true);
    vlist->addWidget(setLinkButton,2,0,1,4);
    linkLayout->addLayout(vlist);
    vbox->addWidget(linkCard);

    QFrame *actionCard = new QFrame;
    GuiFunct::styleEditorPanelCard(actionCard);
    QHBoxLayout *actions = new QHBoxLayout(actionCard);
    actions->setContentsMargins(4,3,4,3);
    actions->addStretch(1);
    QPushButton* closeButton = new QPushButton("Close");
    GuiFunct::styleEditorActionButton(closeButton);
    connect(closeButton, SIGNAL(released()), this, SLOT(close()));
    connect(closeButton, &QPushButton::clicked,
            this, &SignalWindow::userButtonPressed);
    actions->addWidget(closeButton);
    vbox->addWidget(actionCard);
    finalizePopup();
}

void SignalWindow::showForOwner(){
    showExclusive();
}

void SignalWindow::closeEvent(QCloseEvent *event){
    QWidget::closeEvent(event);
    emit helperClosed();
}

void SignalWindow::popupPinClicked(){
    emit userButtonPressed();
}

void SignalWindow::chSubEnabled(int i) {
    if (sobj == NULL)
        return;

    Undo::StateBegin();
    Undo::PushGameObjData(sobj);
    Undo::PushTrackDB(Game::trackDB);
    if (chSub[i].isChecked())
        sobj->enableSubObj(i);
    else
        sobj->disableSubObj(i);
    Undo::StateEnd();
    showObj(sobj);
}

void SignalWindow::bLinkEnabled(int i) {
    if (sobj == NULL)
        return;
    
    currentSubObjLinkInfo = i;
    if(currentSubObjLinkInfo < 0){
        setLinkButton->setDisabled(true);
        setLinkButton->setText("Set Link");
        eLink1.setText("");
        eLink2.setText("");
        eLink3.setText("");
        return;
    }
    this->sobj->subObjSelected = i;
    setLinkButton->setDisabled(false);
    setLinkButton->setText(QString("Set [Head ")+QString::number(i+1)+"]");
    setLinkButton->setStyleSheet(
        QString("border-style: outset; border-color: %1; border-width: 2px;")
        .arg(Game::StyleYellowButton));
    int ids[3];
    int linkId = sobj->getLinkedJunctionValue(i);
    if(linkId < 1){
        eLink1.setText("");
        eLink2.setText("");
        eLink3.setText("");
        return;
    }
    sobj->getLinkInfo((int*)&ids);
    eLink1.setText(QString::number(ids[0]));
    eLink2.setText(QString::number(ids[1]));
    eLink3.setText(QString::number(ids[2]));
    
    setLinkButton->setStyleSheet("");

    /*SignalWindowLink window;
    window.setWindowTitle("Link Signal");
    window.exec();
    if (window.changed) {
        int from = window.from.text().toInt();
        int to = window.to.text().toInt();
        qDebug() << "link val: " << from << " " << to;
        sobj->linkSignal(i, from, to);
        showObj(sobj);
    }*/

}

void SignalWindow::setLink(){
    if(currentSubObjLinkInfo >= 0)
        emit sendMsg("enableTool","signalLinkTool");
}

void SignalWindow::showObj(SignalObj* obj) {
    this->sobj = obj;
    for (int i = 0; i < maxSubObj; i++) {
        this->wSub[i]->hide();
        this->chSub[i].setChecked(false);
        this->bSub[i].hide();
        this->bSub[i].setEnabled(false);
    }

    TDB* tdb = Game::trackDB;
    SignalShape* signalShape = tdb->sigCfg->signalShape[sobj->fileName];

    int iSubObj = signalShape->iSubObj;
    if (iSubObj > maxSubObj) iSubObj = maxSubObj;
    QString backFace = "[B] ";
    for (int i = 0; i < iSubObj; i++) {
        this->wSub[i]->show();
        if(signalShape->subObj[i].backFacing)
            this->dSub[i].setText(backFace+signalShape->subObj[i].desc);
        else
            this->dSub[i].setText(signalShape->subObj[i].desc);
        this->dSub[i].setToolTip(this->dSub[i].text());
        if (sobj->isSubObjEnabled(i))
            this->chSub[i].setChecked(true);
        if (!signalShape->subObj[i].optional)
            this->chSub[i].setEnabled(false);
        else
            this->chSub[i].setEnabled(true);
    }
    int linkPtr;

    for (int i = 0; i < iSubObj; i++) {
        if (signalShape->subObj[i].isJnLink) {
            this->bSub[i].show();
            this->bSub[i].setStyleSheet("color: gray");
            this->bSub[i].setText("Link");
            this->bSub[i].setEnabled(false);
            if (this->chSub[i].isChecked()) {
                setLinkInfo(i);
            }
        } else if (signalShape->subObj[i].iLink > 0) {
            this->bSub[i].show();
            this->bSub[i].setStyleSheet("color: gray");
            this->bSub[i].setText("Link");
            this->bSub[i].setEnabled(false);
            for (int j = 0; j < signalShape->subObj[i].iLink; j++) {
                linkPtr = signalShape->subObj[i].sigSubJnLinkIf[j];
                if (this->chSub[linkPtr].isChecked()) {
                    setLinkInfo(i);
                    break;
                }
            }
        }
    }
    bLinkEnabled(-1);
}

void SignalWindow::updateObj(SignalObj* obj) {
    this->sobj = obj;
    if(sobj == NULL)
        return;
    
    TDB* tdb = Game::trackDB;
    SignalShape* signalShape = tdb->sigCfg->signalShape[sobj->fileName];
    int iSubObj = signalShape->iSubObj;
    if (iSubObj > maxSubObj) iSubObj = maxSubObj;
    int linkPtr;
    
    for (int i = 0; i < iSubObj; i++) {
        if(this->bSub[i].hasFocus())
            continue;
        if (signalShape->subObj[i].isJnLink) {
            this->bSub[i].show();
            this->bSub[i].setStyleSheet("color: gray");
            this->bSub[i].setText("Link");
            this->bSub[i].setEnabled(false);
            if (this->chSub[i].isChecked()) {
                setLinkInfo(i);
            }
        } else if (signalShape->subObj[i].iLink > 0) {
            this->bSub[i].show();
            this->bSub[i].setStyleSheet("color: gray");
            this->bSub[i].setText("Link");
            this->bSub[i].setEnabled(false);
            for (int j = 0; j < signalShape->subObj[i].iLink; j++) {
                linkPtr = signalShape->subObj[i].sigSubJnLinkIf[j];
                if (this->chSub[linkPtr].isChecked()) {
                    setLinkInfo(i);
                    break;
                }
            }
        }
    }
    bLinkEnabled(currentSubObjLinkInfo);
}

void SignalWindow::setLinkInfo(int i) {
    int linkId = sobj->getLinkedJunctionValue(i);
    if (linkId == -1) {
        this->bSub[i].setEnabled(false);
        this->bSub[i].setStyleSheet("color: red");
        this->bSub[i].setText("NULL");
    } else if (linkId == 0) {
        this->bSub[i].setStyleSheet("");
        if (sobj->isJunctionAvailable(i)) {
            this->bSub[i].setText("Link");
            this->bSub[i].setEnabled(true);
        } else {
            this->bSub[i].setText("No Junction");
            this->bSub[i].setEnabled(false);
        }
    } else {        
        this->bSub[i].setEnabled(true);
        this->bSub[i].setStyleSheet("border-style: outset;border-color:green;border-width: 2px;");
        this->bSub[i].setText("Linked: " + QString::number(linkId));
    }
}

SignalWindow::~SignalWindow() {
}

void SignalWindow::exitNow() {       
    this->close();
}
