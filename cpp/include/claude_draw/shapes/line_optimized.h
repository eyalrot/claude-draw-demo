#pragma once

#include "shape_base_optimized.h"
#include "../core/transform2d.h"
#include <cmath>
#include <memory>
#include <algorithm>

namespace claude_draw {
namespace shapes {

/**
 * @brief Optimized Line class with 32-byte footprint
 * 
 * This class provides a full-featured line implementation while maintaining
 * the compact 32-byte memory layout for performance. The line is defined by
 * its two endpoints.
 */
class Line {
private:
    LineShape data_;
    
public:
    // Constructors
    Line() : data_() {}
    
    Line(float x1, float y1, float x2, float y2) 
        : data_(x1, y1, x2, y2) {}
    
    Line(const Point2D& start, const Point2D& end)
        : data_(start.x, start.y, end.x, end.y) {}
    
    // Copy and move constructors
    Line(const Line&) = default;
    Line(Line&&) = default;
    Line& operator=(const Line&) = default;
    Line& operator=(Line&&) = default;
    
    // Accessors
    float get_x1() const { return data_.x1; }
    float get_y1() const { return data_.y1; }
    float get_x2() const { return data_.x2; }
    float get_y2() const { return data_.y2; }
    
    Point2D get_start() const { return Point2D(data_.x1, data_.y1); }
    Point2D get_end() const { return Point2D(data_.x2, data_.y2); }
    
    // Setters
    void set_start(float x, float y) {
        data_.x1 = x;
        data_.y1 = y;
    }
    
    void set_start(const Point2D& point) {
        data_.x1 = point.x;
        data_.y1 = point.y;
    }
    
    void set_end(float x, float y) {
        data_.x2 = x;
        data_.y2 = y;
    }
    
    void set_end(const Point2D& point) {
        data_.x2 = point.x;
        data_.y2 = point.y;
    }
    
    void set_points(float x1, float y1, float x2, float y2) {
        data_.x1 = x1;
        data_.y1 = y1;
        data_.x2 = x2;
        data_.y2 = y2;
    }
    
    void set_points(const Point2D& start, const Point2D& end) {
        data_.x1 = start.x;
        data_.y1 = start.y;
        data_.x2 = end.x;
        data_.y2 = end.y;
    }
    
    // Style accessors (lines typically only have stroke, not fill)
    Color get_stroke_color() const {
        uint32_t color = data_.stroke_color;
        return Color(
            static_cast<uint8_t>(color & 0xFF),
            static_cast<uint8_t>((color >> 8) & 0xFF),
            static_cast<uint8_t>((color >> 16) & 0xFF),
            static_cast<uint8_t>((color >> 24) & 0xFF)
        );
    }
    
    float get_stroke_width() const { return data_.stroke_width; }
    
    // Style setters
    void set_stroke_color(const Color& color) {
        data_.set_stroke_color(color.r, color.g, color.b, color.a);
    }
    
    void set_stroke_width(float width) {
        data_.stroke_width = width;
    }
    
    // Type and flag management
    ShapeType get_type() const { return static_cast<ShapeType>(data_.type); }
    ShapeFlags get_flags() const { return static_cast<ShapeFlags>(data_.flags); }
    uint16_t get_id() const { return data_.id; }
    void set_id(uint16_t id) { data_.id = id; }
    
    bool is_visible() const { 
        return has_flag(static_cast<ShapeFlags>(data_.flags), ShapeFlags::Visible); 
    }
    
    void set_visible(bool visible) {
        if (visible) {
            data_.flags |= static_cast<uint8_t>(ShapeFlags::Visible);
        } else {
            data_.flags &= ~static_cast<uint8_t>(ShapeFlags::Visible);
        }
    }
    
    // Lines are always stroked, never filled
    bool is_stroked() const { return true; }
    bool is_filled() const { return false; }
    
    // Geometric operations
    BoundingBox get_bounds() const {
        return data_.bounds();
    }
    
