// TSRE GenX - Copyright (C) Scott Brunner, Beast of Burden.
// Licensed under GNU GPL v3 or later. See LICENSE.md.
#include "RouteDistroDialog.h"
#include "GuiFunct.h"
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QFileInfo>
#include <QEventLoop>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProgressDialog>
#include <QPushButton>
#include <QScopedValueRollback>
#include <QSignalBlocker>
#include <QSaveFile>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <utility>

namespace {
class DistroItem final : public QTreeWidgetItem {
public:
    using QTreeWidgetItem::QTreeWidgetItem;
    bool operator<(const QTreeWidgetItem &other) const override {
        const int column=treeWidget()?treeWidget()->sortColumn():0;
        if(column==2) {
            const qint64 left=data(0,Qt::UserRole+1).toLongLong(), right=other.data(0,Qt::UserRole+1).toLongLong();
            if(left!=right) return left<right;
            return text(0).localeAwareCompare(other.text(0))<0;
        }
        return QTreeWidgetItem::operator<(other);
    }
};
void prepareProgress(QProgressDialog &progress) {
    GuiFunct::styleEditorDialog(&progress);
    auto *label=new QLabel("Preparing full-route operation...");
    GuiFunct::styleEditorTitle(label); label->setWordWrap(true);
    progress.setLabel(label);
    progress.setWindowTitle("Pack for Distro");
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumWidth(480); progress.setMinimumDuration(0);
    progress.setAutoClose(false); progress.setAutoReset(false);
    progress.show();
}
}

