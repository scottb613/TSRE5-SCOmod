// TSRE GenX route paint editor. GNU GPL v3 or later.
#include "RoutePaintDialog.h"
#include "GuiFunct.h"
#include "Game.h"
#include "RoutePaintBatch.h"
#include <QtWidgets>
#include <QSaveFile>

RoutePaintDialog::RoutePaintDialog(const QJsonArray &available, QWidget *parent)
    : QDialog(parent),availablePresets(available) {
    setWindowTitle("Paint Entire Route");
    setWindowModality(Qt::ApplicationModal);
    GuiFunct::styleEditorDialog(this);
    auto *layout = new QVBoxLayout(this);
    const double scale=qBound(0.75,double(Game::uiScale),1.25);
    const int gap=qRound(8*scale),rowHeight=qRound(28*scale);
    layout->setContentsMargins(gap*2,gap,gap*2,gap*2);
    layout->setSpacing(gap);
    GuiFunct::addEditorDialogHeader(this, "Paint Entire Route");
    auto *help = new QLabel("Rows run in order. Unused rows and features absent from a tile are skipped.");
    help->setWordWrap(true); layout->addWidget(help);
    auto *sequence=new QFrame;
    GuiFunct::styleEditorPanelCard(sequence);
    auto *grid = new QGridLayout(sequence);
    grid->setContentsMargins(gap,gap,gap,gap);
    grid->setHorizontalSpacing(gap); grid->setVerticalSpacing(qRound(4*scale));
    grid->setColumnStretch(1,1);
    grid->setColumnMinimumWidth(1,qRound(230*scale));
    grid->setColumnMinimumWidth(2,qRound(110*scale));
    grid->addWidget(new QLabel("Step"),0,0); grid->addWidget(new QLabel("Preset"),0,1);
    grid->addWidget(new QLabel("Paint"),0,2);
    grid->addWidget(new QLabel("Mirror"),0,3,Qt::AlignHCenter);
    grid->addWidget(new QLabel("Main / Snow"),0,4);
    QFile recipe(Game::routeAppDataDir()+"/route-paint-sequence.json");
    RoutePaintPlan::Plan saved;
    if(recipe.open(QIODevice::ReadOnly)) saved = RoutePaintPlan::fromJson(QJsonDocument::fromJson(recipe.readAll()).object());
    for(int i=0;i<10;++i) {
        presets[i]=new QComboBox; presets[i]->addItem("Unused",QString());
        for(const auto &value:available) {
            const QString name=value.toObject()["name"].toString();
            if(!name.isEmpty()) presets[i]->addItem(name,name);
        }
        targets[i]=new QComboBox; targets[i]->addItems({"Unused","Track","Roads","Water"});
        mirrors[i]=new QCheckBox; mirrors[i]->setChecked(true);
        presets[i]->setMinimumHeight(rowHeight); targets[i]->setMinimumHeight(rowHeight);
        presets[i]->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
        pairStatus[i]=new QLabel;
        pairStatus[i]->setMinimumWidth(qRound(85*scale));
        if(i<saved.steps.size()) {
            const auto &step=saved.steps[i];
            int index=presets[i]->findData(step.preset);
            if(index<0) { presets[i]->addItem(step.preset+" (missing)",step.preset); index=presets[i]->count()-1; }
            presets[i]->setCurrentIndex(index);
            targets[i]->setCurrentIndex(int(step.target)>=0 && int(step.target)<=3 ? int(step.target) : 0);
            mirrors[i]->setChecked(step.mirror);
        }
        auto *number=new QLabel(QString::number(i+1));
        number->setFixedWidth(qRound(28*scale)); number->setAlignment(Qt::AlignCenter);
        number->setStyleSheet("color:#b6a187;");
        grid->addWidget(number,i+1,0); grid->addWidget(presets[i],i+1,1);
        grid->addWidget(targets[i],i+1,2); grid->addWidget(mirrors[i],i+1,3,Qt::AlignHCenter);
        grid->addWidget(pairStatus[i],i+1,4);
    }
    layout->addWidget(sequence);
    auto *saveHelp=new QLabel("Commit saves native-size uncompressed ACE without mipmaps and clears Undo. Use Process All TERRTEX for final output settings. Cancel keeps completed saves.");
    saveHelp->setWordWrap(true); saveHelp->setStyleSheet("color:#b8b8b8;"); layout->addWidget(saveHelp);
    auto *buttons=new QDialogButtonBox;
    commit=buttons->addButton("Commit",QDialogButtonBox::AcceptRole);
    GuiFunct::styleEditorActionButton(commit);
    commit->setAutoDefault(false);
    auto *cancel=buttons->addButton(QDialogButtonBox::Cancel);
    GuiFunct::styleEditorActionButton(cancel);
    layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
    connect(commit,&QPushButton::clicked,this,[this,available] {
        QString error; const auto sequence=plan();
        if(!RoutePaintPlan::validate(sequence,available,error)) { GuiFunct::showEditorStopped(this,"Paint Entire Route",error); return; }
        // Recheck checked pairs here as well as in the runner; files may change
        // after the per-row preview was validated.
        for(int i=0;i<sequence.steps.size();++i) {
            const auto &step=sequence.steps[i];
            if(step.target==RoutePaintPlan::Unused || !step.mirror) continue;
            for(const auto &value:available) if(value.toObject()["name"].toString()==step.preset)
                if(!RoutePaintBatch::validateMirrorPreset(value.toObject(),error)) {
                    GuiFunct::showEditorStopped(this,"Paint Entire Route",QString("Row %1: ").arg(i+1)+error); return;
                }
        }
        const QString path=Game::routeAppDataDir()+"/route-paint-sequence.json";
        QDir().mkpath(QFileInfo(path).absolutePath());
        QSaveFile file(path); const QByteArray bytes=QJsonDocument(RoutePaintPlan::toJson(sequence)).toJson();
        if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
            GuiFunct::showEditorStopped(this,"Paint Entire Route","Could not save the route paint sequence."); return;
        }
        accept();
    });
    for(int i=0;i<10;++i) {
        connect(presets[i],&QComboBox::activated,this,[this,i](int) {
            const QString name=presets[i]->currentData().toString();
            mirrorValid.remove(name); mirrorErrors.remove(name);
            for(int row=0;row<10;++row)
                if(presets[row]->currentData().toString()==name) refreshRow(row);
        });
        connect(targets[i],&QComboBox::currentIndexChanged,this,[this,i](int){refreshRow(i);});
        connect(mirrors[i],&QCheckBox::toggled,this,[this,i](bool checked) {
            if(checked) {
                const QString name=presets[i]->currentData().toString();
                mirrorValid.remove(name); mirrorErrors.remove(name);
                for(int row=0;row<10;++row)
                    if(presets[row]->currentData().toString()==name) refreshRow(row);
                return;
            }
            refreshRow(i);
        });
        refreshRow(i);
    }
    resize(qRound(700*scale),sizeHint().height());
}
RoutePaintPlan::Plan RoutePaintDialog::plan() const {
    RoutePaintPlan::Plan result;
    for(int i=0;i<10;++i) result.steps.append({presets[i]->currentData().toString(),
        RoutePaintPlan::Target(targets[i]->currentIndex()),mirrors[i]->isChecked()});
    return result;
}

