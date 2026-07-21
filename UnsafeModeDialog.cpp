/*
 */

#include "UnsafeModeDialog.h"
#include "Game.h"
#include "GuiFunct.h"
#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QDebug>

UnsafeModeDialog::UnsafeModeDialog(QWidget *parent) : QDialog(parent)
{
    GuiFunct::applyEditorPanelStyle(this);
    setWindowTitle("Route Merge Options");
    this->setFixedSize(500, 350);
    
    // Create checkboxes
    checkBox1 = new QCheckBox("Enable Unsafe Mode");
    checkBox2 = new QCheckBox("Rebuild Databases (WARNING: This will remove all interactives).");
    checkBox3 = new QCheckBox("Use Optional Real Number Precision");
    ;            

    text1.setText("Performing a database rebuild will DELETE all signals, speedposts/mileposts, platform and siding markers, car spawners, level crossings, and other interactives. They CANNOT be recovered after taking this action, and will need to be manually re-added to your route.\n\nDO NOT ATTEMPT if you do not fully understand the risks involved. This is a measure of last resort to save a route that has extreme corruption.\n\nENSURE YOU HAVE A FULL BACKUP prior to proceeding.");
    
    // Create buttons
    okButton = new QPushButton("Proceed with Unsafe Options");
    cancelButton = new QPushButton("Revert to Safe Mode");

    // Connect button signals to slots
    connect(okButton, &QPushButton::clicked, this, &UnsafeModeDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &UnsafeModeDialog::reject);
   
    QStringList args = Game::routeMergeString.split(":");    
    QString unsafeRouteName = args[0];
    unsafeRouteName = unsafeRouteName.replace("\"","");
    unsafeRouteName = unsafeRouteName.replace(";","");
    label1.setText("Unsafe Options for " + Game::route);
    
    if(Game::UnsafeMode) checkBox1->setChecked(true);
    if(Game::routeRebuildTDB) checkBox2->setChecked(true);
    if(Game::rnp != 7) checkBox3->setChecked(true);    
    if(Game::rnp != 7) qDebug() << "RNP:" << Game::rnp;
    checkBox1->setEnabled(false);
    checkBox2->setEnabled(false);
    checkBox3->setEnabled(false);
    label1.setEnabled(false);
    
    //checkBox1->setStyleSheet(QString("QCheckBox { color : ")+Game::StyleMainLabel+"; }");
    //checkBox2->setStyleSheet(QString("QCheckBox { color : ")+Game::StyleMainLabel+"; }");
    //checkBox3->setStyleSheet(QString("QCheckBox { color : ")+Game::StyleMainLabel+"; }");
    label1.setStyleSheet(QString("QLabel { color : ")+Game::StyleMainLabel+"; font-size:12pt; }");
    text1.setStyleSheet(QString("QTextEdit { color : ")+Game::StyleMainLabel+"; font-size:12pt;}");
    
    // Layout checkboxes vertically
    QVBoxLayout *checkBoxLayout = new QVBoxLayout;
    checkBoxLayout->addWidget(&label1);
    // checkBoxLayout->addWidget(checkBox1);
    checkBoxLayout->addWidget(checkBox2);
    checkBoxLayout->addWidget(&text1);
    checkBoxLayout->addWidget(checkBox3);

    // Layout buttons horizontally
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(cancelButton); // Cancel usually on the left
    buttonLayout->addWidget(okButton);

    // Main layout (vertical)
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(checkBoxLayout);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

UnsafeModeDialog::~UnsafeModeDialog()
{
    delete checkBox1;
    delete checkBox2;
    delete checkBox3;
    delete okButton;
    delete cancelButton;
}




