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

namespace {
class WindowPinIconEngine : public QIconEngine {
public:
    QIconEngine *clone() const override {
        return new WindowPinIconEngine(*this);
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode,
                   QIcon::State state) override {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, result.rect(), mode, state);
        return result;
    }

    void paint(QPainter *painter, const QRect &rect,
               QIcon::Mode mode, QIcon::State state) override {
        if(painter == NULL || rect.isEmpty())
            return;
        QColor color = state == QIcon::On ? QColor(35, 35, 35)
                                          : QColor(231, 234, 236);
        if(mode == QIcon::Disabled)
            color = QColor(133, 133, 133);

        const qreal side = qMin(rect.width(), rect.height());
        const QPointF center = rect.center();
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(color, qMax<qreal>(1.2, side * 0.16),
                             Qt::SolidLine, Qt::RoundCap));
        painter->drawEllipse(center, side * 0.34, side * 0.34);
        painter->setPen(Qt::NoPen);
        painter->setBrush(color);
        painter->drawEllipse(center, side * 0.105, side * 0.105);
        painter->restore();
    }
};
}

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
        " width: 13px; height: 13px; background-color: #202020; border: 1px solid #9a9a9a;"
        "}"
        "QRadioButton::indicator:hover { border-color: #f08200; }"
        "QRadioButton::indicator:checked { background-color: #f08200; border-color: #ffad3b; }"
        "QSlider::groove:horizontal { height: 4px; background: #1d1d1d; border: 1px solid #474747; border-radius: 2px; }"
        "QSlider::sub-page:horizontal { background: #b76512; border-radius: 2px; }"
        "QSlider::handle:horizontal { width: 10px; margin: -4px 0; background: #747474; border: 1px solid #989898; border-radius: 2px; }"
        "QSlider::handle:horizontal:hover { background: #8a8a8a; border-color: #f08200; }"
        "QToolTip { color: white; background-color: #252525; border: 1px solid #777777; padding: 3px; }"
    );
}

QString GuiFunct::scoEditorPanelStyle(){
    return scoPanelStyle() + QString(
        "QLineEdit, QSpinBox, QDoubleSpinBox, QTimeEdit, QDateEdit, QDateTimeEdit,"
        " QTextEdit, QPlainTextEdit, QComboBox {"
        " background-color: #202020; color: #f2f2f2; font-weight: normal;"
        " border: 1px solid #555555; border-top-color: #151515; border-radius: 1px; padding: 1px 3px;"
        "}"
        "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QTimeEdit:disabled,"
        " QDateEdit:disabled, QDateTimeEdit:disabled, QTextEdit:disabled, QPlainTextEdit:disabled,"
        " QComboBox:disabled { background-color: #202020; color: #f2f2f2; }"
        "QLineEdit:enabled:!read-only, QTextEdit:enabled:!read-only, QPlainTextEdit:enabled:!read-only,"
        " QSpinBox:enabled:!read-only, QDoubleSpinBox:enabled:!read-only, QTimeEdit:enabled:!read-only,"
        " QDateEdit:enabled:!read-only, QDateTimeEdit:enabled:!read-only {"
        " border: 1px solid #70590e;"
        "}"
        "QLineEdit:enabled:!read-only:hover, QTextEdit:enabled:!read-only:hover,"
        " QPlainTextEdit:enabled:!read-only:hover, QSpinBox:enabled:!read-only:hover,"
        " QDoubleSpinBox:enabled:!read-only:hover, QTimeEdit:enabled:!read-only:hover,"
        " QDateEdit:enabled:!read-only:hover, QDateTimeEdit:enabled:!read-only:hover {"
        " border: 1px solid #f08200;"
        "}"
        "QLineEdit:enabled:!read-only:focus, QTextEdit:enabled:!read-only:focus,"
        " QPlainTextEdit:enabled:!read-only:focus, QSpinBox:enabled:!read-only:focus,"
        " QDoubleSpinBox:enabled:!read-only:focus, QTimeEdit:enabled:!read-only:focus,"
        " QDateEdit:enabled:!read-only:focus, QDateTimeEdit:enabled:!read-only:focus {"
        " border: 1px solid #8a7116;"
        "}"
        "QLineEdit:enabled:!read-only:focus:hover, QTextEdit:enabled:!read-only:focus:hover,"
        " QPlainTextEdit:enabled:!read-only:focus:hover, QSpinBox:enabled:!read-only:focus:hover,"
        " QDoubleSpinBox:enabled:!read-only:focus:hover, QTimeEdit:enabled:!read-only:focus:hover,"
        " QDateEdit:enabled:!read-only:focus:hover, QDateTimeEdit:enabled:!read-only:focus:hover {"
        " border: 1px solid #f08200;"
        "}"
        "QLineEdit:read-only, QTextEdit:read-only, QPlainTextEdit:read-only, QSpinBox:read-only,"
        " QDoubleSpinBox:read-only, QTimeEdit:read-only, QDateEdit:read-only, QDateTimeEdit:read-only {"
        " border: 1px solid #555555; border-top-color: #151515;"
        "}"
        "QSpinBox, QDoubleSpinBox, QTimeEdit, QDateEdit, QDateTimeEdit { padding-right: 18px; }"
        "QSpinBox::up-button, QSpinBox::down-button, QDoubleSpinBox::up-button, QDoubleSpinBox::down-button,"
        " QTimeEdit::up-button, QTimeEdit::down-button, QDateEdit::up-button, QDateEdit::down-button,"
        " QDateTimeEdit::up-button, QDateTimeEdit::down-button {"
        " width: 16px; background-color: #414141; border-left: 1px solid #555555;"
        "}"
        "QSpinBox::up-button:hover, QSpinBox::down-button:hover,"
        " QDoubleSpinBox::up-button:hover, QDoubleSpinBox::down-button:hover,"
        " QTimeEdit::up-button:hover, QTimeEdit::down-button:hover,"
        " QDateEdit::up-button:hover, QDateEdit::down-button:hover,"
        " QDateTimeEdit::up-button:hover, QDateTimeEdit::down-button:hover { background-color: #555555; }"
        "QSpinBox::up-arrow, QSpinBox::down-arrow, QDoubleSpinBox::up-arrow, QDoubleSpinBox::down-arrow,"
        " QTimeEdit::up-arrow, QTimeEdit::down-arrow, QDateEdit::up-arrow, QDateEdit::down-arrow,"
        " QDateTimeEdit::up-arrow, QDateTimeEdit::down-arrow { width: 7px; height: 7px; }"
    );
}

