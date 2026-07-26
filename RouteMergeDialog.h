// RouteMergeDialog.h
#ifndef ROUTEMERGEDIALOG_H
#define ROUTEMERGEDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>

class RouteMergeDialog : public QDialog
{
    Q_OBJECT

public:
    RouteMergeDialog(QWidget *parent = nullptr);
    ~RouteMergeDialog();

    bool getCheckBox1Value() const { return mergeOption1; }
    bool getCheckBox2Value() const { return mergeOption2; }
    bool getCheckBox3Value() const { return mergeOption3; }

private:
    QCheckBox *checkBox1;
    QCheckBox *checkBox2;
    QCheckBox *checkBox3;
    QLineEdit label1;
    QPushButton *okButton;
    QPushButton *cancelButton;

    bool mergeOption1;
    bool mergeOption2;
    bool mergeOption3;

private slots:

};

#endif // ROUTEMERGEDIALOG_H