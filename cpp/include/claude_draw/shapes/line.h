#pragma once

#include "claude_draw/shapes/shape_base.h"
#include <cmath>
#include <algorithm>
#include <optional>

namespace claude_draw {
namespace shapes {

/**
 * @brief Line shape with minimal overhead
 * Total size: 96 bytes (64 base + 32 data)
 * 
 * Data layout (32 bytes):
 * - 16 bytes: start and end points
 * - 8 bytes: cached length and squared length
 * - 8 bytes: cached direction vector (normalized)
 */
class alignas(32) Line : public Shape<Line> {
private:
    // Line-specific data (32 bytes total)
    Point2D start_;           // 8 bytes
    Point2D end_;             // 8 bytes
    float length_;           // 4 bytes (cached)
    float length_squared_;   // 4 bytes (cached)
    Point2D direction_;      // 8 bytes (cached normalized direction)
    
    void update_cache() {
        float dx = end_.x - start_.x;
        float dy = end_.y - start_.y;
        length_squared_ = dx * dx + dy * dy;
        length_ = std::sqrt(length_squared_);
        
        if (length_ > std::numeric_limits<float>::epsilon()) {
            direction_ = Point2D(dx / length_, dy / length_);
        } else {
            direction_ = Point2D(0, 0);
        }
    }
    
public:
    // Constructors
    Line() : Line(Point2D(0, 0), Point2D(0, 0)) {}
    
    Line(const Point2D& start, const Point2D& end)
        : Shape(ShapeType::Line)
        , start_(start)
        , end_(end) {
        // Lines are stroked by default, not filled
        remove_flags(ShapeFlags::Filled);
        add_flags(ShapeFlags::Stroked);
        update_cache();
        static_assert(sizeof(Line) == 96, "Line must be exactly 96 bytes");
    }
    
    Line(float x1, float y1, float x2, float y2)
        : Line(Point2D(x1, y1), Point2D(x2, y2)) {}
    
    // Accessors
    const Point2D& get_start() const { return start_; }
    const Point2D& get_end() const { return end_; }
    float get_length() const { return length_; }
    float get_length_squared() const { return length_squared_; }
    const Point2D& get_direction() const { return direction_; }
    
    Point2D get_midpoint() const {
        return Point2D(
            (start_.x + end_.x) * 0.5f,
            (start_.y + end_.y) * 0.5f
        );
    }
    
    // Modifiers
    void set_start(const Point2D& start) {
        start_ = start;
        update_cache();
        invalidate_cache();
    }
    
    void set_end(const Point2D& end) {
        end_ = end;
        update_cache();
        invalidate_cache();
    }
    
    void set_points(const Point2D& start, const Point2D& end) {
        start_ = start;
        end_ = end;
        update_cache();
        invalidate_cache();
    }
    
    // Shape interface implementation
    BoundingBox calculate_bounds() const override {
        return BoundingBox(
            std::min(start_.x, end_.x),
            std::min(start_.y, end_.y),
            std::max(start_.x, end_.x),
            std::max(start_.y, end_.y)
        );
    }
    
    bool contains_point(const Point2D& point) const override {
        // For lines, we check if point is within a tolerance distance
        float tolerance = style_.stroke_width * 0.5f;
        return distance_to_point(point) <= tolerance;
    }
    
    void transform(const Transform2D& transform) override {
        start_ = transform.transform_point(start_);
        end_ = transform.transform_point(end_);
        update_cache();
        invalidate_cache();
    }
    
    // Line-specific methods
    float slope() const {
        float dx = end_.x - start_.x;
        if (std::abs(dx) < std::numeric_limits<float>::epsilon()) {
            return std::numeric_limits<float>::infinity();
        }
        return (end_.y - start_.y) / dx;
    }
    
    float angle() const {
        return std::atan2(end_.y - start_.y, end_.x - start_.x);
    }
    
    // Distance from point to line segment
    float distance_to_point(const Point2D& point) const {
        if (length_squared_ < std::numeric_limits<float>::epsilon()) {
            // Degenerate line (point)
            return point.distance_to(start_);
        }
        
        // Project point onto line
        float t = ((point.x - start_.x) * (end_.x - start_.x) + 
                  (point.y - start_.y) * (end_.y - start_.y)) / length_squared_;
        
        // Clamp to line segment
        t = std::clamp(t, 0.0f, 1.0f);
        
        // Find closest point on line
        Point2D closest(
            start_.x + t * (end_.x - start_.x),
            start_.y + t * (end_.y - start_.y)
        );
        
        return point.distance_to(closest);
    }
    
    // Get point at parameter t (0 = start, 1 = end)
    Point2D point_at(float t) const {
        return Point2D(
            start_.x + t * (end_.x - start_.x),
            start_.y + t * (end_.y - start_.y)
        );
    }
    
    // Check if two line segments intersect
    bool intersects_line(const Line& other) const {
        // Using cross product method
        auto ccw = [](const Point2D& a, const Point2D& b, const Point2D& c) {
            return (c.y - a.y) * (b.x - a.x) > (b.y - a.y) * (c.x - a.x);
        };
        
        return ccw(start_, other.start_, other.end_) != ccw(end_, other.start_, other.end_) &&
               ccw(start_, end_, other.start_) != ccw(start_, end_, other.end_);
    }
    
    // Get intersection point if lines intersect
    std::optional<Point2D> intersection_point(const Line& other) const {
        float x1 = start_.x, y1 = start_.y;
        float x2 = end_.x, y2 = end_.y;
        float x3 = other.start_.x, y3 = other.start_.y;
        float x4 = other.end_.x, y4 = other.end_.y;
        
        float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        
        if (std::abs(denom) < std::numeric_limits<float>::epsilon()) {
            return std::nullopt;  // Parallel lines
        }
        
        float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;
        
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            return Point2D(x1 + t * (x2 - x1), y1 + t * (y2 - y1));
        }
        
        return std::nullopt;  // Lines don't intersect within segments
    }
    
    // Fast batch operations support
    static void batch_transform(Line* lines, size_t count, const Transform2D& transform) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            lines[i].transform(transform);
        }
    }
    
    static void batch_length(const Line* lines, size_t count, float* lengths) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            lengths[i] = lines[i].get_length();
        }
    }
    
    static void batch_midpoint(const Line* lines, size_t count, Point2D* midpoints) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            midpoints[i] = lines[i].get_midpoint();
        }
    }
};

/**
 * @brief Factory functions for creating lines
 */
inline std::unique_ptr<Line> make_line(const Point2D& start, const Point2D& end) {
    return std::make_unique<Line>(start, end);
}

inline std::unique_ptr<Line> make_line(float x1, float y1, float x2, float y2) {
    return std::make_unique<Line>(x1, y1, x2, y2);
}

} // namespace shapes
} // namespace claude_draw