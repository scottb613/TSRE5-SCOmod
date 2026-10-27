// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "StaticCopyDialog.h"
#include "StaticCopy.h"
#include "StaticObj.h"
#include "SFile.h"
#include "Route.h"
#include "Ref.h"
#include "Game.h"
#include "GuiFunct.h"
#include <QComboBox>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSet>
#include <QSettings>
#include <cmath>

namespace StaticCopyUi {
namespace {
void title(QVBoxLayout *layout,const QString &text) {
    auto *label=new QLabel(text); GuiFunct::styleEditorTitle(label); layout->addWidget(label);
}
QString settings(const Ref::RefItem *ref,const StaticObj *object,QString &error) {
    QString out;
    if(ref) {
        if(!ref->align.isEmpty()) out+="\tAlign ( "+StaticCopy::quote(ref->align)+" )\r\n";
        if(!ref->selectionMethod.isEmpty()) out+="\tSelectionMethod ( "+StaticCopy::quote(ref->selectionMethod)+" )\r\n";
        if(ref->randomTransformation) {
            const auto &r=*ref->randomTransformation;
            struct Range { QString field; float first,last; };
            const QVector<Range> ranges{{"RandomRotX",r.rbX,r.reX},{"RandomRotY",r.rbY,r.reY},{"RandomRotZ",r.rbZ,r.reZ},
                {"RandomTranslationX",r.tbX,r.teX},{"RandomTranslationY",r.tbY,r.teY},{"RandomTranslationZ",r.tbZ,r.teZ}};
            for(const auto &range:ranges) {
                if(!std::isfinite(range.first) || !std::isfinite(range.last)) { error="Source REF contains non-finite placement settings."; return {}; }
                out+="\t"+range.field+" ( "+QString::number(range.first,'g',9)+' '+QString::number(range.last,'g',9)+" )\r\n";
            }
        }
    }
    out+="\tTSREStaticFlags ( "+StaticCopy::quote(QString::number(object->staticFlags))+" )\r\n";
    out+="\tTSREStaticDetailLevel ( "+StaticCopy::quote(QString::number(object->staticDetailLevel))+" )\r\n";
    out+="\tTSRECollideFlags ( "+StaticCopy::quote(QString::number(object->collideFlags))+" )\r\n";
    out+="\tTSRECollideFunction ( "+StaticCopy::quote(QString::number(object->collideFunction))+" )\r\n";
    return out;
}
QString stoppedMessage(const StaticCopy::Result &result,bool workingFolder) {
    return (result.cancelled?QString("Transfer cancelled.\n"):QString())+result.errors.join('\n')
        +(workingFolder?"\nObject folder: "+QDir::toNativeSeparators(result.outputFolder)+"\n":QString("\nDestination REF was not changed.\n"))
        +(result.copied.isEmpty()?QString("Newly copied resources were rolled back."):"Retained partial resources:\n"+result.copied.join('\n'));
}
}

void restoreDestination(QString &destination,bool &workingFolder) {
    QSettings saved(Game::appDataDir()+"/copy-to-options.ini",QSettings::IniFormat);
    workingFolder=saved.value("workingFolderMode",false).toBool();
    destination=saved.value("enabled",false).toBool()?saved.value(workingFolder?"workingFolder":"routeFolder").toString():QString();
}

void chooseDestination(QWidget *parent,const QString &sourceRoute,QString &destination,bool &workingFolder) {
    QSettings saved(Game::appDataDir()+"/copy-to-options.ini",QSettings::IniFormat);
    QString routePath=saved.value("routeFolder").toString(), workingPath=saved.value("workingFolder").toString();
    if(!destination.isEmpty()) (workingFolder?workingPath:routePath)=destination;
    QDialog dialog(parent); dialog.setWindowTitle("Copy To — Destination"); GuiFunct::styleEditorDialog(&dialog);
    auto *layout=new QVBoxLayout(&dialog); title(layout,"COPY TO — DESTINATION");
    auto *mode=new QCheckBox("Choose Working Folder (no REF entry)"); mode->setChecked(workingFolder); layout->addWidget(mode);
    auto *help=new QLabel;
    help->setWordWrap(true); layout->addWidget(help);
    auto *path=new QLineEdit(workingFolder?workingPath:routePath); path->setReadOnly(true); layout->addWidget(path);
    auto *browse=new QPushButton("Choose Route Folder..."); GuiFunct::styleEditorActionButton(browse); layout->addWidget(browse);
    auto *status=new QLabel; status->setTextFormat(Qt::PlainText); status->setWordWrap(true); layout->addWidget(status);
    auto *buttons=new QDialogButtonBox; auto *set=buttons->addButton("Use Destination",QDialogButtonBox::AcceptRole);
    auto *clear=buttons->addButton("Clear Destination",QDialogButtonBox::ResetRole);
    buttons->addButton(QDialogButtonBox::Cancel); layout->addWidget(buttons);
    GuiFunct::styleEditorActionButton(set); GuiFunct::styleEditorActionButton(clear);
    const auto validate=[&](bool probe){
        const auto checked=mode->isChecked()?StaticCopy::validateWorkingFolder(path->text(),probe):StaticCopy::validateDestination(path->text(),probe);
        const QString source=QFileInfo(sourceRoute).canonicalFilePath();
        const bool same=!checked.root.isEmpty() && (checked.root.compare(source,Qt::CaseInsensitive)==0
            || checked.root.startsWith(source+'/',Qt::CaseInsensitive) || source.startsWith(checked.root+'/',Qt::CaseInsensitive));
        const bool valid=checked.valid && !same;
        set->setEnabled(valid);
        status->setText(valid?(mode->isChecked()?QString("Valid working folder. Each object gets a new subfolder; no REF is written."):
            "Valid route: "+checked.name+"\nREF: "+QDir::toNativeSeparators(checked.refPath)):
            (same?QString("Choose a separate folder from the active source route."):checked.errors.join('\n')));
        return valid;
    };
    bool shownWorking=workingFolder;
    const auto updateMode=[&](){
        browse->setText(mode->isChecked()?"Choose Working Folder...":"Choose Route Folder...");
        help->setText(mode->isChecked()?"Copy object resources into a new folder for editing. No REF name or category is requested.":
            "Choose the receiving route for Static Objects. Its descriptor, REF and write access are validated.");
    };
    QObject::connect(mode,&QCheckBox::toggled,&dialog,[&](bool working){
        (shownWorking?workingPath:routePath)=path->text(); shownWorking=working;
        path->setText(working?workingPath:routePath); updateMode(); validate(false);
    });
    QObject::connect(browse,&QPushButton::clicked,&dialog,[&](){
        const QString folder=QFileDialog::getExistingDirectory(&dialog,mode->isChecked()?"Select Working Folder":"Select Destination Route Folder",path->text());
        if(!folder.isEmpty()) { path->setText(folder); validate(true); }
    });
    const auto persist=[&](bool enabled){
        (mode->isChecked()?workingPath:routePath)=path->text();
        saved.setValue("routeFolder",routePath); saved.setValue("workingFolder",workingPath);
        saved.setValue("workingFolderMode",mode->isChecked()); saved.setValue("enabled",enabled); saved.sync();
        if(saved.status()!=QSettings::NoError) GuiFunct::showEditorStopped(&dialog,"Copy To Settings","This selection works for this session, but could not be saved for the next session.");
    };
    QObject::connect(set,&QPushButton::clicked,&dialog,[&](){ if(validate(true)) {
        destination=QFileInfo(path->text()).canonicalFilePath(); workingFolder=mode->isChecked(); path->setText(destination);
        persist(true); dialog.accept();
    } });
    QObject::connect(clear,&QPushButton::clicked,&dialog,[&](){ destination.clear(); workingFolder=mode->isChecked(); persist(false); dialog.accept(); });
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    updateMode(); validate(false); dialog.resize(650,290); dialog.exec();
}

void copyObject(QWidget *parent,StaticObj *object,Route *route,const QString &destination,bool workingFolder) {
    if(!object || !route || !Game::writeEnabled || Game::serverClient || object->polyVegRaw) return;
    const QString shapeRoot=object->resPath.isEmpty()?Game::root+"/routes/"+Game::route+"/SHAPES":object->resPath;
    const QString selected=QFileInfo(QDir(shapeRoot).filePath(object->fileName)).canonicalFilePath();
    if(selected.isEmpty()) { GuiFunct::showEditorStopped(parent,"Copy To","The model file is missing. Save or bake the object's resources before copying."); return; }
    const auto matches=[&](const Ref::RefItem *item){
        if(!item || (item->type!="static" && item->type!="collideobject" && item->type!="gantry")) return false;
        for(QString name:item->filename) {
            name.replace('\\','/');
            if(QFileInfo(name).fileName().compare(QFileInfo(selected).fileName(),Qt::CaseInsensitive)!=0) continue;
            if(QFileInfo(QDir(shapeRoot).filePath(name)).canonicalFilePath().compare(selected,Qt::CaseInsensitive)==0) return true;
        }
        return false;
    };
    const Ref::RefItem *definition=nullptr; QString error;
    if(!workingFolder && route->ref && matches(route->ref->selected)) definition=route->ref->selected;
    else if(!workingFolder && route->ref) {
        QSet<QString> definitions;
        for(auto group=route->ref->refItems.cbegin();group!=route->ref->refItems.cend();++group) for(const auto &item:group.value()) if(matches(&item)) {
            QStringList models; for(const QString &name:item.filename) models<<name.toCaseFolded();
            definitions.insert(settings(&item,object,error)+models.join('|'));
            if(!definition) definition=&item;
        }
        if(definitions.size()>1) { GuiFunct::showEditorStopped(parent,"Copy To","Several REF definitions match this model with different placement settings. Select its catalog entry first, then use Copy To again."); return; }
    }
    StaticCopy::Source source; source.routeRoot=Game::root+"/routes/"+Game::route;
    source.shapeRoot=shapeRoot; source.textureRoot=object->shapePointer && !object->shapePointer->baseTexPath.isEmpty()?object->shapePointer->baseTexPath:source.routeRoot+"/TEXTURES";
    source.type=object->type;
    if(definition) for(const QString &name:definition->filename) source.shapes<<name;
    else source.shapes<<object->fileName;
    if(!workingFolder) source.settings=settings(definition,object,error);
    if(!error.isEmpty()) { GuiFunct::showEditorStopped(parent,"Copy To",error); return; }
    const QString suggestedName=QFileInfo(selected).completeBaseName();
    const QString suggestedCategory=definition && !definition->clas.isEmpty()?definition->clas:QString("Static Objects");
    StaticCopy::Plan plan;
    const auto target=workingFolder?StaticCopy::validateWorkingFolder(destination,true):StaticCopy::validateDestination(destination,true);
    if(!target.valid) plan.errors=target.errors;
    else plan=StaticCopy::prepare(source,target);
    if(plan.cancelled) return;
    if(!plan.valid) { GuiFunct::showEditorStopped(parent,"Copy To",plan.errors.join('\n')); return; }
    if(workingFolder) {
        const auto result=StaticCopy::transfer(plan,{},{});
        if(result.completed) GuiFunct::showEditorNotice(parent,"Copy To Complete",QString("Copied %1 resources (%2 MiB) to:\n%3")
            .arg(result.copied.size()).arg(double(result.bytes)/(1024*1024),0,'f',2).arg(QDir::toNativeSeparators(result.outputFolder)));
        else GuiFunct::showEditorStopped(parent,"Copy To Stopped",stoppedMessage(result,true));
        return;
    }
    QDialog dialog(parent); dialog.setWindowTitle("Copy To — Static Object"); GuiFunct::styleEditorDialog(&dialog);
    auto *layout=new QVBoxLayout(&dialog); title(layout,"COPY TO — STATIC OBJECT");
    auto *destinationLabel=new QLabel("Destination: "+target.name+"\n"+QDir::toNativeSeparators(target.root));
    destinationLabel->setTextFormat(Qt::PlainText); destinationLabel->setWordWrap(true); layout->addWidget(destinationLabel);
    auto *form=new QFormLayout;
    auto *name=new QLineEdit(suggestedName);
    auto *category=new QComboBox; category->setEditable(true); category->addItems(target.categories);
    category->setEditText(suggestedCategory);
    form->addRow("Name:",name); form->addRow("Category:",category); GuiFunct::alignEditorForm(form,80); layout->addLayout(form);
    auto *info=new QLabel(QString("%1 resources, %2 MiB. Identical existing files are reused. Filename conflicts stop the transfer. Settings are preserved automatically.")
        .arg(plan.files.size()).arg(double(plan.bytes)/(1024*1024),0,'f',2)); info->setWordWrap(true); layout->addWidget(info);
    auto *resources=new QPlainTextEdit; resources->setReadOnly(true); QStringList listing;
    for(const auto &file:plan.files) listing<<(file.reuse?"REUSE ":"COPY ")+file.relative+QString(" (%1 bytes)").arg(file.bytes);
    listing<<""<<plan.warnings; resources->setPlainText(listing.join('\n')); layout->addWidget(resources,1);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Cancel);
    auto *copy=buttons->addButton("Copy Object",QDialogButtonBox::AcceptRole); GuiFunct::styleEditorActionButton(copy); layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    QObject::connect(copy,&QPushButton::clicked,&dialog,[&](){
        const auto result=StaticCopy::transfer(plan,name->text(),category->currentText());
        if(result.refUpdated) {
            GuiFunct::showEditorNotice(&dialog,"Copy To Complete",QString("Copied %1 resources (%2 MiB), reused %3 identical files and updated the destination REF.\n\nReload the REF in the receiving route to use the object.")
                .arg(result.copied.size()).arg(double(result.bytes)/(1024*1024),0,'f',2).arg(result.reused.size())); dialog.accept();
        } else GuiFunct::showEditorStopped(&dialog,"Copy To Stopped",stoppedMessage(result,false));
    });
    dialog.resize(780,550); dialog.exec();
}
}
