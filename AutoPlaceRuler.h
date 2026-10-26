// Horizontal-distance object placement on editable AP guides. GPL v3 or later.
#pragma once
#include <QVector>
#include <QVector3D>
#include <QString>
#include <algorithm>
#include <cmath>

namespace AutoPlaceRuler {
struct Station { QVector3D point, direction; double distance=0; bool corner=false; };
inline QVector<Station> plan(const QVector<QVector3D> &input,double spacing,
        bool preserveCorners,QString &error) {
    error.clear(); QVector<Station> result;
    if(!std::isfinite(spacing) || spacing<1) { error="Spacing must be at least 1 m."; return result; }
    QVector<QVector3D> points;
    for(const auto &point:input) {
        if(!std::isfinite(point.x()) || !std::isfinite(point.y()) || !std::isfinite(point.z())) {
            error="Ruler contains an invalid point."; return {};
        }
        if(!points.isEmpty()) {
            const auto delta=point-points.back();
            if(std::hypot(double(delta.x()),double(delta.z()))<0.001) continue;
        }
        points.append(point);
    }
    if(points.size()<2) { error="Ruler (ap) needs at least two distinct points."; return {}; }
    QVector<double> lengths{0};
    QVector<QVector3D> directions;
    for(int i=1;i<points.size();++i) {
        auto direction=points[i]-points[i-1]; direction.setY(0);
        lengths.append(lengths.back()+std::hypot(double(direction.x()),double(direction.z())));
        directions.append(direction.normalized());
    }
    QVector<int> anchors{0};
    if(preserveCorners) for(int i=1;i+1<points.size();++i) {
        const double dot=std::clamp(double(QVector3D::dotProduct(directions[i-1],directions[i])),-1.0,1.0);
        const double turn=std::acos(dot)*180.0/3.14159265358979323846;
        if(turn>=45.0-1.e-5 && turn<=90.0+1.e-5) anchors.append(i);
    }
    anchors.append(points.size()-1);
    int segment=0;
    const auto append=[&](double distance,bool corner,int pointIndex) {
        while(segment+1<directions.size() && lengths[segment+1]<=distance+1.e-8) ++segment;
        const double t=std::clamp((distance-lengths[segment])/(lengths[segment+1]-lengths[segment]),0.0,1.0);
        QVector3D direction=directions[segment];
        if(corner) direction=(directions[pointIndex-1]+directions[pointIndex]).normalized();
        const QVector3D point=pointIndex>=0 ? points[pointIndex]
            : points[segment]+(points[segment+1]-points[segment])*float(t);
        if(result.isEmpty() || distance-result.back().distance>1.e-6)
            result.append(Station{point,direction,distance,corner});
    };
    append(0,false,0);
    for(int a=1;a<anchors.size();++a) {
        const int end=anchors[a];
        const double start=lengths[anchors[a-1]], finish=lengths[end];
        if((finish-start)/spacing>50000 || result.size()>50000) {
            error="Ruler placement exceeds the 50,000-object limit."; return {};
        }
        for(double distance=start+spacing; distance<finish-1.e-6; distance+=spacing) {
            // A forced endpoint/corner wins over a nearly coincident regular
            // post. Elsewhere, ordinary chain spacing is retained.
            if(finish-distance<0.5) break;
            append(distance,false,-1);
            if(result.size()>=50000) { error="Ruler placement exceeds the 50,000-object limit."; return {}; }
        }
        append(finish,a+1<anchors.size(),end);
    }
    if(result.size()>50000) { error="Ruler placement exceeds the 50,000-object limit."; return {}; }
    // Align ordinary posts to the actual placed spans, including stations on
    // either side of a guide bend. Forced corners keep their guide bisector.
    for(int i=0;i<result.size();++i) {
        if(result[i].corner) continue;
        QVector3D incoming, outgoing;
        if(i>0) { incoming=result[i].point-result[i-1].point; incoming.setY(0); incoming.normalize(); }
        if(i+1<result.size()) { outgoing=result[i+1].point-result[i].point; outgoing.setY(0); outgoing.normalize(); }
        const auto direction=incoming+outgoing;
        // A reversal has no unique bisector; retain its original heading.
        if(direction.lengthSquared()>1.e-8f) result[i].direction=direction.normalized();
    }
    return result;
}
}
