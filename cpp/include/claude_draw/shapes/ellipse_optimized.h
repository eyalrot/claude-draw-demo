#pragma once

#include "shape_base_optimized.h"
#include "../core/transform2d.h"
#include <cmath>
#include <memory>
#include <algorithm>

namespace claude_draw {
namespace shapes {

/**
 * @brief Optimized Ellipse class with 32-byte footprint
 * 
 * This class provides a full-featured ellipse implementation while maintaining
 * the compact 32-byte memory layout for performance. The ellipse is defined by
 * its center point and two radii (horizontal and vertical).
 */
class Ellipse {
private:
    EllipseShape data_;
    
public:
    // Constructors
    Ellipse() : data_() {}
    
    Ellipse(float center_x, float center_y, float radius_x, float radius_y) 
        : data_(center_x, center_y, radius_x, radius_y) {}
    
    Ellipse(const Point2D& center, float radius_x, float radius_y)
        : data_(center.x, center.y, radius_x, radius_y) {}
    
    // Copy and move constructors
    Ellipse(const Ellipse&) = default;
    Ellipse(Ellipse&&) = default;
    Ellipse& operator=(const Ellipse&) = default;
    Ellipse& operator=(Ellipse&&) = default;
    
    // Accessors
    float get_center_x() const { return data_.center_x; }
    float get_center_y() const { return data_.center_y; }
    float get_radius_x() const { return data_.radius_x; }
    float get_radius_y() const { return data_.radius_y; }
    
    Point2D get_center() const { return Point2D(data_.center_x, data_.center_y); }
    
    // Setters
    void set_center(float x, float y) {
        data_.center_x = x;
        data_.center_y = y;
    }
    
    void set_center(const Point2D& center) {
        data_.center_x = center.x;
        data_.center_y = center.y;
    }
    
    void set_radii(float rx, float ry) {
        data_.radius_x = rx;
        data_.radius_y = ry;
    }
    
    void set_radius_x(float rx) { data_.radius_x = rx; }
    void set_radius_y(float ry) { data_.radius_y = ry; }
    
    // Style accessors
    Color get_fill_color() const {
        uint32_t color = data_.fill_color;
        return Color(
            static_cast<uint8_t>(color & 0xFF),
            static_cast<uint8_t>((color >> 8) & 0xFF),
            static_cast<uint8_t>((color >> 16) & 0xFF),
            static_cast<uint8_t>((color >> 24) & 0xFF)
        );
    }
    
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
    void set_fill_color(const Color& color) {
        data_.set_fill_color(color.r, color.g, color.b, color.a);
    }
    
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
    
    bool is_filled() const {
        return has_flag(static_cast<ShapeFlags>(data_.flags), ShapeFlags::Filled);
    }
    
    void set_filled(bool filled) {
        if (filled) {
            data_.flags |= static_cast<uint8_t>(ShapeFlags::Filled);
        } else {
            data_.flags &= ~static_cast<uint8_t>(ShapeFlags::Filled);
        }
    }
    
    bool is_stroked() const {
        return has_flag(static_cast<ShapeFlags>(data_.flags), ShapeFlags::Stroked);
    }
    
    void set_stroked(bool stroked) {
        if (stroked) {
            data_.flags |= static_cast<uint8_t>(ShapeFlags::Stroked);
        } else {
            data_.flags &= ~static_cast<uint8_t>(ShapeFlags::Stroked);
        }
    }
    
    // Geometric operations
    BoundingBox get_bounds() const {
        return data_.bounds();
    }
    
    bool contains_point(const Point2D& point) const {
        return data_.contains(point.x, point.y);
    }
    
    bool contains_point(float x, float y) const {
        return data_.contains(x, y);
    }
    
    float area() const {
        return M_PI * data_.radius_x * data_.radius_y;
    }
    
    // Approximation of ellipse perimeter using Ramanujan's formula
    float perimeter() const {
        float a = data_.radius_x;
        float b = data_.radius_y;
        float h = ((a - b) * (a - b)) / ((a + b) * (a + b));
        return M_PI * (a + b) * (1.0f + (3.0f * h) / (10.0f + std::sqrt(4.0f - 3.0f * h)));
    }
    
    // Transformation
    void transform(const Transform2D& transform) {
        // Transform center
        Point2D new_center = transform.transform_point(get_center());
        data_.center_x = new_center.x;
        data_.center_y = new_center.y;
        
        // For scaling, we need to handle non-uniform scaling
        // This is an approximation that works for uniform scaling and rotation
        // For general affine transforms, the ellipse may become a rotated ellipse
        float tx, ty, rotation, sx, sy;
        transform.decompose(tx, ty, rotation, sx, sy);
        
        data_.radius_x *= std::abs(sx);
        data_.radius_y *= std::abs(sy);
    }
    
    // Check if this ellipse is actually a circle
    bool is_circle() const {
        return std::abs(data_.radius_x - data_.radius_y) < 1e-6f;
    }
    
    // Convert to circle if radii are equal
    float get_average_radius() const {
        return (data_.radius_x + data_.radius_y) * 0.5f;
    }
    
    // Distance from center to edge in a given direction (angle in radians)
    float radius_at_angle(float angle) const {
        float cos_angle = std::cos(angle);
        float sin_angle = std::sin(angle);
        float rx2 = data_.radius_x * data_.radius_x;
        float ry2 = data_.radius_y * data_.radius_y;
        return (data_.radius_x * data_.radius_y) / 
               std::sqrt(ry2 * cos_angle * cos_angle + rx2 * sin_angle * sin_angle);
    }
    
    // Point on ellipse at given angle
    Point2D point_at_angle(float angle) const {
        return Point2D(
            data_.center_x + data_.radius_x * std::cos(angle),
            data_.center_y + data_.radius_y * std::sin(angle)
        );
    }
    
    // Clone operation
    std::unique_ptr<Ellipse> clone() const {
        return std::make_unique<Ellipse>(*this);
    }
    
    // Direct access to underlying data (for batch operations)
    const EllipseShape& data() const { return data_; }
    EllipseShape& data() { return data_; }
    
    // Static batch operations
    static void batch_transform(Ellipse* ellipses, size_t count, const Transform2D& transform);
    static void batch_contains(const Ellipse* ellipses, size_t count, 
                              const Point2D* points, bool* results);
    static void batch_area(const Ellipse* ellipses, size_t count, float* results);
};

// Batch operation implementations
inline void Ellipse::batch_transform(Ellipse* ellipses, size_t count, const Transform2D& transform) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        ellipses[i].transform(transform);
    }
}

inline void Ellipse::batch_contains(const Ellipse* ellipses, size_t count, 
                                   const Point2D* points, bool* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = ellipses[i].contains_point(points[i]);
    }
}

inline void Ellipse::batch_area(const Ellipse* ellipses, size_t count, float* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = ellipses[i].area();
    }
}

// Factory functions
inline std::unique_ptr<Ellipse> make_ellipse(float cx, float cy, float rx, float ry) {
    return std::make_unique<Ellipse>(cx, cy, rx, ry);
}

inline std::unique_ptr<Ellipse> make_ellipse(const Point2D& center, float rx, float ry) {
    return std::make_unique<Ellipse>(center, rx, ry);
}

// Convenience function to create a circle using the ellipse class
inline std::unique_ptr<Ellipse> make_circle_as_ellipse(float cx, float cy, float radius) {
    return std::make_unique<Ellipse>(cx, cy, radius, radius);
}

// Type trait to identify this as a shape
template<>
struct is_shape<Ellipse> : std::true_type {};

} // namespace shapes
} // namespace claude_draw