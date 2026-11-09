// TSRE GenX - opt-in startup diagnostics. Licensed under GNU GPL v3 or later.
#pragma once

#include <QDebug>
#include <QElapsedTimer>

class RouteLoadTiming {
public:
    static bool enabled() {
        static const bool value = qEnvironmentVariableIntValue("TSRE_PROFILE_ROUTE_LOAD") != 0;
        return value;
    }

    RouteLoadTiming() { if(enabled()) timer.start(); }

    void mark(const char *stage) {
        if(!enabled()) return;
        const qint64 now = timer.elapsed();
        qInfo() << "Route load timing:" << stage << "stage_ms=" << now - previous
                << "total_ms=" << now;
        previous = now;
    }

private:
    QElapsedTimer timer;
    qint64 previous = 0;
};
