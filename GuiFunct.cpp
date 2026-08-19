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

#ifdef Q_OS_WIN
#include <cstring>
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {
void applyGenXWindowsCaption(QWidget *window);

void playGenXDialogSound(){
    if(!Game::scoSoundEnabled)
        return;
#ifdef Q_OS_WIN
    const QString soundPath = QCoreApplication::applicationDirPath()
        + "/content/SCOpluck.wav";
    if(QFile::exists(soundPath)){
        ::PlaySoundW(NULL, NULL, 0);
        ::PlaySoundW(reinterpret_cast<const wchar_t*>(soundPath.utf16()), NULL,
                     SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    }
#endif
}

void replaceNativeMessageBoxIcon(QMessageBox *messageBox){
    if(messageBox == NULL
    || messageBox->property("scoNativeMessageSoundSuppressed").toBool())
        return;

    messageBox->setProperty("scoNativeMessageSoundSuppressed", true);
    QStyle::StandardPixmap standardPixmap;
    switch(messageBox->icon()){
        case QMessageBox::Information:
            standardPixmap = QStyle::SP_MessageBoxInformation;
            break;
        case QMessageBox::Warning:
            standardPixmap = QStyle::SP_MessageBoxWarning;
            break;
        case QMessageBox::Critical:
            standardPixmap = QStyle::SP_MessageBoxCritical;
            break;
        case QMessageBox::Question:
            standardPixmap = QStyle::SP_MessageBoxQuestion;
            break;
        case QMessageBox::NoIcon:
        default:
            return;
    }

    const int iconSize = messageBox->style()->pixelMetric(
        QStyle::PM_MessageBoxIconSize, NULL, messageBox);
    const QPixmap pixmap = messageBox->style()->standardIcon(
        standardPixmap, NULL, messageBox).pixmap(iconSize, iconSize);
    if(!pixmap.isNull())
        // A custom pixmap retains the expected visible icon while changing
        // QMessageBox::icon() to NoIcon, preventing Qt/Windows from selecting
        // a native information, warning, critical, or question sound.
        messageBox->setIconPixmap(pixmap);
}

class ImportantDialogCenteringFilter : public QObject {
public:
    explicit ImportantDialogCenteringFilter(QObject *parent)
        : QObject(parent) {
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        QLineEdit *input = qobject_cast<QLineEdit*>(watched);
        if(input != NULL && input->isEnabled() && !input->isReadOnly()){
            bool selectForEntry = false;
            if(event->type() == QEvent::MouseButtonPress){
                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
                selectForEntry = mouseEvent->button() == Qt::LeftButton;
            } else if(event->type() == QEvent::FocusIn){
                QFocusEvent *focusEvent = static_cast<QFocusEvent*>(event);
                selectForEntry = focusEvent->reason() == Qt::TabFocusReason
                                 || focusEvent->reason() == Qt::BacktabFocusReason
                                 || focusEvent->reason() == Qt::ShortcutFocusReason;
            }
            if(selectForEntry){
                QPointer<QLineEdit> guardedInput(input);
                QTimer::singleShot(0, input, [guardedInput](){
                    if(!guardedInput.isNull())
                        guardedInput->selectAll();
                });
            }
        }

        QWidget *dialog = qobject_cast<QWidget*>(watched);
        QMessageBox *messageBox = qobject_cast<QMessageBox*>(dialog);
        if(messageBox != NULL
        && (event->type() == QEvent::Polish
            || event->type() == QEvent::Show))
            replaceNativeMessageBoxIcon(messageBox);

        if(dialog == NULL || event->type() != QEvent::Show)
            return QObject::eventFilter(watched, event);

        if(messageBox != NULL
        && !messageBox->property("scoDialogSoundPlayed").toBool()){
            messageBox->setProperty("scoDialogSoundPlayed", true);
            playGenXDialogSound();
        }

        // Qt implements combo-box lists and menus as temporary top-level
        // popup widgets.  Calling winId() on those while they are being shown
        // forces native-window creation before Qt has finished positioning
        // them, which can detach the popup from its control.  Apply the DWM
        // caption treatment only to genuine application windows.
        const Qt::WindowType windowType = static_cast<Qt::WindowType>(
            (dialog->windowFlags() & Qt::WindowType_Mask).toInt());
        const bool isTransientPopup =
            windowType == Qt::Popup
            || windowType == Qt::ToolTip
            || windowType == Qt::SplashScreen;

        if(dialog->isWindow() && !isTransientPopup
        && !dialog->windowTitle().trimmed().isEmpty()
        && !(dialog->windowFlags() & Qt::FramelessWindowHint)){
            QPointer<QWidget> guardedWindow(dialog);
            QTimer::singleShot(0, dialog, [guardedWindow](){
                if(!guardedWindow.isNull())
                    applyGenXWindowsCaption(guardedWindow.data());
            });
        }

        const bool importantMessage =
            qobject_cast<QMessageBox*>(dialog) != NULL
            || dialog->property("scoCenterOnScreen").toBool();
        if(!importantMessage)
            return QObject::eventFilter(watched, event);

        QPointer<QWidget> guardedDialog(dialog);
        QTimer::singleShot(0, dialog, [guardedDialog](){
            if(guardedDialog.isNull())
                return;

            QWidget *window = guardedDialog.data();
            QPoint screenPoint = QCursor::pos();
            QWidget *parent = window->parentWidget();
            if(parent != NULL)
                screenPoint = parent->window()->frameGeometry().center();

            QScreen *screen = QGuiApplication::screenAt(screenPoint);
            if(screen == NULL)
                screen = QGuiApplication::primaryScreen();
            if(screen == NULL)
                return;

            const QRect available = screen->availableGeometry();
            const QSize frameSize = window->frameGeometry().size();
            window->move(
                available.left() + (available.width() - frameSize.width()) / 2,
                available.top() + (available.height() - frameSize.height()) / 2);
        });
        return QObject::eventFilter(watched, event);
    }
};

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

#ifdef Q_OS_WIN
void applyGenXWindowsCaption(QWidget *window){
    if(window == NULL)
        return;

    // Never call QWidget::winId() here.  Doing so can force an ordinary child
    // widget (and its ancestors) to become native while the UI hierarchy is
    // still being assembled.  Caption styling is optional until Qt has
    // created a genuine top-level QWindow during Show.
    QWindow *nativeWindow = window->windowHandle();
    if(nativeWindow == NULL)
        return;

    typedef HRESULT (WINAPI *DwmSetWindowAttributeFunction)(
        HWND, DWORD, LPCVOID, DWORD);
    static HMODULE dwmLibrary = LoadLibraryW(L"dwmapi.dll");
    if(dwmLibrary == NULL)
        return;
    static DwmSetWindowAttributeFunction setAttribute = [](){
        DwmSetWindowAttributeFunction function = NULL;
        FARPROC resolvedFunction =
            GetProcAddress(dwmLibrary, "DwmSetWindowAttribute");
        static_assert(sizeof(function) == sizeof(resolvedFunction),
                      "Windows function pointer sizes must match");
        std::memcpy(&function, &resolvedFunction, sizeof(function));
        return function;
    }();
    if(setAttribute == NULL)
        return;

    const HWND handle = reinterpret_cast<HWND>(nativeWindow->winId());
    const BOOL darkMode = TRUE;
    const COLORREF captionColor = RGB(38, 38, 38);
    const COLORREF captionText = RGB(196, 164, 128);
    const COLORREF borderColor = RGB(91, 74, 58);

    // Windows 10 1809 used attribute 19; newer Windows uses 20.
    // Unsupported attributes are intentionally ignored.
    const DWORD immersiveDarkModeBefore20H1 = 19;
    const DWORD immersiveDarkMode = 20;
    const DWORD nativeBorderColor = 34;
    const DWORD nativeCaptionColor = 35;
    const DWORD nativeTextColor = 36;
    if(FAILED(setAttribute(handle, immersiveDarkMode, &darkMode,
                           sizeof(darkMode)))){
        setAttribute(handle, immersiveDarkModeBefore20H1, &darkMode,
                     sizeof(darkMode));
    }
    setAttribute(handle, nativeCaptionColor, &captionColor,
                 sizeof(captionColor));
    setAttribute(handle, nativeTextColor, &captionText,
                 sizeof(captionText));
    setAttribute(handle, nativeBorderColor, &borderColor,
                 sizeof(borderColor));
}
#else
void applyGenXWindowsCaption(QWidget *){
}
#endif
}

void GuiFunct::installImportantDialogCentering(){
    if(qApp == NULL
    || qApp->property("scoImportantDialogCenteringInstalled").toBool())
        return;

    ImportantDialogCenteringFilter *filter =
        new ImportantDialogCenteringFilter(qApp);
    qApp->installEventFilter(filter);
    qApp->setProperty("scoImportantDialogCenteringInstalled", true);
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
        " color: %7; background: #343434;"
        " border-color: %7; border-bottom-color: %7;"
        "}"
        "QPushButton:checked:hover {"
        " color: %8; background: #3a3a3a;"
        " border-color: %8; border-bottom-color: %8;"
        "}"
        "QPushButton:disabled { color: #858585; background: #3b3b3b; border-color: #4b4b4b; }"
        "QPushButton[dialogRole=\"primary\"] { border-left: 3px solid %1; }"
        "QPushButton[dialogRole=\"primary\"]:hover { border-left-color: %2; }"
        "QPushButton[dialogRole=\"positive\"] { border-left: 3px solid %3; }"
        "QPushButton[dialogRole=\"positive\"]:hover { border-left-color: %4; }"
        "QPushButton[dialogRole=\"danger\"] { border-left: 3px solid %5; }"
        "QPushButton[dialogRole=\"danger\"]:hover { border-left-color: %6; }"
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
    ).arg(Game::StyleOrangeButton, Game::StyleOrangeButtonHover,
          Game::StyleGreenButton, Game::StyleGreenButtonHover,
          Game::StyleRedButton, Game::StyleRedButtonHover,
          Game::StyleOrangeButton, Game::StyleOrangeButtonHover);
}

