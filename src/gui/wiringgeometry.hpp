#pragma once
#include <QLineF>
#include <algorithm>
#include <cmath>
// Geometric overlap does not imply electrical contact. Explicit endpoints do.
inline bool overlappingSegments(const QLineF &a, const QLineF &b) {
    const QPointF d = a.p2() - a.p1(), v = b.p1() - a.p1(), w = b.p2() - a.p1();
    const auto cross = [](QPointF x, QPointF y) { return x.x() * y.y() - x.y() * y.x(); };
    const double length = std::hypot(d.x(), d.y());
    if (length < .01)
        return false;
    if (std::abs(cross(d, v)) / length > .25 || std::abs(cross(d, w)) / length > .25)
        return false;
    auto projection = [&](QPointF x) { return (x.x() * d.x() + x.y() * d.y()) / length; };
    auto lo = std::min(projection(v), projection(w)), hi = std::max(projection(v), projection(w));
    return std::min(length, hi) - std::max(0., lo) > .5;
}
