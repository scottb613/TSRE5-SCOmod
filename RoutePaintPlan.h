// TSRE GenX route paint sequence. GNU GPL v3 or later.
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>
#include <QFile>
#include <QStringList>

namespace RoutePaintPlan {
enum Target { Unused = 0, Track = 1, Roads = 2, Water = 3 };
struct Step { QString preset; Target target = Unused; bool mirror = true; };
struct Plan {
    QVector<Step> steps;
};
inline bool validate(const Plan &plan, const QJsonArray &presets, QString &error) {
    if(presets.isEmpty()) { error = "Create paint presets in the Texture Panel first."; return false; }
    if(plan.steps.size() > 10) { error = "The sequence supports ten slots."; return false; }
    bool used = false;
    for(qsizetype i = 0; i < plan.steps.size(); ++i) {
        const auto &step = plan.steps[i];
        if(step.preset.isEmpty() && step.target == Unused) continue;
        bool found = false;
        for(const auto &preset : presets)
            if(preset.toObject()["name"].toString() == step.preset) found = true;
        if(!found || step.target < Track || step.target > Water) {
            error = QString("Row %1: choose an existing preset and a paint target, or clear both.").arg(i+1);
            return false;
        }
        used = true;
    }
    if(!used) error = "Configure at least one paint row.";
    return used;
}
template<class Paint>
bool execute(const Plan &plan, unsigned presence, Paint paint) {
    for(qsizetype i = 0; i < plan.steps.size(); ++i) {
        const auto &step = plan.steps[i];
        if(step.target != Unused && (presence & (1u << step.target)))
            if(!paint(i, step)) return false;
    }
    return true;
}
inline QJsonObject toJson(const Plan &plan) {
    QJsonArray rows;
    for(const auto &step : plan.steps) rows.append(QJsonObject{
        {"preset",step.preset},{"target",int(step.target)},{"mirror",step.mirror}});
    return {{"version",1},{"rows",rows}};
}
inline Plan fromJson(const QJsonObject &object) {
    Plan result;
    // Legacy format/mipmap/size recipe fields are intentionally ignored.
    for(const auto &value : object["rows"].toArray()) {
        const auto row = value.toObject();
        result.steps.append({row["preset"].toString(),Target(row["target"].toInt()),row["mirror"].toBool(true)});
    }
    return result;
}
inline QString freshPatchName(QString base,bool dds,const QStringList &folders) {
    for(;;) {
        const QString output=base+(dds?"_dds24.dds":".ace");
        bool exists=false;
        for(const QString &folder:folders)
            exists|=QFile::exists(folder+output) || QFile::exists(folder+base+".dds")
                || QFile::exists(folder+base+".ace") || QFile::exists(folder+base+"_dds24.dds");
        if(!exists) return output;
        base+="_ace";
    }
}
}
