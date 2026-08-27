#include "TrackDbFormatValidator.h"

#include <cmath>

namespace {
bool reject(QString *error, const QString &message){
    if(error)
        *error = message;
    return false;
}
}

bool TrackDbFormatValidator::integer(float value, int minimum, int maximum,
                                     int &result, QString *error){
    const double numeric = double(value);
    if(!std::isfinite(value) || std::trunc(value) != value
            || numeric < double(minimum) || numeric > double(maximum))
        return reject(error, "Track database value is not a bounded integer.");
    result = int(value);
    return true;
}

bool TrackDbFormatValidator::nodeCount(int count, QString *error){
    if(count < 0 || count > MaximumNodes)
        return reject(error, "Track-node count is negative or excessive.");
    return true;
}

bool TrackDbFormatValidator::nodeId(int id, int declaredCount, QString *error){
    if(id <= 0 || id > declaredCount)
        return reject(error, "Track-node ID is outside the declared node range.");
    return true;
}

bool TrackDbFormatValidator::sectionCount(int count, qint64 currentTotal,
                                          QString *error){
    if(count < 0 || count > MaximumSectionsPerNode
            || currentTotal < 0
            || count > MaximumSectionsTotal - currentTotal)
        return reject(error, "Vector-section count is negative or excessive.");
    return true;
}

bool TrackDbFormatValidator::itemRefCount(int count, qint64 currentTotal,
                                          QString *error){
    if(count < 0 || count > MaximumItemRefsPerNode
            || currentTotal < 0
            || count > MaximumItemRefsTotal - currentTotal)
        return reject(error, "Track-item reference count is negative or excessive.");
    return true;
}

bool TrackDbFormatValidator::pinCounts(int nodeType, int first, int second,
                                       QString *error){
    const bool valid = (nodeType == 0 && first == 1 && second == 0)
        || (nodeType == 1 && first == 1 && second == 1)
        || (nodeType == 2 && first == 1 && second == 2);
    if(!valid)
        return reject(error, "Track-node pin counts do not match its node type.");
    return true;
}

bool TrackDbFormatValidator::pin(int targetNode, int direction, int maximumNode,
                                 QString *error){
    if(targetNode < 0 || targetNode > maximumNode)
        return reject(error, "Track pin references an out-of-range node.");
    if(direction != 0 && direction != 1)
        return reject(error, "Track pin direction must be zero or one.");
    return true;
}
