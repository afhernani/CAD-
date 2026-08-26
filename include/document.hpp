#pragma once
#include "geometry.hpp"
#include <vector>

namespace cad {

    class Document {
    public:
        std::vector<Line> lines;
        std::vector<Circle> circles;

        void clear() {
            lines.clear();
            circles.clear();
        }
    };

} // namespace cad