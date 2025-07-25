#pragma once

#include "claude_draw/shapes/shape_base.h"
#include <cmath>

namespace claude_draw {
namespace shapes {

/**
 * @brief Circle shape with optimized 32-byte data layout
 * Total size: 96 bytes (64 base + 32 data)
 * 
 * Data layout (32 bytes):
 * - 8 bytes: center point (2 floats)
 * - 4 bytes: radius
 * - 4 bytes: squared radius (cached for faster containment checks)
 * - 16 bytes: padding for alignment
 */
class Circle : public Shape<Circle> {
private:
    // Circle-specific data (32 bytes total)
    Point2D center_;           // 8 bytes
    float radius_;            // 4 bytes
    float radius_squared_;    // 4 bytes (cached)
    float padding_[4];        // 16 bytes padding
    
public:
    // Constructors
    Circle() : Circle(Point2D(0, 0), 0) {}
    
    Circle(const Point2D& center, float radius)
        : Shape(ShapeType::Circle)
        , center_(center)
        , radius_(radius)
        , radius_squared_(radius * radius) {
    }
    
    Circle(float cx, float cy, float radius)
        : Circle(Point2D(cx, cy), radius) {}
    
    // Accessors
    const Point2D& get_center() const { return center_; }
    float get_radius() const { return radius_; }
    float get_radius_squared() const { return radius_squared_; }
    
    // Modifiers
    void set_center(const Point2D& center) {
        center_ = center;
        invalidate_cache();
    }
    
    void set_center(float x, float y) {
        center_ = Point2D(x, y);
        invalidate_cache();
    }
    
    void set_radius(float radius) {
        radius_ = radius;
        radius_squared_ = radius * radius;
        invalidate_cache();
    }
    
    // Shape interface implementation
    BoundingBox calculate_bounds() const override {
        return BoundingBox(
            center_.x - radius_,
            center_.y - radius_,
            center_.x + radius_,
            center_.y + radius_
        );
    }
    
    bool contains_point(const Point2D& point) const override {
        // Use squared distance to avoid sqrt
        float dx = point.x - center_.x;
        float dy = point.y - center_.y;
        return (dx * dx + dy * dy) <= radius_squared_;
    }
    
    void transform(const Transform2D& transform) override {
        // Transform center
        center_ = transform.transform_point(center_);
        
        // For uniform scaling, we can transform the radius
        // For non-uniform scaling, circle becomes ellipse (not handled here)
        Point2D scale_point(radius_, 0);
        Point2D transformed_scale = transform.transform_vector(scale_point);
        radius_ = transformed_scale.magnitude();
        radius_squared_ = radius_ * radius_;
        
        invalidate_cache();
    }
    
    // Circle-specific methods
    float circumference() const {
        return 2.0f * M_PI * radius_;
    }
    
    float area() const {
        return M_PI * radius_squared_;
    }
    
    bool intersects_circle(const Circle& other) const {
        float distance_squared = center_.distance_squared_to(other.center_);
        float radii_sum = radius_ + other.radius_;
        return distance_squared <= (radii_sum * radii_sum);
    }
    
    // Fast batch operations support
    static void batch_transform(Circle* circles, size_t count, const Transform2D& transform) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            circles[i].transform(transform);
        }
    }
    
    static void batch_contains(const Circle* circles, size_t count, 
                              const Point2D* points, bool* results) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = circles[i].contains_point(points[i]);
        }
    }
};

/**
 * @brief Factory functions for creating circles
 */
inline std::unique_ptr<Circle> make_circle(const Point2D& center, float radius) {
    return std::make_unique<Circle>(center, radius);
}

inline std::unique_ptr<Circle> make_circle(float cx, float cy, float radius) {
    return std::make_unique<Circle>(cx, cy, radius);
}

} // namespace shapes
} // namespace claude_draw