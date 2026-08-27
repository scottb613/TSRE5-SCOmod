#ifndef TRACKDBFORMATVALIDATOR_H
#define TRACKDBFORMATVALIDATOR_H

#include <QString>
#include <QtGlobal>

class TrackDbFormatValidator {
public:
    static constexpr int MaximumNodes = 1000000;
    static constexpr int MaximumSectionsPerNode = 100000;
    static constexpr qint64 MaximumSectionsTotal = 2000000;
    static constexpr int MaximumItemRefsPerNode = 1000000;
    static constexpr qint64 MaximumItemRefsTotal = 8000000;

    static bool integer(float value, int minimum, int maximum, int &result,
                        QString *error = NULL);
    static bool nodeCount(int count, QString *error = NULL);
    static bool nodeId(int id, int declaredCount, QString *error = NULL);
    static bool sectionCount(int count, qint64 currentTotal,
                             QString *error = NULL);
    static bool itemRefCount(int count, qint64 currentTotal,
                             QString *error = NULL);
    static bool pinCounts(int nodeType, int first, int second,
                          QString *error = NULL);
    static bool pin(int targetNode, int direction, int maximumNode,
                    QString *error = NULL);
};

#endif
