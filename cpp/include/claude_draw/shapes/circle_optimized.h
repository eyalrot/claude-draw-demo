#pragma once

#include "shape_base_optimized.h"
#include "shape_validation.h"
#include "../core/transform2d.h"
#include <cmath>
#include <memory>
#include <algorithm>

namespace claude_draw {
namespace shapes {

/**
 * @brief Optimized Circle class with 32-byte footprint
 * 
 * This class provides a full-featured circle implementation while maintaining
 * the compact 32-byte memory layout for performance.
 */
class Circle {
private:
    CircleShape data_;
    
public:
    // Constructors
    Circle() : data_() {}
    
    Circle(float x, float y, float radius) : data_(x, y, radius) {
        VALIDATE_IF_ENABLED(validate::is_finite(x), "Circle center x must be finite");
        VALIDATE_IF_ENABLED(validate::is_finite(y), "Circle center y must be finite");
        VALIDATE_IF_ENABLED(validate::is_finite(radius), "Circle radius must be finite");
        VALIDATE_IF_ENABLED(validate::is_non_negative(radius), "Circle radius must be non-negative");
    }
    
    Circle(const Point2D& center, float radius) 
        : data_(center.x, center.y, radius) {
        VALIDATE_IF_ENABLED(validate::is_finite(center.x), "Circle center x must be finite");
        VALIDATE_IF_ENABLED(validate::is_finite(center.y), "Circle center y must be finite");
        VALIDATE_IF_ENABLED(validate::is_finite(radius), "Circle radius must be finite");
        VALIDATE_IF_ENABLED(validate::is_non_negative(radius), "Circle radius must be non-negative");
    }
    
    // Copy and move constructors
    Circle(const Circle&) = default;
    Circle(Circle&&) = default;
    Circle& operator=(const Circle&) = default;
    Circle& operator=(Circle&&) = default;
    
    // Accessors
    float get_center_x() const { return data_.center_x; }
    float get_center_y() const { return data_.center_y; }
    Point2D get_center() const { return Point2D(data_.center_x, data_.center_y); }
    float get_radius() const { return data_.radius; }
    
    // Setters
    void set_center(float x, float y) {
        data_.center_x = x;
        data_.center_y = y;
    }
    
    void set_center(const Point2D& center) {
        data_.center_x = center.x;
        data_.center_y = center.y;
    }
    
    void set_radius(float radius) {
        data_.radius = radius;
    }
    
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
    ShapeType get_type() const { return data_.type; }
    ShapeFlags get_flags() const { return data_.flags; }
    uint32_t get_id() const { return data_.id; }
    void set_id(uint32_t id) { data_.id = id; }
    
    bool is_visible() const { 
        return has_flag(data_.flags, ShapeFlags::Visible); 
    }
    
    void set_visible(bool visible) {
        if (visible) {
            data_.flags = data_.flags | ShapeFlags::Visible;
        } else {
            data_.flags = static_cast<ShapeFlags>(
                static_cast<uint8_t>(data_.flags) & ~static_cast<uint8_t>(ShapeFlags::Visible)
            );
        }
    }
    
    bool is_filled() const {
        return has_flag(data_.flags, ShapeFlags::Filled);
    }
    
    void set_filled(bool filled) {
        if (filled) {
            data_.flags = data_.flags | ShapeFlags::Filled;
        } else {
            data_.flags = static_cast<ShapeFlags>(
                static_cast<uint8_t>(data_.flags) & ~static_cast<uint8_t>(ShapeFlags::Filled)
            );
        }
    }
    
    bool is_stroked() const {
        return has_flag(data_.flags, ShapeFlags::Stroked);
    }
    
    void set_stroked(bool stroked) {
        if (stroked) {
            data_.flags = data_.flags | ShapeFlags::Stroked;
        } else {
            data_.flags = static_cast<ShapeFlags>(
                static_cast<uint8_t>(data_.flags) & ~static_cast<uint8_t>(ShapeFlags::Stroked)
            );
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
    
    float circumference() const {
        return 2.0f * M_PI * data_.radius;
    }
    
    float area() const {
        return M_PI * data_.radius * data_.radius;
    }
    
    // Transformation
    void transform(const Transform2D& transform) {
        // Transform center
        Point2D center = transform.transform_point(Point2D(data_.center_x, data_.center_y));
        data_.center_x = center.x;
        data_.center_y = center.y;
        
        // For uniform scaling, transform the radius
        // Note: Non-uniform scaling would turn circle into ellipse
        Point2D scale_point(data_.radius, 0);
        Point2D transformed_scale = transform.transform_vector(scale_point);
        data_.radius = transformed_scale.magnitude();
    }
    
    // Circle-specific operations
    bool intersects_circle(const Circle& other) const {
        float dx = other.data_.center_x - data_.center_x;
        float dy = other.data_.center_y - data_.center_y;
        float distance_squared = dx * dx + dy * dy;
        float radii_sum = data_.radius + other.data_.radius;
        return distance_squared <= (radii_sum * radii_sum);
    }
    
    float distance_to_edge(const Point2D& point) const {
        float dx = point.x - data_.center_x;
        float dy = point.y - data_.center_y;
        float distance_to_center = std::sqrt(dx * dx + dy * dy);
        return std::abs(distance_to_center - data_.radius);
    }
    
    // Clone operation
    std::unique_ptr<Circle> clone() const {
        return std::make_unique<Circle>(*this);
    }
    
    // Validation
    bool is_valid() const {
        return validate::is_finite(data_.center_x) &&
               validate::is_finite(data_.center_y) &&
               validate::is_finite(data_.radius) &&
               validate::is_non_negative(data_.radius);
    }
    
    // Direct access to underlying data (for batch operations)
    const CircleShape& data() const { return data_; }
    CircleShape& data() { return data_; }
    
    // Static batch operations
    static void batch_transform(Circle* circles, size_t count, const Transform2D& transform);
    static void batch_contains(const Circle* circles, size_t count, 
                              const Point2D* points, bool* results);
    static void batch_intersects(const Circle* circles1, const Circle* circles2,
                                size_t count, bool* results);
};

// Batch operation implementations
inline void Circle::batch_transform(Circle* circles, size_t count, const Transform2D& transform) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        circles[i].transform(transform);
    }
}

inline void Circle::batch_contains(const Circle* circles, size_t count, 
                                  const Point2D* points, bool* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = circles[i].contains_point(points[i]);
    }
}

inline void Circle::batch_intersects(const Circle* circles1, const Circle* circles2,
                                   size_t count, bool* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = circles1[i].intersects_circle(circles2[i]);
    }
}

// Factory functions
inline std::unique_ptr<Circle> make_circle(float x, float y, float radius) {
    return std::make_unique<Circle>(x, y, radius);
}

inline std::unique_ptr<Circle> make_circle(const Point2D& center, float radius) {
    return std::make_unique<Circle>(center, radius);
}

// Type trait to identify this as a shape
template<>
struct is_shape<Circle> : std::true_type {};

} // namespace shapes
} // namespace claude_draw