void RoutePaintDialog::refreshRow(int row) {
    const QString name=presets[row]->currentData().toString();
    mirrors[row]->setEnabled(!name.isEmpty());
    mirrors[row]->setVisible(!name.isEmpty());
    const QString unusedStyle=name.isEmpty()?"QComboBox { color:#929292; }":QString();
    presets[row]->setStyleSheet(unusedStyle);
    targets[row]->setStyleSheet(unusedStyle);
    QString status,tip;
    QString color="#858585";
    if(!name.isEmpty() && !mirrors[row]->isChecked()) status="Main only";
    else if(!name.isEmpty()) {
        if(!mirrorValid.contains(name)) {
            QString error="Preset is missing.";
            bool valid=false;
            for(const auto &value:availablePresets) if(value.toObject()["name"].toString()==name) {
                valid=RoutePaintBatch::validateMirrorPreset(value.toObject(),error); break;
            }
            mirrorValid.insert(name,valid); mirrorErrors.insert(name,error);
        }
        const bool valid=mirrorValid.value(name);
        status=valid?"Ready":"Unavailable";
        color=valid?"#8bc58b":"#e3a16f";
        tip=valid?"Main/Snow pair validated.":mirrorErrors.value(name)+"\nChoose a valid pair or uncheck Mirror.";
    }
    pairStatus[row]->setText(status); pairStatus[row]->setStyleSheet("color:"+color+';');
    pairStatus[row]->setToolTip(tip); mirrors[row]->setToolTip(tip);
    refreshCommit();
}

void RoutePaintDialog::refreshCommit() {
    bool ready=true;
    for(int i=0;i<10;++i) {
        const QString name=presets[i]->currentData().toString();
        if(!name.isEmpty() && mirrors[i]->isChecked() && !mirrorValid.value(name,true)) ready=false;
    }
    commit->setEnabled(ready);
    commit->setToolTip(ready?QString():"Fix unavailable Main/Snow pairs or uncheck Mirror for those rows.");
}