RouteDistroDialog::RouteDistroDialog(const QString &routeRoot, const QString &install,
        QWidget *parent, std::function<bool()> allowed, std::function<void()> refresh)
    : QDialog(parent),root(routeRoot),installationRoot(install),canClean(std::move(allowed)),refreshTerrain(std::move(refresh)) {
    setWindowTitle("Pack for Distro"); resize(1100,740);
    GuiFunct::styleEditorDialog(this);
    auto *layout=new QVBoxLayout(this);
    auto *title=new QLabel("PACK FOR DISTRO",this);
    GuiFunct::styleEditorTitle(title); layout->addWidget(title);
    auto *help=new QLabel("Scan this saved route folder, review the files and savings, then permanently delete checked files. "
        "Elevation, terrain flags and referenced assets are protected. Optional _e.raw/_n.raw buffers can be selected. "
        "Warnings allow unrelated cleanup. No ZIP or backup is created; deletion cannot be undone.",this);
    help->setWordWrap(true); layout->addWidget(help);
    const RouteDistro::Options defaults;
    keepEditor=new QCheckBox("Preserve assets referenced by editor catalogs and PolyVeg schemas",this);
    keepEditor->setChecked(defaults.keepEditorAssets);
    keepEditor->setToolTip("On preserves catalog-only assets for further editing. Off scans a playable distribution; catalog-only models can become removal candidates.");
    layout->addWidget(keepEditor);
    consolidate=new QCheckBox("Offer consolidation of identical TERRTEX families (including all seasonal variants)",this);
    consolidate->setChecked(defaults.consolidateTerrtex);
    consolidate->setToolTip("Offer matching families for review. Only checked deletion rows update terrain references and remove duplicates. No recompression.");
    editorCaches=new QCheckBox("Include unreferenced maps, overlays and editing geodata (production copy)",this);
    editorCaches->setChecked(defaults.includeEditorCaches);
    editorCaches->setToolTip("Offer saved terrain_maps map tiles/overlays and osm_data files. Keep these for further editing; runtime references and uncertain dependencies remain protected.");
    layout->addWidget(consolidate); layout->addWidget(editorCaches);
    status=new QLabel("Choose Scan Full Route. Nothing is selected for removal automatically.",this);
    status->setWordWrap(true); layout->addWidget(status);
    auto *categoryActions=new QHBoxLayout;
    category=new QComboBox(this); category->addItem("Choose category",QString());
    category->setToolTip("Choose a category, then select or clear its deletion rows. Other categories stay unchanged.");
    selectCategoryButton=new QPushButton("Select Category",this);
    clearCategoryButton=new QPushButton("Clear Category",this);
    GuiFunct::styleEditorActionButton(selectCategoryButton); GuiFunct::styleEditorActionButton(clearCategoryButton);
    categoryActions->addWidget(category,1); categoryActions->addWidget(selectCategoryButton); categoryActions->addWidget(clearCategoryButton);
    layout->addLayout(categoryActions);
    items=new QTreeWidget(this);
    items->setHeaderLabels({"File (check to remove)","Category","MiB","Reason"});
    items->setRootIsDecorated(false); items->setUniformRowHeights(true);
    items->header()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
    items->header()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
    items->header()->setSectionResizeMode(2,QHeaderView::ResizeToContents);
    items->header()->setStretchLastSection(true);
    items->setSortingEnabled(true); items->sortItems(2,Qt::DescendingOrder);
    items->header()->setToolTip("Click MiB to sort by exact file bytes. Largest files are shown first initially.");
    layout->addWidget(items,2);
    details=new QPlainTextEdit(this); details->setReadOnly(true);
    details->setPlaceholderText("Blocking problems, warnings, external prerequisites and protected duplicates appear here.");
    warnings=new QTreeWidget(this); warnings->setHeaderLabels({"Warning group / details"});
    warnings->setUniformRowHeights(true);
    tabs=new QTabWidget(this); tabs->addTab(warnings,"Warnings (0)"); tabs->addTab(details,"Details / Results");
    layout->addWidget(tabs,1);
    selection=new QLabel(this); selection->setWordWrap(true); layout->addWidget(selection);
    auto *actions=new QHBoxLayout;
    scanButton=new QPushButton("Scan Full Route",this);
    allButton=new QPushButton("Select All Candidates",this);
    noneButton=new QPushButton("Select None",this);
    exportButton=new QPushButton("Export Report...",this);
    cleanButton=new QPushButton("Delete Selected",this);
    cleanButton->setToolTip("Permanently delete the checked files from the active route folder after confirmation.");
    closeButton=new QPushButton("Close",this);
    for(auto *button:{scanButton,allButton,noneButton,exportButton,cleanButton,closeButton}) {
        GuiFunct::styleEditorActionButton(button); actions->addWidget(button);
    }
    layout->addLayout(actions);
    connect(scanButton,&QPushButton::clicked,this,[this](){ scan(); });
    connect(cleanButton,&QPushButton::clicked,this,[this](){ clean(); });
    connect(closeButton,&QPushButton::clicked,this,&QDialog::reject);
    connect(category,&QComboBox::currentIndexChanged,this,[this](){ updateSelection(); });
    connect(selectCategoryButton,&QPushButton::clicked,this,[this](){ selectCategory(Qt::Checked); });
    connect(clearCategoryButton,&QPushButton::clicked,this,[this](){ selectCategory(Qt::Unchecked); });
    connect(exportButton,&QPushButton::clicked,this,[this](){ exportReport(); });
    const auto invalidate=[this](){
        reviewValid=false; status->setText("Scan options changed. Scan Full Route again before cleaning."); updateSelection();
    };
    for(auto *option:{keepEditor,consolidate,editorCaches}) connect(option,&QCheckBox::toggled,this,invalidate);
    connect(items,&QTreeWidget::itemChanged,this,[this](){ updateSelection(); });
    connect(allButton,&QPushButton::clicked,this,[this](){
        { const QSignalBlocker block(items);
          for(int i=0;i<items->topLevelItemCount();++i) items->topLevelItem(i)->setCheckState(0,Qt::Checked); }
        updateSelection();
    });
    connect(noneButton,&QPushButton::clicked,this,[this](){
        { const QSignalBlocker block(items);
          for(int i=0;i<items->topLevelItemCount();++i) items->topLevelItem(i)->setCheckState(0,Qt::Unchecked); }
        updateSelection();
    });
    updateSelection();
}
void RouteDistroDialog::reject() { if(!busy) QDialog::reject(); }
void RouteDistroDialog::closeEvent(QCloseEvent *event) {
    if(busy) event->ignore(); else QDialog::closeEvent(event);
}
void RouteDistroDialog::updateSelection() {
    qint64 bytes=0; int count=0;
    QMap<QString,qint64> selectedCategories;
    for(int i=0;i<items->topLevelItemCount();++i) {
        const auto *item=items->topLevelItem(i);
        if(item->checkState(0)==Qt::Checked) {
            count+=qMax(1,item->data(0,Qt::UserRole+2).toStringList().size());
            bytes+=item->data(0,Qt::UserRole+1).toLongLong();
            selectedCategories[item->text(1)]+=item->data(0,Qt::UserRole+1).toLongLong();
        }
    }
    QStringList categoryTotals;
    for(auto it=selectedCategories.cbegin();it!=selectedCategories.cend();++it)
        categoryTotals<<QString("%1: %2 MiB").arg(it.key()).arg(double(it.value())/(1024*1024),0,'f',2);
    selection->setText(QString("Selected: %1 files, %2 MiB").arg(count).arg(double(bytes)/(1024*1024),0,'f',2)
        +(categoryTotals.isEmpty()?QString():"\n"+categoryTotals.join("; ")));
    cleanButton->setEnabled(!busy && reviewValid && report.complete && count>0);
    allButton->setEnabled(!busy && reviewValid && report.complete && !report.removals.isEmpty());
    noneButton->setEnabled(!busy && count>0);
    scanButton->setEnabled(!busy); keepEditor->setEnabled(!busy); closeButton->setEnabled(!busy);
    consolidate->setEnabled(!busy); editorCaches->setEnabled(!busy);
    const bool chooseCategory=!busy && reviewValid && report.complete && !category->currentData().toString().isEmpty();
    category->setEnabled(!busy && reviewValid && report.complete);
    selectCategoryButton->setEnabled(chooseCategory); clearCategoryButton->setEnabled(chooseCategory);
    exportButton->setEnabled(!busy && (hasScan || !operationReports.isEmpty()));
}
void RouteDistroDialog::selectCategory(Qt::CheckState state) {
    if(busy || !reviewValid || !report.complete) return;
    const QString selectedCategory=category->currentData().toString();
    if(selectedCategory.isEmpty()) return;
    { const QSignalBlocker block(items);
      for(int i=0;i<items->topLevelItemCount();++i) {
        auto *item=items->topLevelItem(i);
        if(item->text(1)==selectedCategory) item->setCheckState(0,state);
      }
    }
    updateSelection();
}
QStringList RouteDistroDialog::checkedPaths() const {
    QStringList paths;
    for(int i=0;i<items->topLevelItemCount();++i) {
        const auto *item=items->topLevelItem(i);
        if(item->checkState(0)==Qt::Checked) paths<<item->data(0,Qt::UserRole).toString();
    }
    return paths;
}
void RouteDistroDialog::scan() {
    if(busy || (canClean && !canClean())) return;
    {
        QScopedValueRollback<bool> running(busy,true); updateSelection();
        QProgressDialog progress("Scanning complete route...","Cancel",0,1000,this); prepareProgress(progress);
        QElapsedTimer elapsed; elapsed.start(); int shown=0;
        report=RouteDistro::scan(root,installationRoot,{keepEditor->isChecked(),consolidate->isChecked(),editorCaches->isChecked()},
            [&](double fraction,const QString &message){
                if(elapsed.elapsed()>=40 || fraction>=1) {
                    shown=qMax(shown,qMin(1000,int(fraction*1000)));
                    progress.setValue(shown);
                    progress.setLabelText(QString("%1% — %2").arg(shown/10).arg(message));
                    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents); elapsed.restart();
                }
                return !progress.wasCanceled();
            });
    }
    hasScan=true;
    const int sortColumn=items->sortColumn();
    const Qt::SortOrder sortOrder=items->header()->sortIndicatorOrder();
    items->setSortingEnabled(false);
    { const QSignalBlocker block(items);
      items->clear();
      for(const auto &removal:report.removals) {
        auto *item=new DistroItem(items,QStringList{removal.path,removal.category,
            QString::number(double(removal.bytes)/(1024*1024),'f',3),removal.reason});
        item->setData(0,Qt::UserRole,removal.path); item->setData(0,Qt::UserRole+1,removal.bytes);
        item->setData(0,Qt::UserRole+2,removal.related);
        item->setToolTip(0,removal.reason); item->setToolTip(3,removal.reason);
        item->setToolTip(2,QString("%1 exact file bytes; click the MiB header to sort numerically").arg(removal.bytes));
        item->setCheckState(0,Qt::Unchecked);
        if(!report.complete) item->setDisabled(true);
      }
    }
    items->setSortingEnabled(true); items->sortItems(sortColumn,sortOrder);
    { const QSignalBlocker block(category);
      category->clear(); category->addItem("Choose category",QString());
      QMap<QString,qint64> sizes; QMap<QString,qsizetype> counts;
      for(const auto &removal:report.removals) {
        sizes[removal.category]+=removal.bytes; counts[removal.category]+=qMax(qsizetype(1),removal.related.size());
      }
      for(auto it=sizes.cbegin();it!=sizes.cend();++it)
        category->addItem(QString("%1 — %2 files, %3 MiB").arg(it.key()).arg(counts[it.key()]).arg(double(it.value())/(1024*1024),0,'f',2),it.key());
    }
    reviewValid=report.complete && !report.cancelled;
    status->setText(QString("Scanned %1 route files. %2 candidate entries, %3 MiB removable.\n%4")
        .arg(report.files).arg(report.removals.size()).arg(double(report.removableBytes)/(1024*1024),0,'f',2)
        .arg(report.cancelled?"Scan cancelled; deletion disabled.":(reviewValid?"Review and check files to delete.":"Blocking problems; deletion disabled.")));
    details->setPlainText(RouteDistro::summary(report)+"\n\n"+(report.problems.isEmpty()?QString("Dependency problems: none\n"):
        "DEPENDENCY PROBLEMS — CLEANUP DISABLED\n"+report.problems.join('\n')+'\n')
        +"\n\nNOTICES / TERRTEX GROUPS\n"+report.notices.join('\n'));
    warnings->clear();
    const auto grouped=RouteDistro::warningGroups(report.warnings);
    for(auto it=grouped.cbegin();it!=grouped.cend();++it) {
        auto *group=new QTreeWidgetItem(warnings,QStringList{QString("%1 (%2)").arg(it.key()).arg(it->size())});
        for(const QString &warning:it.value()) {
            auto *item=new QTreeWidgetItem(group,QStringList{warning}); item->setToolTip(0,warning);
        }
    }
    warnings->collapseAll(); tabs->setTabText(0,QString("Warnings (%1)").arg(report.warnings.size()));
    tabs->setCurrentWidget(report.problems.isEmpty() && !grouped.isEmpty()?static_cast<QWidget*>(warnings):static_cast<QWidget*>(details));
    updateSelection();
}
void RouteDistroDialog::clean() {
    if(busy || !reviewValid || !report.complete || (canClean && !canClean())) return;
    QStringList selected;
    qint64 selectedBytes=0; qsizetype fileCount=0; bool consolidating=false;
    for(int i=0;i<items->topLevelItemCount();++i) if(items->topLevelItem(i)->checkState(0)==Qt::Checked) {
        const auto *item=items->topLevelItem(i);
        selected<<item->data(0,Qt::UserRole).toString();
        const auto related=item->data(0,Qt::UserRole+2).toStringList();
        fileCount+=qMax(qsizetype(1),related.size());
        selectedBytes+=item->data(0,Qt::UserRole+1).toLongLong();
        consolidating=consolidating || !related.isEmpty();
    }
    if(selected.isEmpty()) return;
    QString message=QString("Permanently DELETE %1 checked files (%2 MiB)?\n\nRoute folder:\n%3\n\n"
        "These files will be deleted from this folder. No ZIP or backup is created. Undo cannot restore deleted files.\n\n")
        .arg(fileCount).arg(double(selectedBytes)/(1024*1024),0,'f',2).arg(QDir::toNativeSeparators(root));
    if(consolidating) message+="For selected duplicate TERRTEX families, terrain references are updated to the retained identical textures before the duplicates are deleted. Loaded terrain refreshes and editor Undo history clears.\n\n";
    message+="The route and files are rechecked before deletion. Cancel stops remaining work; completed deletions and terrain-reference updates remain applied.";
    if(!GuiFunct::confirmDestructiveAction(this,"Delete Selected Files",message)) return;
    RouteDistro::Result result;
    {
        QScopedValueRollback<bool> running(busy,true); updateSelection();
        QProgressDialog progress("Revalidating route before deletion...","Cancel",0,1000,this); prepareProgress(progress);
        QElapsedTimer elapsed; elapsed.start(); int shown=0;
        result=RouteDistro::clean(report,selected,[&](double fraction,const QString &message){
            if(elapsed.elapsed()>=40 || fraction>=1) {
                shown=qMax(shown,qMin(950,int(fraction*950))); progress.setValue(shown);
                progress.setLabelText(QString("%1% — %2").arg(shown/10).arg(message));
                QApplication::processEvents(QEventLoop::ExcludeUserInputEvents); elapsed.restart();
            }
            return !progress.wasCanceled();
        });
        if(!result.rewritten.isEmpty() && refreshTerrain) {
            progress.setLabelText("Refreshing loaded terrain after reference updates...");
            progress.setValue(960); refreshTerrain();
        }
        if(!result.cancelled && result.errors.isEmpty() && result.sizeMeasured) progress.setValue(1000);
    }
    operationReports.append(RouteDistro::auditReport(report,selected,&result));
    reviewValid=false;
    status->setText(QString("%1Deleted %2 files from %3 selected entries (%4 MiB). Updated %5 terrain descriptors. Scan Full Route again to review the current folder.")
        .arg(result.cancelled?"Deletion cancelled. ":(result.errors.isEmpty()?"Deletion complete. ":"Deletion stopped. "))
        .arg(result.removed.size()).arg(selected.size()).arg(double(result.bytes)/(1024*1024),0,'f',2).arg(result.rewritten.size())
        +"\n"+RouteDistro::sizeSummary(result));
    details->appendPlainText("\nMEASURED ROUTE SIZE\n"+RouteDistro::sizeSummary(result)+"\n\nUPDATED TERRAIN\n"+result.rewritten.join('\n')+"\n\nDELETED\n"+result.removed.join('\n')+"\n\n"+result.errors.join('\n'));
    tabs->setCurrentWidget(details);
    { const QSignalBlocker block(items);
      for(int i=0;i<items->topLevelItemCount();++i) items->topLevelItem(i)->setDisabled(true); }
    updateSelection();
}