QString GuiFunct::editorTitleStyle(){
    return QString(
        "QLabel { color: %1; font-weight: bold;"
        " background-color: #292929; border: none;"
        " border-left: 3px solid %1;"
        " padding: 6px 8px; }").arg(Game::StyleMainLabel);
}

QString GuiFunct::editorSubtitleStyle(){
    return QString(
        "QLabel { color: %1; font-weight: bold;"
        " background-color: #2b2b2b; border: none;"
        " padding: 2px 6px; }").arg(Game::StyleMainLabel);
}

void GuiFunct::styleEditorTitle(QLabel *label){
    if(label == NULL)
        return;
    label->setStyleSheet(editorTitleStyle());
    label->setContentsMargins(0,0,0,0);
}

void GuiFunct::styleEditorSubtitle(QLabel *label){
    if(label == NULL)
        return;
    label->setStyleSheet(editorSubtitleStyle());
    label->setContentsMargins(qRound(6.0f * qMax(1.0f, Game::uiScale)),0,0,0);
}

void GuiFunct::applyEditorPanelStyle(QWidget *panel){
    if(panel == NULL)
        return;
    panel->setStyleSheet(scoEditorPanelStyle());
    QTimer::singleShot(0, panel, [panel](){
        QFont fieldFont = panel->font();
        fieldFont.setBold(false);
        const int fieldHeight = qRound(22.0f * qMax(1.0f, Game::uiScale));
        const int rowSpacing = qRound(3.0f * qMax(1.0f, Game::uiScale));

        foreach(QLineEdit *field, panel->findChildren<QLineEdit*>()){
            field->setFont(fieldFont);
            field->setMinimumHeight(fieldHeight);
        }
        foreach(QSpinBox *field, panel->findChildren<QSpinBox*>()){
            field->setFont(fieldFont);
            field->setMinimumHeight(fieldHeight);
        }
        foreach(QDoubleSpinBox *field, panel->findChildren<QDoubleSpinBox*>()){
            field->setFont(fieldFont);
            field->setMinimumHeight(fieldHeight);
        }
        foreach(QComboBox *field, panel->findChildren<QComboBox*>()){
            field->setFont(fieldFont);
            field->setMinimumHeight(fieldHeight);
        }
        foreach(QTextEdit *field, panel->findChildren<QTextEdit*>())
            field->setFont(fieldFont);
        foreach(QPlainTextEdit *field, panel->findChildren<QPlainTextEdit*>())
            field->setFont(fieldFont);
        foreach(QLabel *label, panel->findChildren<QLabel*>()){
            if(label->text().trimmed().startsWith(QChar(0x2022)))
                styleEditorSubtitle(label);
        }
        foreach(QFormLayout *layout, panel->findChildren<QFormLayout*>())
            layout->setVerticalSpacing(rowSpacing);
        foreach(QGridLayout *layout, panel->findChildren<QGridLayout*>())
            layout->setVerticalSpacing(rowSpacing);
    });
}

void GuiFunct::setupWindowPinButton(QToolButton *button){
    if(button == NULL)
        return;
    button->setText(QString());
    button->setIcon(QIcon(new WindowPinIconEngine));
    const int iconSide = qRound(15.0f * qMax(1.0f, Game::uiScale));
    button->setIconSize(QSize(iconSide, iconSide));
}
