// TSRE GenX - maintained editor source and regression support.
// TSRE GenX modifications Copyright (C) Scott Brunner, Beast of Burden.
// Based on TSRE5 by Piotr Gadecki and TSRE 8.x by Eric Olesen.
// Part of the TSRE GenX route-editor application.
// Licensed under GNU GPL v3 or later. See LICENSE.md.

#include "TSREvcWIP/TrackDbFormatValidator.h"

#include <QDebug>
#include <limits>

namespace {
bool expect(bool condition, const char *message){
    if(!condition)
        qCritical() << message;
    return condition;
}
}

int main(){
    QString error;
    bool ok = true;
    int integer = 0;
    ok &= expect(TrackDbFormatValidator::integer(42.0f, 0, 100, integer, &error)
              && integer == 42, "bounded integer rejected");
    ok &= expect(!TrackDbFormatValidator::integer(1.5f, 0, 100, integer, &error),
                 "fractional count accepted");
    ok &= expect(!TrackDbFormatValidator::integer(
        std::numeric_limits<float>::infinity(), 0, 100, integer, &error),
        "non-finite count accepted");
    ok &= expect(TrackDbFormatValidator::nodeCount(5000, &error),
                 "normal node count rejected");
    ok &= expect(!TrackDbFormatValidator::nodeCount(-1, &error),
                 "negative node count accepted");
    ok &= expect(!TrackDbFormatValidator::nodeCount(
        TrackDbFormatValidator::MaximumNodes + 1, &error),
        "excessive node count accepted");
    ok &= expect(!TrackDbFormatValidator::nodeId(0, 10, &error),
                 "zero node ID accepted");
    ok &= expect(!TrackDbFormatValidator::sectionCount(-1, 0, &error),
                 "negative section count accepted");
    ok &= expect(!TrackDbFormatValidator::sectionCount(1,
        TrackDbFormatValidator::MaximumSectionsTotal, &error),
        "aggregate section overflow accepted");
    ok &= expect(!TrackDbFormatValidator::itemRefCount(1,
        TrackDbFormatValidator::MaximumItemRefsTotal, &error),
        "aggregate item-reference overflow accepted");
    ok &= expect(TrackDbFormatValidator::pinCounts(0, 1, 0, &error)
              && TrackDbFormatValidator::pinCounts(1, 1, 1, &error)
              && TrackDbFormatValidator::pinCounts(2, 1, 2, &error),
                 "legal pin counts rejected");
    ok &= expect(!TrackDbFormatValidator::pinCounts(2, 3, 3, &error),
                 "overflowing pin counts accepted");
    ok &= expect(!TrackDbFormatValidator::pin(11, 0, 10, &error),
                 "out-of-range pin target accepted");
    ok &= expect(!TrackDbFormatValidator::pin(1, 2, 10, &error),
                 "invalid pin direction accepted");
    return ok ? 0 : 1;
}