QString GuiFunct::scoEditorPanelStyle(){
    const qreal panelScale = qBound(0.75f, Game::uiScale, 1.25f);
    const int cardRadius = qRound(3.0 * panelScale);
    const int buttonVerticalPadding = qRound(5.0 * panelScale);
    const int buttonHorizontalPadding = qRound(8.0 * panelScale);
    return scoPanelStyle() + QString(
        "QFrame[editorPanelCard=\"true\"] { background-color: #252525;"
        " border: 1px solid #454545; border-radius: %2px; }"
        "QPushButton[editorPanelAction=\"true\"] { color: #e2e2e2;"
        " background-color: #343434; border: 1px solid #5b5b5b;"
        " border-radius: 2px; padding: %3px %4px; font-weight: 600; }"
        "QPushButton[editorPanelAction=\"true\"]:hover { color: %1;"
        " background-color: #3a3a3a; border-color: %1; }"
        "QPushButton[editorPanelAction=\"true\"]:pressed { color: %1;"
        " background-color: #202020; border-color: %1; }"
        "QPushButton[editorPanelAction=\"true\"]:checked { color: %1;"
        " background-color: #343434; border-color: %1; }"
        "QPushButton[editorPanelAction=\"true\"]:checked:hover { color: %1;"
        " background-color: #3a3a3a; border-color: %1; }"
        "QPushButton[editorPanelAction=\"true\"]:disabled { color: #777777;"
        " background-color: #2b2b2b; border-color: #3f3f3f; }"
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
    ).arg(Game::StyleMainLabel)
     .arg(cardRadius)
     .arg(buttonVerticalPadding)
     .arg(buttonHorizontalPadding);
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
    label->setContentsMargins(qRound(6.0f * qBound(0.75f, Game::uiScale, 1.25f)),0,0,0);
}

