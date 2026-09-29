// Focused policy tests; no route files or OpenGL context required.
#include "TrackOverlapScan.h"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace TrackOverlapScan;
static int failures = 0;
static void require(bool condition,const char* message) {
    if(!condition) { std::cerr << "FAIL: " << message << '\n'; ++failures; }
}
static std::map<int,Match> scan(const std::vector<Point>& a,const std::vector<Point>& b,
                              bool suspect = true) {
    Geometry geometry;
    require(geometry.addPath(0,a),"first path accepted");
    require(geometry.addPath(1,b),"second path accepted");
    std::map<int,Match> matches;
    require(geometry.find([&](int x,int y){return suspect && x == 0 && y == 1;},
        [](std::size_t,std::size_t){return true;},matches),"scan completed");
    return matches;
}
int main() {
    require(!jointSeparated({0,0,0},{0.083,0,0},0.25,0.20),"gap below configured horizontal threshold is ignored");
    require(jointSeparated({0,0,0},{0.083,0,0},0.05,0.20),"same gap above smaller threshold is reported");
    require(!jointSeparated({0,0,0},{0.25,0.20,0},0.25,0.20),"exact threshold is allowed");
    require(jointSeparated({0,0,0},{0,0.21,0},0.25,0.20),"vertical threshold independently reports a gap");
    require(!linkedJointSeparated(1,{0,0,0},{0.083,0,0},0.25,0.20),"linked joints use configured thresholds");
    require(!linkedJointSeparated(0,{0,0,0},{5,5,0},0.25,0.20),"terminal markers remain excluded");
    Geometry thresholds;
    require(thresholds.addPath(0,{{7.9,0,0},{7.9,0,25}}),"threshold baseline");
    require(thresholds.addPath(1,{{8.3,0.3,0},{8.3,0.3,25}}),"offset across spatial cell boundary");
    const auto thresholdMatches = [&](double horizontal, double vertical){
        std::map<int,Match> result;
        require(thresholds.find([](int a,int b){return a == 0 && b == 1;},
            [](std::size_t,std::size_t){return true;},result,
            [](int){return true;},horizontal,vertical),"threshold scan completes");
        return result.size();
    };
    require(thresholdMatches(0.25,0.20) == 0,"defaults reject offset track");
    require(thresholdMatches(0.50,0.20) == 0,"height threshold still applies");
    require(thresholdMatches(0.25,0.40) == 0,"horizontal threshold still applies");
    require(thresholdMatches(0.50,0.40) == 1,"both expanded thresholds admit overlap");
    require(thresholdMatches(0.01,0.01) == 0,"minimum thresholds accepted");
    std::map<int,Match> invalidThresholdMatches;
    require(!thresholds.find([](int,int){return true;},
        [](std::size_t,std::size_t){return true;},invalidThresholdMatches,
        [](int){return true;},std::numeric_limits<double>::quiet_NaN(),0.20),"reject nonfinite threshold");
    const std::vector<Point> straight{{0,0,0},{0,0,25}};
    auto matches = scan(straight,straight);
    require(matches.size() == 1 && std::abs(matches[0].metres-25) < 1e-6,"exact duplicate, once only");
    matches = scan({{0,0,10},{0,0,15}},straight);
    require(matches.size() == 1 && std::abs(matches[0].metres-5) < 1e-6,"short piece inside longer footprint");
    matches = scan({{0,0,15},{0,0,10}},straight);
    require(matches.size() == 1,"reversed duplicate");
    require(scan(straight,{{0,0,25},{0,0,50}}).empty(),"shared endpoint is not stacking");
    require(scan(straight,{{-10,0,12},{10,0,12}}).empty(),"perpendicular crossover");
    require(scan(straight,{{3,0,0},{3,0,25}}).empty(),"parallel adjacent track");
    require(scan(straight,{{0,4,0},{0,4,25}}).empty(),"bridge above track");
    require(scan(straight,{{0,0.21,0},{0,0.21,25}}).empty(),"height tolerance");
    require(scan(straight,{{0.26,0,0},{0.26,0,25}}).empty(),"lateral tolerance");
    require(!scan(straight,{{0.20,0.1,0},{0.20,0.1,25}}).empty(),"slightly offset stacked piece");
    require(scan(straight,straight,false).empty(),"valid connected overlay excluded by TDB policy");
    require(scan({{0,0,0},{0,1,25}},{{0,0,0},{0,1,25}}).size() == 1,"matching grade");
    require(scan(straight,{{-1,0,0},{1,0,25}}).empty(),"shallow crossing lacks sustained overlap");
    require(scan({{0,0,0},{0,0,0.1}},{{0,0,0},{0,0,0.1}}).empty(),"sub-25cm overlap is too little evidence");
    require(scan({{0,0,0},{0,0,0.5}},{{0,0,0},{0,0,0.5}}).size() == 1,"short half-metre duplicate");
    // Different tessellation and reversed order on a curve spanning cell/tile boundaries.
    std::vector<Point> curve;
    for(int i=0;i<=100;++i) {
        const double angle = i*0.002;
        curve.push_back({-30000000+100*std::cos(angle),7,4090+100*std::sin(angle)});
    }
    std::vector<Point> reverse(curve.rbegin(),curve.rend());
    require(scan(curve,reverse).size() == 1,"curve, reverse orientation, distant origin and tile boundary");
    Geometry geometry;
    require(geometry.addPath(0,straight),"add cancel fixture");
    std::map<int,Match> cancelled;
    require(!geometry.find([](int,int){return true;},[](std::size_t,std::size_t){return false;},cancelled),"cancel scan");
    require(!geometry.addPath(2,{{0,0,0},{std::numeric_limits<double>::quiet_NaN(),0,1}}),"reject NaN geometry");
    require(!geometry.addPath(2,{{0,0,0},{0,0,100001}}),"bound extreme geometry");
    require(geometry.paths.size() == 1,"invalid path does not partly enter index");

    std::vector<Node> isolated{{1,0,0,true,{2}},{2,1,25,true,{1,3}},{3,0,0,true,{2}}};
    auto graph = classify(isolated);
    require(graph.components.size() == 1 && graph.components[0].shortIsolated(100),"short isolated vector");
    require(!graph.components[0].shortIsolated(20),"orphan length is configurable");
    auto longNetwork = isolated; longNetwork[1].metres = 500;
    require(!classify(longNetwork).components[0].shortIsolated(100),"long connected network");
    auto broken = isolated; broken[2].links = {99};
    require(!classify(broken).components[0].shortIsolated(100),"broken reciprocal link is unknown, not orphan");
    // A short section linked to a much longer vector is measured as one component.
    std::vector<Node> chain{{1,0,0,true,{2}},{2,1,10,true,{1,4}},
        {4,1,200,true,{2,3}},{3,0,0,true,{4}}};
    graph = classify(chain);
    require(graph.components.size() == 1 && !graph.components[0].shortIsolated(100),"whole-component length, not local vector length");
    std::vector<Node> siding{{1,2,0,true,{2,4,6}},{2,1,10,true,{1,3}},{3,0,0,true,{2}},
        {4,1,20,true,{1,5}},{5,0,0,true,{4}},{6,1,30,true,{1,7}},{7,0,0,true,{6}}};
    graph = classify(siding);
    require(graph.components.size() == 1 && !graph.components[0].shortIsolated(100),"connected siding and junction are preserved");
    auto unknown = isolated; unknown[1].valid = false;
    require(!classify(unknown).components[0].shortIsolated(100),"missing section geometry must not create an orphan");
    auto incoming = isolated;
    incoming.push_back({4,1,500,true,{2,5}});
    incoming.push_back({5,0,0,true,{4}});
    graph = classify(incoming);
    require(graph.components.size() == 1 && !graph.components[0].shortIsolated(100),"incoming one-way link joins component conservatively");
    std::vector<Node> loop{{1,1,10,true,{2,2}},{2,1,10,true,{1,1}}};
    require(!classify(loop).components[0].shortIsolated(100),"closed loop is not an isolated two-ended stub");
    require(!jointSeparated({0,0,0},{0.01,0.01,0.01}),"joint rounding tolerance");
    require(jointSeparated({0,0,0},{0.10,0,0}),"horizontal joint gap");
    require(jointSeparated({0,0,0},{0,0.10,0}),"vertical joint gap");
    require(jointSeparated({30000000,0,-30000000},{30000000.1,0,-30000000}),"distant joint precision");
    require(!linkedJointSeparated(0,{0,10,0},{0,9.879,0}),"stale terminal-marker elevation is not a rail joint gap");
    require(linkedJointSeparated(1,{0,10,0},{0,9.879,0}),"same elevation gap between vectors remains actionable");
    require(linkedJointSeparated(2,{0,10,0},{0,9.879,0}),"same elevation gap at a junction remains actionable");
    require(reciprocalPin(5,1,0,1,0,{5},{1}) == 0,"vector start to end node");
    require(reciprocalPin(5,1,1,1,0,{5},{0}) == 0,"vector finish to end node");
    require(reciprocalPin(5,1,0,1,2,{5,5,7},{1,0,1}) == 0,"loop starts at junction");
    require(reciprocalPin(5,1,1,1,2,{5,5,7},{1,0,1}) == 1,"loop returns to same junction");
    require(reciprocalPin(5,0,0,1,1,{5,6},{1,1}) == 0,"end node points to vector start");
    require(reciprocalPin(5,0,0,0,1,{5,6},{1,1}) < 0,"wrong endpoint direction detected");
    require(reciprocalPin(5,1,0,1,0,{99},{1}) < 0,"missing reciprocal link detected");
    require(validSubsection(0,0,0,0),"zero-length dynamic subsection is valid");
    require(validSubsection(1,0,500,0),"zero-angle curve with valid radius is permitted");
    require(validSubsection(1,0,500,0.08),"ordinary dynamic curve is valid");
    require(!validSubsection(0,-1,0,0),"negative straight length rejected");
    require(!validSubsection(1,0,0,0.08),"nonzero curve requires a positive radius");
    require(!validSubsection(0,std::numeric_limits<double>::quiet_NaN(),0,0),"NaN length rejected");
    require(!validSubsection(2,10,500,0.08),"unknown subsection type rejected");
    // A curve/zero-length-placeholder/straight joint remains checked at the
    // placeholder's position. Millimetre rounding is fine; a real gap is not.
    const Point curveEnd{100.0026,20,50}, placeholder{100,20,50};
    require(!jointSeparated(curveEnd,placeholder),"curve into placeholder tolerates millimetre rounding");
    require(!jointSeparated(placeholder,{100,20.0022,50}),"placeholder into next track tolerates millimetre rounding");
    require(jointSeparated(placeholder,{100.1,20,50}),"real gap beside zero-length subsection is still detected");
    // Terminal nodes have one reciprocal vector link; no spatial-neighbour
    // heuristic is used to manufacture a connection between intentional ends.
    require(reciprocalPin(2,0,0,1,1,{2,3},{1,1}) == 0,"normal start terminal is valid");
    require(reciprocalPin(3,0,0,0,1,{2,3},{1,1}) == 1,"normal finish terminal is valid");
    require(classify(isolated).components[0].singlePiece(1),"one TDB track object is standalone");
    require(!classify(isolated).components[0].singlePiece(2),"normal end piece attached to another object is not standalone");
    require(classify(longNetwork).components[0].singlePiece(1),"standalone single piece has no length limit");
    require(classify(siding).components[0].singlePiece(1),"single detached turnout can have internal junctions");
    require(!classify(unknown).components[0].singlePiece(1),"unknown TDB connectivity is not declared standalone");
    Geometry adjacency;
    require(adjacency.addPath(0,{{0,0,0},{0,0,10}}),"add first unregistered fixture");
    require(adjacency.addPath(1,{{0,0,10},{0,0,20}}),"add touching neighbour");
    require(adjacency.addPath(2,{{50,0,0},{50,0,1000}}),"add long standalone piece");
    require(adjacency.addPath(3,{{0,3,10},{0,3,20}}),"add vertically separate piece");
    std::set<int> connected;
    require(adjacency.endpointConnections(connected,[](std::size_t,std::size_t){return true;}),"endpoint scan completes");
    require(connected == std::set<int>({0,1}),"only end-to-end neighbours are connected");
    require(!adjacency.endpointConnections(connected,[](std::size_t,std::size_t){return false;}),"endpoint scan cancels");
    if(failures) return EXIT_FAILURE;
    std::cout << "Track overlap and connectivity policy checks passed.\n";
    return EXIT_SUCCESS;
}