    float length() const {
        float dx = data_.x2 - data_.x1;
        float dy = data_.y2 - data_.y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Get the midpoint of the line
    Point2D get_midpoint() const {
        return Point2D((data_.x1 + data_.x2) * 0.5f,
                      (data_.y1 + data_.y2) * 0.5f);
    }
    
    // Get normalized direction vector
    Point2D get_direction() const {
        float dx = data_.x2 - data_.x1;
        float dy = data_.y2 - data_.y1;
        float len = std::sqrt(dx * dx + dy * dy);
        if (len > 0) {
            return Point2D(dx / len, dy / len);
        }
        return Point2D(0, 0);
    }
    
    // Get normal vector (perpendicular to line)
    Point2D get_normal() const {
        Point2D dir = get_direction();
        return Point2D(-dir.y, dir.x);
    }
    
    // Angle in radians (from positive x-axis)
    float angle() const {
        return std::atan2(data_.y2 - data_.y1, data_.x2 - data_.x1);
    }
    
    // Distance from point to line segment
    float distance_to_point(const Point2D& point) const {
        return distance_to_point(point.x, point.y);
    }
    
    float distance_to_point(float px, float py) const {
        float dx = data_.x2 - data_.x1;
        float dy = data_.y2 - data_.y1;
        float t = ((px - data_.x1) * dx + (py - data_.y1) * dy) / (dx * dx + dy * dy);
        
        // Clamp t to [0, 1] to handle endpoints
        t = std::max(0.0f, std::min(1.0f, t));
        
        float nearest_x = data_.x1 + t * dx;
        float nearest_y = data_.y1 + t * dy;
        
        float dist_x = px - nearest_x;
        float dist_y = py - nearest_y;
        return std::sqrt(dist_x * dist_x + dist_y * dist_y);
    }
    
    // Check if point is on line (within tolerance)
    bool contains_point(const Point2D& point, float tolerance = 1.0f) const {
        return distance_to_point(point) <= tolerance;
    }
    
    bool contains_point(float x, float y, float tolerance = 1.0f) const {
        return distance_to_point(x, y) <= tolerance;
    }
    
    // Point along line at parameter t (0 = start, 1 = end)
    Point2D point_at(float t) const {
        return Point2D(
            data_.x1 + t * (data_.x2 - data_.x1),
            data_.y1 + t * (data_.y2 - data_.y1)
        );
    }
    
    // Transformation
    void transform(const Transform2D& transform) {
        Point2D start = transform.transform_point(get_start());
        Point2D end = transform.transform_point(get_end());
        data_.x1 = start.x;
        data_.y1 = start.y;
        data_.x2 = end.x;
        data_.y2 = end.y;
    }
    
    // Check if two lines intersect
    bool intersects_line(const Line& other, Point2D* intersection = nullptr) const {
        float x1 = data_.x1, y1 = data_.y1, x2 = data_.x2, y2 = data_.y2;
        float x3 = other.data_.x1, y3 = other.data_.y1, x4 = other.data_.x2, y4 = other.data_.y2;
        
        float denom = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
        
        // Lines are parallel
        if (std::abs(denom) < 1e-10f) {
            return false;
        }
        
        float t = ((x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4)) / denom;
        float u = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3)) / denom;
        
        if (t >= 0 && t <= 1 && u >= 0 && u <= 1) {
            if (intersection) {
                intersection->x = x1 + t * (x2 - x1);
                intersection->y = y1 + t * (y2 - y1);
            }
            return true;
        }
        
        return false;
    }
    
    // Clone operation
    std::unique_ptr<Line> clone() const {
        return std::make_unique<Line>(*this);
    }
    
    // Direct access to underlying data (for batch operations)
    const LineShape& data() const { return data_; }
    LineShape& data() { return data_; }
    
    // Static batch operations
    static void batch_transform(Line* lines, size_t count, const Transform2D& transform);
    static void batch_length(const Line* lines, size_t count, float* results);
    static void batch_intersects(const Line* lines1, const Line* lines2, 
                                size_t count, bool* results);
};

// Batch operation implementations
inline void Line::batch_transform(Line* lines, size_t count, const Transform2D& transform) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        lines[i].transform(transform);
    }
}

inline void Line::batch_length(const Line* lines, size_t count, float* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = lines[i].length();
    }
}

inline void Line::batch_intersects(const Line* lines1, const Line* lines2,
                                  size_t count, bool* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = lines1[i].intersects_line(lines2[i]);
    }
}

// Factory functions
inline std::unique_ptr<Line> make_line(float x1, float y1, float x2, float y2) {
    return std::make_unique<Line>(x1, y1, x2, y2);
}

inline std::unique_ptr<Line> make_line(const Point2D& start, const Point2D& end) {
    return std::make_unique<Line>(start, end);
}

// Type trait to identify this as a shape
template<>
struct is_shape<Line> : std::true_type {};

} // namespace shapes
} // namespace claude_draw