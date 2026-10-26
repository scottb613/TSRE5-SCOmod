// TSRE GenX compact editor disclosure card. GPL v3 or later.
#ifndef EDITORCOLLAPSIBLESECTION_H
#define EDITORCOLLAPSIBLESECTION_H

#include "Game.h"
#include "GuiFunct.h"
#include <QFrame>
#include <QPushButton>
#include <QVBoxLayout>

class EditorCollapsibleSection : public QFrame {
public:
    EditorCollapsibleSection(const QString &title, QWidget *body, QWidget *parent = nullptr)
        : QFrame(parent), content(body), titleText(title) {
        GuiFunct::styleEditorPanelCard(this);
        const int padding = qRound(4*qBound(0.75f, Game::uiScale, 1.25f));
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(padding, padding, padding, padding);
        layout->setSpacing(padding);
        toggle = new QPushButton(QString(QChar(0x25B6)) + ' ' + title, this);
        toggle->setCheckable(true);
        toggle->setToolTip("Show or hide controls. Collapsing preserves their settings.");
        GuiFunct::styleEditorActionButton(toggle);
        layout->addWidget(toggle);
        layout->addWidget(body);
        body->hide();
        QObject::connect(toggle, &QPushButton::toggled, body, [button = toggle, body, title](bool expanded) {
            button->setText(QString(QChar(expanded ? 0x25BC : 0x25B6)) + ' ' + title);
            body->setVisible(expanded);
        });
    }
    QPushButton *toggleButton() const { return toggle; }
    void setExpanded(bool expanded) {
        toggle->setChecked(expanded);
        // Also refresh after a caller restores the button under QSignalBlocker.
        toggle->setText(QString(QChar(expanded ? 0x25BC : 0x25B6)) + ' ' + titleText);
        content->setVisible(expanded);
    }
private:
    QPushButton *toggle = nullptr;
    QWidget *content = nullptr;
    QString titleText;
};
#endif