void RouteDistroDialog::exportReport() {
    if(busy || (!hasScan && operationReports.isEmpty())) return;
    QFileDialog chooser(this,"Export Review / Deletion Report",
        QDir(QFileInfo(root).absolutePath()).filePath(QFileInfo(root).fileName()+"-distro-report-"
            +QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmss")+".txt"),"Text reports (*.txt)");
    chooser.setAcceptMode(QFileDialog::AcceptSave);
    chooser.setFileMode(QFileDialog::AnyFile);
    chooser.setDefaultSuffix("txt");
    if(chooser.exec()!=QDialog::Accepted || chooser.selectedFiles().isEmpty()) return;
    const QString filename=chooser.selectedFiles().first();
    const QString canonical=QFileInfo(filename).canonicalFilePath();
    const QString route=QFileInfo(root).canonicalFilePath();
    if(!canonical.isEmpty() && canonical.startsWith(route+'/',Qt::CaseInsensitive)) {
        GuiFunct::showEditorStopped(this,"Export Report","Choose a new report filename or save outside the route folder to preserve existing route files."); return;
    }
    QString text="Current review ready for deletion revalidation: "+QString(reviewValid?"yes":"no; rescan required")
        +"\nCurrent dialog status: "+status->text()+"\n\n";
    if(hasScan) text+=RouteDistro::auditReport(report,checkedPaths());
    for(qsizetype i=0;i<operationReports.size();++i)
        text+="\n\n========== DELETION OPERATION "+QString::number(i+1)+" ==========\n"+operationReports[i];
    QSaveFile file(filename); file.setDirectWriteFallback(false);
    const QByteArray bytes=text.toUtf8();
    if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit()) {
        GuiFunct::showEditorStopped(this,"Export Report","Could not save the report:\n"+file.errorString()); return;
    }
    if(QFileInfo(filename).canonicalFilePath().startsWith(route+'/',Qt::CaseInsensitive)) {
        reviewValid=false;
        status->setText("Report saved inside the route folder. Scan Full Route again before deleting files.");
        updateSelection();
    }
    GuiFunct::showEditorNotice(this,"Report Saved",QDir::toNativeSeparators(filename));
}
