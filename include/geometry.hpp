#pragma once
#include <vector>

namespace cad {

    struct Point2D {
        double x = 0.0;
        double y = 0.0;
    };

    struct Line {
        Point2D p1;
        Point2D p2;
    };

    struct Circle {
        Point2D center;
        double radius = 0.0;
    };

} // namespace cad