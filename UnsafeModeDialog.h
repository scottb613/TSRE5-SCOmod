// UnsafeModeDialog.h
#ifndef UNSAFEMODEDIALOG_H
#define UNSAFEMODEDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>

class UnsafeModeDialog : public QDialog
{
    Q_OBJECT

public:
    UnsafeModeDialog(QWidget *parent = nullptr);
    ~UnsafeModeDialog();

    bool getCheckBox1Value() const { return unsafeOption1; }
    bool getCheckBox2Value() const { return unsafeOption2; }
    bool getCheckBox3Value() const { return unsafeOption3; }

private:
    QCheckBox *checkBox1;
    QCheckBox *checkBox2;
    QCheckBox *checkBox3;
    QLineEdit label1;
    QTextEdit text1;
    QPushButton *okButton;
    QPushButton *cancelButton;

    bool unsafeOption1;
    bool unsafeOption2;
    bool unsafeOption3;

private slots:

};

#endif // ROUTEMERGEDIALOG_H