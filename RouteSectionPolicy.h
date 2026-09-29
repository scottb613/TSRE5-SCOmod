// Route-local IDs are not required to start at the Global allocation boundary.
#ifndef ROUTESECTIONPOLICY_H
#define ROUTESECTIONPOLICY_H
#include <algorithm>
#include <set>
#include <vector>

namespace RouteSectionPolicy {
template<class Map>
bool occupied(const Map &definitions, int id){
    const auto found = definitions.find(id);
    return found != definitions.end() && found->second != nullptr;
}

template<class Map>
std::vector<int> savedIds(const Map &definitions, const std::set<int> &localIds,
                          int boundary, const std::set<int> &globalIds = {}){
    std::vector<int> result;
    for(const auto &entry : definitions)
        if(entry.second != nullptr
                && (localIds.count(entry.first)
                    || (entry.first >= boundary && !globalIds.count(entry.first))))
            result.push_back(entry.first);
    std::sort(result.begin(), result.end());
    return result;
}
}
#endif
