#pragma once

#include "shape_validation.h"
#include "circle_optimized.h"
#include "rectangle_optimized.h"
#include "ellipse_optimized.h"
#include "line_optimized.h"

namespace claude_draw {
namespace shapes {
namespace unchecked {

// Specialized unchecked creation functions
inline Circle create_circle(float cx, float cy, float radius) {
    ValidationScope scope(ValidationMode::None);
    return Circle(cx, cy, radius);
}

inline Rectangle create_rectangle(float x, float y, float width, float height) {
    ValidationScope scope(ValidationMode::None);
    return Rectangle(x, y, width, height);
}

inline Ellipse create_ellipse(float cx, float cy, float rx, float ry) {
    ValidationScope scope(ValidationMode::None);
    return Ellipse(cx, cy, rx, ry);
}

inline Line create_line(float x1, float y1, float x2, float y2) {
    ValidationScope scope(ValidationMode::None);
    return Line(x1, y1, x2, y2);
}

} // namespace unchecked
} // namespace shapes
} // namespace claude_draw