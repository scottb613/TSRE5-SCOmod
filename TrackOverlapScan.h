// Read-only geometry and connectivity policy for the full-route track scan.
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef TRACK_OVERLAP_SCAN_H
#define TRACK_OVERLAP_SCAN_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace TrackOverlapScan {
struct Point { double x = 0, y = 0, z = 0; };
inline Point operator+(Point a, Point b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Point operator-(Point a, Point b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Point operator*(Point a, double t) { return {a.x*t, a.y*t, a.z*t}; }
inline double dot(Point a, Point b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline double length(Point a) { return std::sqrt(dot(a,a)); }
inline bool finite(Point a) { return std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(a.z); }

// Zero-length straight subsections are valid dynamic-track placeholders.
// Preserve their coordinates for joint checks; only their rail length is zero.
inline bool validSubsection(int type,double size,double radius,double angle) {
    if(type == 0) return std::isfinite(size) && size >= 0 && size <= 100000;
    if(type != 1 || !std::isfinite(radius) || radius <= 0 || !std::isfinite(angle)) return false;
    const double metres = std::abs(radius*angle);
    return std::isfinite(metres) && metres <= 100000;
}
struct Node {
    int id = 0;
    int type = -1; // 0 end, 1 vector, 2 junction
    double metres = 0;
    bool valid = true;
    std::vector<int> links;
};
struct Component {
    int id = -1;
    double metres = 0;
    int ends = 0, vectors = 0, junctions = 0;
    bool valid = true;
    bool singlePiece(std::size_t owners) const {
        return valid && vectors > 0 && owners == 1;
    }
    bool shortIsolated(double limit) const {
        return valid && ends == 2 && vectors > 0 && junctions == 0
            && metres > 0 && metres <= limit;
    }
};
struct Connectivity {
    std::unordered_map<int, int> componentOf;
    std::vector<Component> components;
};
// Include incoming as well as outgoing links. A broken one-way link must not
// turn part of a connected network into an apparently independent orphan.
inline Connectivity classify(const std::vector<Node>& nodes) {
    Connectivity result;
    std::unordered_map<int, const Node*> byId;
    std::unordered_map<int, std::vector<int>> edges;
    std::unordered_map<int, bool> valid;
    for(const Node& n : nodes) {
        byId.emplace(n.id, &n);
        valid[n.id] = n.valid && std::isfinite(n.metres) && n.metres >= 0;
    }
    for(const Node& n : nodes) {
        const int expected = n.type == 0 ? 1 : n.type == 1 ? 2 : n.type == 2 ? 3 : -1;
        if(int(n.links.size()) != expected) valid[n.id] = false;
        for(int other : n.links) {
            auto it = byId.find(other);
            if(other <= 0 || other == n.id || it == byId.end()) {
                valid[n.id] = false;
                continue;
            }
            edges[n.id].push_back(other);
            edges[other].push_back(n.id);
            const auto& back = it->second->links;
            if(std::find(back.begin(), back.end(), n.id) == back.end()) {
                valid[n.id] = false;
                valid[other] = false;
            }
        }
    }
    for(const Node& start : nodes) {
        if(result.componentOf.count(start.id)) continue;
        Component component;
        component.id = int(result.components.size());
        std::vector<int> pending{start.id};
        result.componentOf.emplace(start.id, component.id);
        while(!pending.empty()) {
            const int id = pending.back(); pending.pop_back();
            const Node& n = *byId.at(id);
            component.valid = component.valid && valid[id];
            component.metres += n.metres;
            component.ends += n.type == 0;
            component.vectors += n.type == 1;
            component.junctions += n.type == 2;
            for(int other : edges[id])
                if(result.componentOf.emplace(other, component.id).second)
                    pending.push_back(other);
        }
        result.components.push_back(component);
    }
    return result;
}

inline bool jointSeparated(Point a, Point b, double horizontalTolerance = 0.05,
                           double verticalTolerance = 0.05) {
    return finite(a) && finite(b)
        && (std::hypot(a.x-b.x,a.z-b.z) > horizontalTolerance
            || std::abs(a.y-b.y) > verticalTolerance);
}
// A terminal marker has no adjoining rail beyond the vector endpoint. Its
// auxiliary position can be stale without creating a rail-to-rail gap.
// Validate its reciprocal link separately, but compare geometry only where
// another vector or junction actually continues the track.
inline bool linkedJointSeparated(int targetType, Point a, Point b,
                                 double horizontalTolerance = 0.05, double verticalTolerance = 0.05) {
    return (targetType == 1 || targetType == 2)
            && jointSeparated(a,b,horizontalTolerance,verticalTolerance);
}
// TrPinK identifies the endpoint on a vector target: 1 = start, 0 = end.
// Use it to resolve legitimate loops with two links to the same node.
inline int reciprocalPin(int sourceId,int sourceType,int sourcePin,int direction,
                         int targetType,const std::vector<int>& links,
                         const std::vector<int>& directions) {
    if(direction != 0 && direction != 1) return -1;
    if(targetType == 1) {
        const int pin = direction == 1 ? 0 : 1;
        if(pin >= int(links.size()) || links[pin] != sourceId) return -1;
        if(sourceType == 1 && (pin >= int(directions.size())
                || directions[pin] != (sourcePin == 0 ? 1 : 0))) return -1;
        return pin;
    }
    for(int i = 0; i < int(links.size()); ++i)
        if(links[i] == sourceId && (sourceType != 1
                || (i < int(directions.size()) && directions[i] == (sourcePin == 0 ? 1 : 0)))) return i;
    return -1;
}
struct Path {
    int owner = -1;
    double metres = 0;
    Point start, end;
};
struct Segment { Point a, b; int path = -1; };
struct Cell {
    long long x = 0, z = 0;
    bool operator==(const Cell& other) const { return x == other.x && z == other.z; }
};
struct CellHash {
    std::size_t operator()(Cell c) const {
        const auto a = std::hash<long long>{}(c.x);
        const auto b = std::hash<long long>{}(c.z);
        return a ^ (b + std::size_t(0x9e3779b9) + (a << 6) + (a >> 2));
    }
};
struct Match { int other = -1; double metres = 0; double fraction = 0; };

// Clip an interval to |offset + slope*t| <= tolerance.
inline bool clip(double offset, double slope, double tolerance, double& lo, double& hi) {
    if(std::abs(slope) < 1e-12) return std::abs(offset) <= tolerance;
    double a = (-tolerance-offset)/slope, b = (tolerance-offset)/slope;
    if(a > b) std::swap(a,b);
    lo = std::max(lo,a); hi = std::min(hi,b);
    return hi > lo;
}
// Nearly parallel rail centre lines, including reversed placement. Height is
// checked separately so a bridge over the same alignment is not a duplicate.
inline bool overlap(const Segment& a, const Segment& b, double& lo, double& hi,
                    double horizontalTolerance = 0.25, double verticalTolerance = 0.20) {
    const Point delta = a.b-a.a, other = b.b-b.a;
    const double len = length(delta), otherLen = length(other);
    if(len < 1e-8 || otherLen < 1e-8) return false;
    const Point axis = delta*(1/len);
    const double cosine = dot(axis,other)*(1/otherLen);
    if(std::abs(cosine) < 0.9961946980917455) return false; // five degrees
    const double p0 = dot(b.a-a.a,axis), p1 = dot(b.b-a.a,axis);
    if(std::abs(p1-p0) < 1e-8) return false;
    lo = std::max(0.0,std::min(p0,p1));
    hi = std::min(len,std::max(p0,p1));
    if(hi <= lo) return false;
    const Point rate = other*(1/(p1-p0));
    const Point offset = b.a-a.a-rate*p0;
    const Point slope = rate-axis;
    const double horizontal = std::hypot(axis.x,axis.z);
    if(horizontal < 1e-8) return false;
    const Point side{-axis.z/horizontal,0,axis.x/horizontal};
    return clip(dot(offset,side),dot(slope,side),horizontalTolerance,lo,hi)
        && clip(offset.y,slope.y,verticalTolerance,lo,hi) && hi > lo;
}

class Geometry {
public:
    std::vector<Path> paths;
    std::vector<Segment> segments;
    std::unordered_map<Cell,std::vector<int>,CellHash> cells;
    static Cell cell(Point p) { return {static_cast<long long>(std::floor(p.x/8)),static_cast<long long>(std::floor(p.z/8))}; }
    // Bound samples for corrupt or extreme input. Caller reports skipped paths.
    bool addPath(int owner, const std::vector<Point>& points) {
        if(points.size() < 2) return false;
        double metres = 0;
        std::size_t count = 0;
        for(std::size_t i = 0; i < points.size(); ++i) {
            if(!finite(points[i]) || std::abs(points[i].x) > 1e12 || std::abs(points[i].z) > 1e12) return false;
            if(i == 0) continue;
            const double d = length(points[i]-points[i-1]);
            if(!std::isfinite(d) || d > 100000) return false;
            metres += d;
            count += static_cast<std::size_t>(std::ceil(d));
            if(count > 200000 || segments.size()+count > 5000000) return false;
        }
        if(metres < 0.01) return false;
        const int path = int(paths.size());
        paths.push_back({owner,metres,points.front(),points.back()});
        for(std::size_t i = 1; i < points.size(); ++i) {
            const Point delta = points[i]-points[i-1];
            const int countHere = static_cast<int>(std::ceil(length(delta)));
            for(int j = 0; j < countHere; ++j) {
                const Point a = points[i-1]+delta*(double(j)/countHere);
                const Point b = points[i-1]+delta*(double(j+1)/countHere);
                const int id = int(segments.size());
                segments.push_back({a,b,path});
                cells[cell((a+b)*0.5)].push_back(id);
            }
        }
        return true;
    }
    // Physical end-to-end adjacency is used only for pieces with no TDB entry.
    // It never generates a warning merely because a normal track end is near
    // another one. A connection on ANY path keeps the object out of this list.
    bool endpointConnections(std::set<int>& connected,
            const std::function<bool(std::size_t,std::size_t)>& progress) const {
        std::unordered_map<Cell,std::vector<std::pair<int,Point>>,CellHash> ends;
        for(std::size_t i = 0; i < paths.size(); ++i) {
            if(i % 256 == 0 && !progress(i,paths.size())) return false;
            const Path& path = paths[i];
            for(Point point : {path.start,path.end}) {
                const Cell c = cell(point);
                for(int dx = -1; dx <= 1; ++dx) for(int dz = -1; dz <= 1; ++dz) {
                    const auto it = ends.find({c.x+dx,c.z+dz});
                    if(it == ends.end()) continue;
                    for(const auto& other : it->second) {
                        if(other.first == path.owner) continue;
                        const Point delta = point-other.second;
                        if(std::hypot(delta.x,delta.z) <= 0.25 && std::abs(delta.y) <= 0.10) {
                            connected.insert(path.owner);
                            connected.insert(other.first);
                        }
                    }
                }
                ends[c].push_back({path.owner,point});
            }
        }
        return progress(paths.size(),paths.size());
    }
    // Candidate filter performs the TDB policy before expensive geometry work.
    // Cancellation is checked within large paths as well as between objects.
    bool find(const std::function<bool(int,int)>& eligible,
              const std::function<bool(std::size_t,std::size_t)>& progress,
              std::map<int,Match>& matches,
              const std::function<bool(int)>& candidate = [](int) { return true; },
              double horizontalTolerance = 0.25, double verticalTolerance = 0.20) const {
        if(!std::isfinite(horizontalTolerance) || !std::isfinite(verticalTolerance)
                || horizontalTolerance < 0.01 || horizontalTolerance > 5.0
                || verticalTolerance < 0.01 || verticalTolerance > 5.0) return false;
        const int searchRadius = horizontalTolerance <= 0.25 ? 1
                : int(std::ceil(horizontalTolerance/8.0)) + 1;
        std::map<std::pair<int,int>,double> totals;
        std::size_t comparisons = 0;
        for(std::size_t i = 0; i < segments.size(); ++i) {
            if(i % 2048 == 0 && !progress(i,segments.size())) return false;
            const Segment& s = segments[i];
            const int owner = paths[s.path].owner;
            if(!candidate(owner)) continue;
            // Collect and merge intervals per opposing path: adjacent samples
            // and duplicate tracks must never double-count the covered length.
            std::map<int,std::vector<std::pair<double,double>>> intervals;
            const Cell c = cell((s.a+s.b)*0.5);
            for(int dx = -searchRadius; dx <= searchRadius; ++dx)
                for(int dz = -searchRadius; dz <= searchRadius; ++dz) {
                auto found = cells.find({c.x+dx,c.z+dz});
                if(found == cells.end()) continue;
                for(int j : found->second) {
                    if(++comparisons % 32768 == 0 && !progress(i,segments.size())) return false;
                    const Segment& t = segments[j];
                    const int other = paths[t.path].owner;
                    if(owner == other || !eligible(owner,other)) continue;
                    double lo,hi;
                    if(overlap(s,t,lo,hi,horizontalTolerance,verticalTolerance)) intervals[t.path].push_back({lo,hi});
                }
            }
            for(auto& entry : intervals) {
                auto& ranges = entry.second;
                std::sort(ranges.begin(),ranges.end());
                double lo = ranges[0].first, hi = ranges[0].second, total = 0;
                for(std::size_t j = 1; j < ranges.size(); ++j) {
                    if(ranges[j].first > hi) { total += hi-lo; lo = ranges[j].first; }
                    hi = std::max(hi,ranges[j].second);
                }
                totals[{s.path,entry.first}] += total+hi-lo;
            }
        }
        for(const auto& entry : totals) {
            const Path& a = paths[entry.first.first];
            const Path& b = paths[entry.first.second];
            const double shorter = std::min(a.metres,b.metres);
            const double fraction = std::min(1.0,entry.second/shorter);
            // A single crossing or a shared endpoint is insufficient evidence.
            if(entry.second < 0.25 || fraction < 0.60) continue;
            Match& best = matches[a.owner];
            if(entry.second > best.metres)
                best = {b.owner,entry.second,fraction};
        }
        return progress(segments.size(),segments.size());
    }
};
} // namespace TrackOverlapScan
#endif
