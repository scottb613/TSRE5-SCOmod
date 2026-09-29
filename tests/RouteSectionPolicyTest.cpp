#include "RouteSectionPolicy.h"
#include <cstdlib>
#include <iostream>
#include <unordered_map>

int main(){
    int definition = 1;
    std::unordered_map<int, int*> definitions{{5, &definition}, {41000, &definition}};
    int failures = 0;
    const auto require = [&](bool ok, const char *message){
        if(!ok){ ++failures; std::cerr << message << '\n'; }
    };
    require(!RouteSectionPolicy::occupied(definitions, 700),
            "Unused legacy ID below allocation boundary must not conflict");
    require(RouteSectionPolicy::occupied(definitions, 5), "Occupied low ID must conflict");
    require(RouteSectionPolicy::occupied(definitions, 41000), "Occupied high ID must conflict");
    definitions[701] = nullptr;
    require(!RouteSectionPolicy::occupied(definitions, 701), "Lookup placeholder is not a definition");
    definitions[700] = &definition;
    definitions[703] = &definition;
    definitions[40000] = &definition;
    const std::set<int> local{700, 703};
    const auto saved = RouteSectionPolicy::savedIds(definitions, local, 40000);
    require(saved == std::vector<int>({700,703,40000,41000}),
            "Saving must retain low route IDs and newly allocated IDs in ascending order");
    require(std::find(saved.begin(), saved.end(), 5) == saved.end(),
            "Saving must not copy unrelated low Global definitions");
    require(RouteSectionPolicy::savedIds(definitions, local, 40000, {5,41000})
                == std::vector<int>({700,703,40000}),
            "Global definitions above the declared boundary must not leak into the route file");
    std::unordered_map<int, int*> reloaded{{5, &definition}};
    std::set<int> reloadedLocal;
    for(int id : saved){
        reloaded[id] = &definition;
        reloadedLocal.insert(id);
    }
    require(RouteSectionPolicy::savedIds(reloaded, reloadedLocal, 40000) == saved,
            "A second save must preserve the same ID inventory");
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
