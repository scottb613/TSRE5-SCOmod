/*  This file is part of TSRE5.
 *
 *  TSRE5 - train sim game engine and MSTS/OR Editors. 
 *  Copyright (C) 2016 Piotr Gadecki <pgadecki@gmail.com>
 *
 *  Licensed under GNU General Public License 3.0 or later. 
 *
 *  See LICENSE.md or https://www.gnu.org/licenses/gpl.html
 */

#ifndef GUIFUNCTIONS_H
#define	GUIFUNCTIONS_H

#include <QPoint>
#include <QString>
#include <QTimer>
#include <QWidget>

class QLabel;
class QLineEdit;
class QAction;
class QAbstractButton;
class QToolButton;
class QVBoxLayout;
class QMoveEvent;
class QDialog;

class GuiFunct {
public:
    static QLabel* newQLabel(QString text, int width);
    static QLabel* newTQLabel(QString text, int width = -1);
    static QLineEdit* newQLineEdit(int width, int length);
    static QAction* newMenuCheckAction(QString desc, QWidget* window, bool checked = true);
    static QString scoPanelStyle();
    static QString scoEditorPanelStyle();
    static QString editorTitleStyle();
    static QString editorSubtitleStyle();
    static void styleEditorTitle(QLabel *label);
    static void styleEditorSubtitle(QLabel *label);
    static void applyEditorPanelStyle(QWidget *panel);
    static void setEditorToolWindowTitle(QWidget *window);
    static void styleEditorDialog(QDialog *dialog);
    static void addEditorDialogHeader(QDialog *dialog, const QString &title,
                                      const QString &subtitle = QString());
    static void showEditorNotice(QWidget *parent, const QString &heading,
                                 const QString &message);
    static void showEditorStopped(QWidget *parent, const QString &heading,
                                  const QString &message);
    static void installImportantDialogCentering();
    static void setupWindowPinButton(QToolButton *button);
    static QPoint snappedWindowPosition(QWidget *window, int snapDistance = 10);
    static void setEditorPopupButtonActive(QAbstractButton *button, bool active);
    static bool confirmDestructiveAction(QWidget *parent, const QString &heading,
                                         const QString &message);
private:

};

class EditorPopupWindow : public QWidget {
public:
    EditorPopupWindow(QWidget *owner, const QString &title,
                      const QString &pinnedPositionKey, int baseWidth = 300);

    QVBoxLayout *popupLayout() const;
    QLabel *addPopupSubtitle(const QString &subtitle);
    void finalizePopup();
    bool isPopupPositionPinned() const;
    void setPopupPinToolTips(const QString &unpinnedToolTip,
                            const QString &pinnedToolTip);

protected:
    void moveEvent(QMoveEvent *event) override;
    virtual void popupPinChanged(bool pinned);
    virtual void popupPinClicked();

private:
    void updatePinAppearance();

    QString positionKey;
    QVBoxLayout *rootLayout = NULL;
    QToolButton *pinButton = NULL;
    QTimer snapTimer;
    QTimer pinSaveTimer;
    bool positionPinned = false;
    bool snapping = false;
    bool positionRestored = false;
    QString unpinnedPinToolTip = "Save this helper position between sessions.";
    QString pinnedPinToolTip = "This helper position is saved between sessions.";
};

#endif	/* GUIFUNCTIONS_H */

