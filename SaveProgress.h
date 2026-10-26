// Optional synchronous progress sink; headless saves and probes need no UI.
// GNU GPL v3 or later.
#pragma once
#include <QString>
#include <functional>
#include <utility>
#include <algorithm>

namespace SaveProgress {
struct Position {
    double begin = 0;
    double end = 1;
    double value = 0;
};
inline Position &position() {
    static thread_local Position value;
    return value;
}
inline double fraction() {
    return position().value;
}
inline void set(double value) {
    auto &p = position();
    p.value = p.begin + std::clamp(value, 0.0, 1.0)*(p.end-p.begin);
}
// Nested work ranges keep row/file progress within its share of the whole save.
// Destruction restores the parent range without marking failed work complete.
class Range {
public:
    Range(double first, double last) : begin(position().begin), end(position().end) {
        position().begin = begin + first*(end-begin);
        position().end = begin + last*(end-begin);
        set(0);
    }
    ~Range() {
        position().begin = begin;
        position().end = end;
    }
    Range(const Range &) = delete;
    Range &operator=(const Range &) = delete;
private:
    double begin;
    double end;
};
using Callback = std::function<void(const QString &)>;
// Function-local storage avoids duplicate TLS initialization symbols emitted
// by MinGW for a dynamically initialized inline thread_local variable.
inline Callback &sink() {
    static thread_local Callback value;
    return value;
}
inline void report(const QString &message) {
    if(sink()) sink()(message);
}
class Scope {
public:
    explicit Scope(Callback next) : previous(std::move(sink())) {
        sink() = std::move(next);
    }
    ~Scope() { sink() = std::move(previous); }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
private:
    Callback previous;
};
}
