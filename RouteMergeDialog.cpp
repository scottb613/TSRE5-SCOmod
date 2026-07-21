#include "routemergedialog.h"
#include "Game.h"
#include "GuiFunct.h"
#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>

RouteMergeDialog::RouteMergeDialog(QWidget *parent) : QDialog(parent)
{
    GuiFunct::applyEditorPanelStyle(this);
    setWindowTitle("Route Merge Options");
    this->setFixedSize(500, 200);
    
    // Create checkboxes
    checkBox1 = new QCheckBox("Merge TDB");
    checkBox2 = new QCheckBox("Merge Terrain");
    checkBox3 = new QCheckBox("Merge Terrain Textures");
            
    
    // Create buttons
    okButton = new QPushButton("Proceed with Merge");
    cancelButton = new QPushButton("Cancel Merge");

    // Connect button signals to slots
    connect(okButton, &QPushButton::clicked, this, &RouteMergeDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &RouteMergeDialog::reject);
   
    QStringList args = Game::routeMergeString.split(":");    
    QString mergeRouteName = args[0];
    mergeRouteName = mergeRouteName.replace("\"","");
    mergeRouteName = mergeRouteName.replace(";","");
    label1.setText("Merging " + mergeRouteName + " into " + Game::route);
    
    if(Game::routeMergeTDB) checkBox1->setChecked(true);
    if(Game::routeMergeTerrain) checkBox2->setChecked(true);
    if(Game::routeMergeTerrtex) checkBox3->setChecked(true);

    checkBox1->setEnabled(false);
    checkBox2->setEnabled(false);
    checkBox3->setEnabled(false);
    label1.setEnabled(false);
    
    // Layout checkboxes vertically
    QVBoxLayout *checkBoxLayout = new QVBoxLayout;
    checkBoxLayout->addWidget(&label1);
    checkBoxLayout->addWidget(checkBox1);
    checkBoxLayout->addWidget(checkBox2);
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

RouteMergeDialog::~RouteMergeDialog()
{
    delete checkBox1;
    delete checkBox2;
    delete checkBox3;
    delete okButton;
    delete cancelButton;
}




