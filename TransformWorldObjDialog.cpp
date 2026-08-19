/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#include "TransformWorldObjDialog.h"
#include "Game.h"
#include "GuiFunct.h"

TransformWorldObjDialog::TransformWorldObjDialog(QWidget *parent)
    : EditorPopupWindow(parent, "TRANSFORM", "transformWorldObject", 300) {
    const qreal scale = qBound(0.75f, Game::uiScale, 1.25f);
    const int cardMargin = qRound(6.0f * scale);
    const int controlSpacing = qRound(4.0f * scale);

    QPushButton* ok = new QPushButton("OK");
    QPushButton* cancel = new QPushButton("Cancel");
    GuiFunct::styleEditorActionButton(ok);
    GuiFunct::styleEditorActionButton(cancel);
    connect(ok, &QPushButton::clicked,
            this, [this](){ emit userButtonPressed(); });
    connect(cancel, &QPushButton::clicked,
            this, [this](){ emit userButtonPressed(); });
    connect(ok, SIGNAL (released()), this, SLOT (ok()));
    connect(cancel, SIGNAL (released()), this, SLOT (cancel()));

    QVBoxLayout *root = popupLayout();
    addPopupSubtitle(QString(QChar(0x2022)) + " Object Offset");

    QFrame *transformCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(transformCard);
    QGridLayout *vlist = new QGridLayout(transformCard);
    vlist->setHorizontalSpacing(controlSpacing);
    vlist->setVerticalSpacing(controlSpacing);
    vlist->setContentsMargins(cardMargin, cardMargin,
                              cardMargin, cardMargin);
    useObjRot.setChecked(true);
    useObjRot.setText("Use Object Rotation for translation");
    vlist->addWidget(&useObjRot, 0, 0, 1, 2);

    QLabel *translateTitle = new QLabel("TRANSLATE", transformCard);
    GuiFunct::styleEditorSubtitle(translateTitle);
    QLabel *rotateTitle = new QLabel("ROTATE", transformCard);
    GuiFunct::styleEditorSubtitle(rotateTitle);
    vlist->addWidget(translateTitle, 1, 0);
    vlist->addWidget(rotateTitle, 1, 1);

    QFormLayout *translateForm = new QFormLayout;
    translateForm->setContentsMargins(0, 0, 0, 0);
    translateForm->setSpacing(controlSpacing);
    translateForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    translateForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    translateForm->addRow("X:", &this->posX);
    this->posX.setText("0");
    translateForm->addRow("Y:", &this->posY);
    this->posY.setText("0");
    translateForm->addRow("Z:", &this->posZ);
    this->posZ.setText("0");

    QFormLayout *rotateForm = new QFormLayout;
    rotateForm->setContentsMargins(0, 0, 0, 0);
    rotateForm->setSpacing(controlSpacing);
    rotateForm->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rotateForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    rotateForm->addRow("X:", &this->rotX);
    this->rotX.setText("0");
    rotateForm->addRow("Y:", &this->rotY);
    this->rotY.setText("0");
    rotateForm->addRow("Z:", &this->rotZ);
    this->rotZ.setText("0");
    vlist->addLayout(translateForm, 2, 0);
    vlist->addLayout(rotateForm, 2, 1);
    root->addWidget(transformCard);

    QFrame *actionCard = new QFrame(this);
    GuiFunct::styleEditorPanelCard(actionCard);
    QHBoxLayout *buttons = new QHBoxLayout(actionCard);
    buttons->setContentsMargins(cardMargin, cardMargin,
                                cardMargin, cardMargin);
    buttons->setSpacing(controlSpacing);
    buttons->addStretch(1);
    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    root->addWidget(actionCard);
    finalizePopup();
}

TransformWorldObjDialog::~TransformWorldObjDialog() {
}

void TransformWorldObjDialog::showForOwner(){
    isOk = false;
    finishing = false;
    useObjRot.setChecked(true);
    posX.setText("0");
    posY.setText("0");
    posZ.setText("0");
    rotX.setText("0");
    rotY.setText("0");
    rotZ.setText("0");
    showExclusive();
}

void TransformWorldObjDialog::cancel(){
    this->isOk = false;
    finishing = true;
    this->close();
    emit finished();
}
void TransformWorldObjDialog::ok(){
    this->isOk = true;
    if(this->useObjRot.checkState() == Qt::Checked)
        this->isUseObjRot = true;
    else
        this->isUseObjRot = false;
    this->x = this->posX.text().toFloat();
    this->y = this->posY.text().toFloat();
    this->z = this->posZ.text().toFloat();
    this->rx = this->rotX.text().toFloat();
    this->ry = this->rotY.text().toFloat();
    this->rz = this->rotZ.text().toFloat();

    finishing = true;
    this->close();
    emit finished();
}

void TransformWorldObjDialog::closeEvent(QCloseEvent *event){
    if(!finishing){
        isOk = false;
        emit finished();
    }
    QWidget::closeEvent(event);
}

void TransformWorldObjDialog::popupPinClicked(){
    emit userButtonPressed();
}