void GuiFunct::styleEditorPanelCard(QFrame *card){
    if(card == NULL)
        return;
    card->setProperty("editorPanelCard", true);
}

void GuiFunct::styleEditorActionButton(QPushButton *button){
    if(button == NULL)
        return;
    button->setProperty("editorPanelAction", true);
    button->setMinimumHeight(qRound(28.0f * qBound(0.75f, Game::uiScale, 1.25f)));
}

void GuiFunct::alignEditorForm(QFormLayout *form, int baseLabelWidth){
    if(form == NULL)
        return;
    const int labelWidth = qRound(
        baseLabelWidth * qBound(0.75f, Game::uiScale, 1.25f));
    form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setRowWrapPolicy(QFormLayout::DontWrapRows);
    for(int row = 0; row < form->rowCount(); row++){
        QLayoutItem *labelItem = form->itemAt(row, QFormLayout::LabelRole);
        if(labelItem != NULL && labelItem->widget() != NULL)
            labelItem->widget()->setMinimumWidth(labelWidth);
    }
}

void GuiFunct::applyEditorPanelStyle(QWidget *panel){
    if(panel == NULL)
        return;
    panel->setStyleSheet(scoEditorPanelStyle());
    QTimer::singleShot(0, panel, [panel](){
        QFont fieldFont = panel->font();
        fieldFont.setBold(false);
        const int fieldHeight = qRound(22.0f * qBound(0.75f, Game::uiScale, 1.25f));
        const int rowSpacing = qRound(3.0f * qBound(0.75f, Game::uiScale, 1.25f));

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

void GuiFunct::setEditorToolWindowTitle(QWidget *window){
    if(window == NULL)
        return;
    window->setWindowTitle(Game::AppName);
    window->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    applyGenXWindowsCaption(window);
}

void GuiFunct::styleEditorDialog(QDialog *dialog){
    if(dialog == NULL)
        return;

    applyEditorPanelStyle(dialog);
    setEditorToolWindowTitle(dialog);
    QTimer::singleShot(0, dialog, [dialog](){
        foreach(QDialogButtonBox *box,
                dialog->findChildren<QDialogButtonBox*>()){
            foreach(QAbstractButton *button, box->buttons()){
                const QDialogButtonBox::ButtonRole role =
                    box->buttonRole(button);
                if(role == QDialogButtonBox::AcceptRole
                || role == QDialogButtonBox::ApplyRole)
                    button->setProperty("dialogRole", "primary");
                else if(role == QDialogButtonBox::YesRole)
                    button->setProperty("dialogRole", "positive");
                else if(role == QDialogButtonBox::NoRole
                     || role == QDialogButtonBox::DestructiveRole)
                    button->setProperty("dialogRole", "danger");
                button->style()->unpolish(button);
                button->style()->polish(button);
            }
        }
    });
}

void GuiFunct::addEditorDialogHeader(QDialog *dialog, const QString &title,
                                      const QString &subtitle){
    if(dialog == NULL || dialog->layout() == NULL)
        return;

    QLabel *titleLabel = new QLabel(title.toUpper(), dialog);
    styleEditorTitle(titleLabel);
    titleLabel->setSizePolicy(QSizePolicy::Expanding,
                              QSizePolicy::Preferred);

    QLabel *subtitleLabel = NULL;
    if(!subtitle.trimmed().isEmpty()){
        QString subtitleText = subtitle.trimmed();
        if(!subtitleText.startsWith(QChar(0x2022)))
            subtitleText.prepend(QString(QChar(0x2022)) + " ");
        subtitleLabel = new QLabel(subtitleText.toUpper(), dialog);
        styleEditorSubtitle(subtitleLabel);
        subtitleLabel->setSizePolicy(QSizePolicy::Expanding,
                                     QSizePolicy::Preferred);
    }

    if(QBoxLayout *box = qobject_cast<QBoxLayout*>(dialog->layout())){
        box->insertWidget(0, titleLabel);
        if(subtitleLabel != NULL)
            box->insertWidget(1, subtitleLabel);
        return;
    }

    QGridLayout *grid = qobject_cast<QGridLayout*>(dialog->layout());
    if(grid == NULL){
        titleLabel->deleteLater();
        if(subtitleLabel != NULL)
            subtitleLabel->deleteLater();
        return;
    }

    struct GridEntry {
        QLayoutItem *item;
        int row;
        int column;
        int rowSpan;
        int columnSpan;
        Qt::Alignment alignment;
    };
    QVector<GridEntry> entries;
    while(grid->count() > 0){
        int row = 0;
        int column = 0;
        int rowSpan = 1;
        int columnSpan = 1;
        grid->getItemPosition(0, &row, &column, &rowSpan, &columnSpan);
        QLayoutItem *item = grid->takeAt(0);
        entries.append({item, row, column, rowSpan, columnSpan,
                        item == NULL ? Qt::Alignment() : item->alignment()});
    }

    const int headerRows = subtitleLabel == NULL ? 1 : 2;
    const int columns = qMax(1, grid->columnCount());
    grid->addWidget(titleLabel, 0, 0, 1, columns);
    if(subtitleLabel != NULL)
        grid->addWidget(subtitleLabel, 1, 0, 1, columns);
    for(const GridEntry &entry : entries){
        if(entry.item != NULL)
            grid->addItem(entry.item, entry.row + headerRows,
                          entry.column, entry.rowSpan, entry.columnSpan,
                          entry.alignment);
    }
}

void GuiFunct::showEditorNotice(QWidget *parent, const QString &heading,
                                 const QString &message){
    QMessageBox notice(parent);
    const int standardWidth =
        qRound(420.0f * qBound(0.75f, Game::uiScale, 1.25f));
    notice.setIcon(QMessageBox::Information);
    notice.setWindowTitle(Game::AppName);
    notice.setText(heading.toUpper());
    notice.setInformativeText(message);
    notice.setStandardButtons(QMessageBox::Ok);
    notice.setStyleSheet(
        QString("QLabel#qt_msgbox_label {"
                " color: %1; font-weight: bold;"
                " background-color: #292929; border: none;"
                " border-left: 3px solid %1;"
                " padding: 6px 8px;"
                "}").arg(Game::StyleMainLabel));
    applyGenXWindowsCaption(&notice);

    QTimer::singleShot(0, &notice, [&notice, parent, standardWidth](){
        const int textWidth = qMax(
            qRound(260.0f * qBound(0.75f, Game::uiScale, 1.25f)),
            standardWidth - qRound(100.0f * qBound(0.75f, Game::uiScale, 1.25f)));
        const char *labelNames[2] = {
            "qt_msgbox_label", "qt_msgbox_informativelabel"
        };
        for(int i = 0; i < 2; ++i){
            QLabel *label = notice.findChild<QLabel*>(labelNames[i]);
            if(label == NULL)
                continue;
            label->setMinimumWidth(textWidth);
            label->setMaximumWidth(textWidth);
            label->setWordWrap(true);
        }
        notice.setMinimumWidth(standardWidth);
        notice.setMaximumWidth(standardWidth);
        notice.adjustSize();
        notice.resize(standardWidth, notice.height());
        QPoint point = parent == NULL
            ? QCursor::pos() : parent->window()->frameGeometry().center();
        QScreen *screen = QGuiApplication::screenAt(point);
        if(screen == NULL)
            screen = QGuiApplication::primaryScreen();
        if(screen == NULL)
            return;
        const QRect available = screen->availableGeometry();
        notice.move(available.left()
                        + (available.width() - notice.frameGeometry().width()) / 2,
                    available.top()
                        + (available.height() - notice.frameGeometry().height()) / 2);
    });

    notice.exec();
}

void GuiFunct::showEditorStopped(QWidget *parent, const QString &heading,
                                 const QString &message){
    QMessageBox stopped(parent);
    const int standardWidth =
        qRound(420.0f * qBound(0.75f, Game::uiScale, 1.25f));
    stopped.setIcon(QMessageBox::Warning);
    stopped.setWindowTitle(Game::AppName);
    stopped.setText(heading.toUpper());
    stopped.setInformativeText(message);
    stopped.setStandardButtons(QMessageBox::Ok);
    styleEditorDialog(&stopped);
    stopped.setStyleSheet(stopped.styleSheet() + QString(
        "QLabel#qt_msgbox_label {"
        " color: %1; font-weight: bold;"
        " background-color: #292929; border: none;"
        " border-left: 3px solid %1;"
        " padding: 6px 8px;"
        "}").arg(Game::StyleOrangeButton));

    QTimer::singleShot(0, &stopped, [&stopped, parent, standardWidth](){
        const int textWidth = qMax(
            qRound(260.0f * qBound(0.75f, Game::uiScale, 1.25f)),
            standardWidth - qRound(100.0f * qBound(0.75f, Game::uiScale, 1.25f)));
        const char *labelNames[2] = {
            "qt_msgbox_label", "qt_msgbox_informativelabel"
        };
        for(int i = 0; i < 2; ++i){
            QLabel *label = stopped.findChild<QLabel*>(labelNames[i]);
            if(label == NULL)
                continue;
            label->setMinimumWidth(textWidth);
            label->setMaximumWidth(textWidth);
            label->setWordWrap(true);
        }
        stopped.setMinimumWidth(standardWidth);
        stopped.setMaximumWidth(standardWidth);
        stopped.adjustSize();
        stopped.resize(standardWidth, stopped.height());
        QPoint point = parent == NULL
            ? QCursor::pos() : parent->window()->frameGeometry().center();
        QScreen *screen = QGuiApplication::screenAt(point);
        if(screen == NULL)
            screen = QGuiApplication::primaryScreen();
        if(screen == NULL)
            return;
        const QRect available = screen->availableGeometry();
        stopped.move(available.left()
                        + (available.width() - stopped.frameGeometry().width()) / 2,
                     available.top()
                        + (available.height() - stopped.frameGeometry().height()) / 2);
    });

    stopped.exec();
}

void GuiFunct::setupWindowPinButton(QToolButton *button){
    if(button == NULL)
        return;
    button->setText(QString());
    button->setIcon(QIcon(new WindowPinIconEngine));
    const int iconSide = qRound(15.0f * qBound(0.75f, Game::uiScale, 1.25f));
    button->setIconSize(QSize(iconSide, iconSide));
}

QPoint GuiFunct::snappedWindowPosition(QWidget *window, int snapDistance){
    if(window == NULL)
        return QPoint();

    QRect moving = window->frameGeometry();
    QPoint snappedFramePos = moving.topLeft();
    int bestX = snapDistance + 1;
    int bestY = snapDistance + 1;

    QScreen *screen = QGuiApplication::screenAt(moving.center());
    if(screen != NULL){
        const QRect available = screen->availableGeometry();
        const int xCandidates[2] = {
            available.left() - moving.left(),
            available.right() - moving.right()
        };
        const int yCandidates[2] = {
            available.top() - moving.top(),
            available.bottom() - moving.bottom()
        };
        for(int i = 0; i < 2; ++i){
            if(qAbs(xCandidates[i]) <= snapDistance &&
                    qAbs(xCandidates[i]) < bestX){
                bestX = qAbs(xCandidates[i]);
                snappedFramePos.setX(moving.left() + xCandidates[i]);
            }
            if(qAbs(yCandidates[i]) <= snapDistance &&
                    qAbs(yCandidates[i]) < bestY){
                bestY = qAbs(yCandidates[i]);
                snappedFramePos.setY(moving.top() + yCandidates[i]);
            }
        }
    }

    for(QWidget *targetWidget : QApplication::topLevelWidgets()){
        if(targetWidget == window || !targetWidget->isVisible())
            continue;
        const QRect target = targetWidget->frameGeometry();
        const bool verticalNear =
            moving.bottom() >= target.top() - snapDistance &&
            moving.top() <= target.bottom() + snapDistance;
        const bool horizontalNear =
            moving.right() >= target.left() - snapDistance &&
            moving.left() <= target.right() + snapDistance;
        const int xCandidates[4] = {
            target.left() - moving.left(),
            target.right() - moving.right(),
            target.right() + 1 - moving.left(),
            target.left() - 1 - moving.right()
        };
        const int yCandidates[4] = {
            target.top() - moving.top(),
            target.bottom() - moving.bottom(),
            target.bottom() + 1 - moving.top(),
            target.top() - 1 - moving.bottom()
        };
        for(int i = 0; i < 4; ++i){
            int distance = qAbs(xCandidates[i]);
            if(verticalNear && distance <= snapDistance && distance < bestX){
                bestX = distance;
                snappedFramePos.setX(moving.left() + xCandidates[i]);
            }
            distance = qAbs(yCandidates[i]);
            if(horizontalNear && distance <= snapDistance && distance < bestY){
                bestY = distance;
                snappedFramePos.setY(moving.top() + yCandidates[i]);
            }
        }
    }

    return window->pos() + (snappedFramePos - moving.topLeft());
}

void GuiFunct::setEditorPopupButtonActive(QAbstractButton *button, bool active){
    static QPointer<QAbstractButton> activePopupButton;
    if(button == NULL)
        return;

    if(!active){
        if(activePopupButton == button)
            activePopupButton.clear();
        return;
    }

    if(!activePopupButton.isNull()
    && activePopupButton != button
    && activePopupButton->isChecked()){
        // Reuse the established toggle-off handler. setChecked() does not emit
        // clicked(), so the automatic close remains silent.
        activePopupButton->setChecked(false);
    }
    activePopupButton = button;
}

bool GuiFunct::confirmDestructiveAction(QWidget *parent, const QString &heading,
                                        const QString &message){
    QMessageBox warning(parent);
    const int standardWidth =
        qRound(420.0f * qBound(0.75f, Game::uiScale, 1.25f));
    warning.setIcon(QMessageBox::Warning);
    warning.setWindowTitle(Game::AppName);
    warning.setText(heading.toUpper());
    warning.setInformativeText(message);
    warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    warning.setDefaultButton(QMessageBox::No);
    warning.setEscapeButton(QMessageBox::No);
    styleEditorDialog(&warning);
    warning.setStyleSheet(warning.styleSheet() + QString(
        "QLabel#qt_msgbox_label {"
        " color: %1; font-weight: bold;"
        " background-color: #292929; border: none;"
        " border-left: 3px solid %1;"
        " padding: 6px 8px;"
        "}").arg(Game::StyleYellowButton));
    QLabel *warningTitle = warning.findChild<QLabel*>("qt_msgbox_label");
    if(warningTitle != NULL)
        warningTitle->setSizePolicy(
            QSizePolicy::Expanding, QSizePolicy::Preferred);

    QTimer::singleShot(0, &warning, [&warning, parent, standardWidth](){
        const int textWidth = qMax(
            qRound(260.0f * qBound(0.75f, Game::uiScale, 1.25f)),
            standardWidth - qRound(100.0f * qBound(0.75f, Game::uiScale, 1.25f)));
        QLabel *titleLabel =
            warning.findChild<QLabel*>("qt_msgbox_label");
        QLabel *messageLabel =
            warning.findChild<QLabel*>("qt_msgbox_informativelabel");
        if(titleLabel != NULL){
            titleLabel->setMinimumWidth(textWidth);
            titleLabel->setMaximumWidth(textWidth);
            titleLabel->setWordWrap(true);
        }
        if(messageLabel != NULL){
            messageLabel->setMinimumWidth(textWidth);
            messageLabel->setMaximumWidth(textWidth);
            messageLabel->setWordWrap(true);
        }
        warning.setMinimumWidth(standardWidth);
        warning.setMaximumWidth(standardWidth);
        if(warning.layout() != NULL){
            warning.layout()->invalidate();
            warning.layout()->activate();
        }
        warning.adjustSize();
        warning.resize(standardWidth, warning.height());
        QPoint screenPoint = QCursor::pos();
        if(parent != NULL)
            screenPoint = parent->window()->frameGeometry().center();
        QScreen *screen = QGuiApplication::screenAt(screenPoint);
        if(screen == NULL)
            screen = QGuiApplication::primaryScreen();
        if(screen == NULL)
            return;
        const QRect available = screen->availableGeometry();
        const QPoint centered(
            available.left() + (available.width() - warning.frameGeometry().width()) / 2,
            available.top() + (available.height() - warning.frameGeometry().height()) / 2);
        warning.move(centered);
    });

    return warning.exec() == QMessageBox::Yes;
}

QPointer<EditorPopupWindow> EditorPopupWindow::activePopup;

EditorPopupWindow::EditorPopupWindow(QWidget *owner, const QString &title,
                                     const QString &pinnedPositionKey, int baseWidth)
    : QWidget(owner == NULL ? NULL : owner->window(), Qt::Tool),
      positionKey(pinnedPositionKey) {
    GuiFunct::applyEditorPanelStyle(this);
    GuiFunct::setEditorToolWindowTitle(this);
    setFixedWidth(qRound(baseWidth * qBound(0.75f, Game::uiScale, 1.25f)));

    rootLayout = new QVBoxLayout(this);
    rootLayout->setSpacing(4);
    rootLayout->setContentsMargins(4,4,4,4);

    QLabel *titleLabel = new QLabel(title);
    GuiFunct::styleEditorTitle(titleLabel);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0,0,0,0);
    titleRow->addWidget(titleLabel);
    titleRow->addStretch();

    pinButton = new QToolButton;
    GuiFunct::setupWindowPinButton(pinButton);
    pinButton->setCheckable(true);
    pinButton->setFocusPolicy(Qt::NoFocus);
    pinButton->setFixedSize(
        qRound(30.0f * qBound(0.75f, Game::uiScale, 1.25f)),
        qRound(17.0f * qBound(0.75f, Game::uiScale, 1.25f)));
    positionPinned = Game::pinnedWindowPosition(this->positionKey, NULL);
    pinButton->setChecked(positionPinned);
    updatePinAppearance();
    titleRow->addWidget(pinButton);
    rootLayout->addLayout(titleRow);

    snapTimer.setSingleShot(true);
    QObject::connect(&snapTimer, &QTimer::timeout, this, [this](){
        if(snapping)
            return;
        QPoint snappedPosition = GuiFunct::snappedWindowPosition(this);
        if(snappedPosition == pos())
            return;
        snapping = true;
        move(snappedPosition);
        snapping = false;
    });

    pinSaveTimer.setSingleShot(true);
    QObject::connect(&pinSaveTimer, &QTimer::timeout, this, [this](){
        if(positionPinned)
            Game::savePinnedWindowPosition(this->positionKey, pos());
    });

    QObject::connect(pinButton, &QToolButton::toggled, this, [this](bool pinned){
        positionPinned = pinned;
        updatePinAppearance();
        if(!pinned)
            pinSaveTimer.stop();
        popupPinChanged(pinned);
    });
    QObject::connect(pinButton, &QToolButton::clicked, this, [this](){
        popupPinClicked();
    });
}

void EditorPopupWindow::showExclusive(){
    if(!activePopup.isNull() && activePopup != this)
        activePopup->close();
    activePopup = this;
    show();
    raise();
    activateWindow();
}

void EditorPopupWindow::showEvent(QShowEvent *event){
    if(!activePopup.isNull() && activePopup != this)
        activePopup->close();
    activePopup = this;
    QWidget::showEvent(event);
}

void EditorPopupWindow::closeActiveUnlessSupportedBy(QWidget *panel){
    if(activePopup.isNull() || !activePopup->isVisible())
        return;
    if(panel != NULL){
        const QList<QAbstractButton*> launchers =
            panel->findChildren<QAbstractButton*>();
        for(QAbstractButton *launcher : launchers){
            if(launcher->property("editorPopupKey").toString()
                    == activePopup->positionKey)
                return;
        }
    }
    activePopup->close();
}

QVBoxLayout *EditorPopupWindow::popupLayout() const {
    return rootLayout;
}

QLabel *EditorPopupWindow::addPopupSubtitle(const QString &subtitle){
    QLabel *label = new QLabel(subtitle);
    GuiFunct::styleEditorSubtitle(label);
    rootLayout->addWidget(label);
    return label;
}

void EditorPopupWindow::finalizePopup(){
    rootLayout->activate();
    setFixedHeight(rootLayout->sizeHint().height());
    if(positionRestored)
        return;
    positionRestored = true;
    QPoint pinnedPosition;
    if(Game::pinnedWindowPosition(positionKey, &pinnedPosition))
        move(Game::visibleWindowPosition(pinnedPosition, size()));
}

bool EditorPopupWindow::isPopupPositionPinned() const {
    return positionPinned;
}

void EditorPopupWindow::setPopupPinToolTips(const QString &unpinnedToolTip,
                                            const QString &pinnedToolTip){
    unpinnedPinToolTip = unpinnedToolTip;
    pinnedPinToolTip = pinnedToolTip;
    updatePinAppearance();
}

void EditorPopupWindow::moveEvent(QMoveEvent *event){
    QWidget::moveEvent(event);
    if(snapping)
        return;
    snapTimer.start(120);
    if(positionPinned)
        pinSaveTimer.start(240);
}

void EditorPopupWindow::popupPinChanged(bool pinned){
    if(pinned)
        Game::savePinnedWindowPosition(positionKey, pos());
    else
        Game::clearPinnedWindowPosition(positionKey);
}

void EditorPopupWindow::popupPinClicked(){
}

void EditorPopupWindow::updatePinAppearance(){
    GuiFunct::setupWindowPinButton(pinButton);
    pinButton->setToolTip(positionPinned ? pinnedPinToolTip : unpinnedPinToolTip);
    if(positionPinned){
        pinButton->setStyleSheet(QString(
            "QToolButton { color: #232323; background-color: %1;"
            " border: 1px solid %1; padding: 0px 3px; font-weight: normal; }"
            "QToolButton:hover { border-color: #e4c5a3; }"
            "QToolButton:pressed { background-color: #a98a69; }")
            .arg(Game::StyleMainLabel));
    } else {
        pinButton->setStyleSheet(
            "QToolButton { color: #e7eaec; background-color: #26292c;"
            " border: 1px solid #383d41; padding: 0px 3px; font-weight: normal; }"
            "QToolButton:hover { background-color: #303438; border-color: #f08200; }"
            "QToolButton:pressed { background-color: #191b1d; }");
    }
}
