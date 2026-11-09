// Route paint order, configuration and staged-save regression. GNU GPL v3 or later.
#include "RoutePaintPlan.h"
#include "RouteSaveTransaction.h"
#include "TerrainTextureFormat.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QDebug>

int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);
    bool ok=true;
    auto require=[&](bool value,const char *message){if(!value){qCritical()<<message;ok=false;}};
    using namespace RoutePaintPlan;
    Plan plan;
    plan.steps={{"Wide",Track,true},{"Narrow",Track,false},{},{"Road",Roads,true},{"Bank",Water,true}};
    const QJsonArray presets={QJsonObject{{"name","Wide"}},QJsonObject{{"name","Narrow"}},
        QJsonObject{{"name","Road"}},QJsonObject{{"name","Bank"}}};
    QString error;
    require(validate(plan,presets,error),"valid sequence rejected");
    QVector<int> rows;
    require(execute(plan,(1u<<Track)|(1u<<Water),[&](qsizetype row,const Step &step){
        rows.append(int(row)); if(row==0) require(step.mirror,"mirror setting lost");
        if(row==1) require(!step.mirror,"mirror setting leaked from previous pass");
        return true;
    }),"paint execution failed");
    require(rows==QVector<int>({0,1,4}),"layer order or absent-road skipping incorrect");
    rows.clear();
    execute(plan,(1u<<Roads)|(1u<<Water),[&](qsizetype row,const Step &){rows.append(int(row));return true;});
    require(rows==QVector<int>({3,4}),"track-free tile must still run roads/water in order");
    rows.clear();
    execute(plan,0,[&](qsizetype row,const Step &){rows.append(int(row));return true;});
    require(rows.isEmpty(),"empty tile painted");
    rows.clear();
    require(!execute(plan,~0u,[&](qsizetype row,const Step &){rows.append(int(row));return row!=1;}),"failed row ignored");
    require(rows==QVector<int>({0,1}),"failure must stop later layers");
    const auto restored=fromJson(toJson(plan));
    require(restored.steps.size()==5
        && !restored.steps[1].mirror && restored.steps[4].target==Water,"recipe round trip failed");
    Plan invalid=plan; invalid.steps[0].preset="Removed";
    require(!validate(invalid,presets,error),"missing preset accepted");
    invalid=plan; invalid.steps[0].target=Unused;
    require(!validate(invalid,presets,error),"partially configured row accepted");
    require(!validate(plan,{},error),"no presets accepted");
    require(!validate(Plan{},presets,error),"empty sequence accepted");
    invalid=plan; invalid.steps.resize(11);
    require(!validate(invalid,presets,error),"more than ten slots accepted");
    auto legacy=toJson(plan);
    legacy["dds"]=true; legacy["mipmaps"]=true; legacy["outputResolution"]=1024;
    const auto migrated=toJson(fromJson(legacy));
    require(!migrated.contains("dds")&&!migrated.contains("mipmaps")&&!migrated.contains("outputResolution"),
        "removed output controls must not survive legacy recipe loading");
    require(fromJson(legacy).steps.size()==plan.steps.size(),"legacy recipe paint sequence lost");
    QTemporaryDir directory;
    require(directory.isValid(),"temporary route unavailable");
    const QString route=directory.path()+"/route";
    require(QDir().mkpath(route+"/terrtex"),"fixture folder unavailable");
    require(QDir().mkpath(route+"/terrtex/snow"),"snow folder unavailable");
    auto write=[&](const QString &path){QFile file(path);return file.open(QIODevice::WriteOnly)&&file.write("original")==8;};
    require(write(route+"/terrtex/patch.ace"),"shared input fixture failed");
    require(write(route+"/terrtex/snow/patch_ace_dds24.dds"),"season collision fixture failed");
    const QString fresh=freshPatchName("patch",true,{route+"/terrtex/",route+"/terrtex/snow/"});
    require(fresh=="patch_ace_ace_dds24.dds","fresh name must avoid shared/opposite-format/season files");
    require(TerrainTextureFormat::isPaintedPatchName(fresh,"patch.ace"),"fresh output lost ordinary paint compatibility");
    const QString output=route+"/terrtex/patch.dds";
    {
        RouteSaveTransaction transaction(route,directory.path()+"/recovery");
        require(transaction.addFile(output,QByteArray(2*1024*1024,'x'),&error),"disk staging failed");
        require(!QFile::exists(output),"staged patch published before tile commit");
        // Simulate a later row failing: destruction must discard staged output.
    }
    require(!QFile::exists(output),"failed tile left a painted patch on disk");
    {
        RouteSaveTransaction transaction(route,directory.path()+"/recovery");
        require(transaction.addFile(output,"saved",&error),"completed patch staging failed");
        require(transaction.commit(&error),"completed tile commit failed");
    }
    QFile file(output); require(file.open(QIODevice::ReadOnly) && file.readAll()=="saved","completed save missing");
    return ok?0:1;
}
