#pragma once

#include "claude_draw/shapes/shape_base.h"
#include <cmath>

namespace claude_draw {
namespace shapes {

/**
 * @brief Ellipse shape extending Circle concept with separate x/y radii
 * Total size: 96 bytes (64 base + 32 data)
 * 
 * Data layout (32 bytes):
 * - 8 bytes: center point
 * - 4 bytes: x radius
 * - 4 bytes: y radius
 * - 4 bytes: rotation angle (radians)
 * - 12 bytes: cached values and padding
 */
class alignas(32) Ellipse : public Shape<Ellipse> {
private:
    // Ellipse-specific data (32 bytes total)
    Point2D center_;          // 8 bytes
    float rx_;               // 4 bytes (x radius)
    float ry_;               // 4 bytes (y radius)
    float angle_;            // 4 bytes (rotation in radians)
    float cos_angle_;        // 4 bytes (cached)
    float sin_angle_;        // 4 bytes (cached)
    float padding_;          // 4 bytes padding
    
    void update_angle_cache() {
        cos_angle_ = std::cos(angle_);
        sin_angle_ = std::sin(angle_);
    }
    
public:
    // Constructors
    Ellipse() : Ellipse(Point2D(0, 0), 0, 0, 0) {}
    
    Ellipse(const Point2D& center, float rx, float ry, float angle = 0)
        : Shape(ShapeType::Ellipse)
        , center_(center)
        , rx_(rx)
        , ry_(ry)
        , angle_(angle) {
        update_angle_cache();
        static_assert(sizeof(Ellipse) == 96, "Ellipse must be exactly 96 bytes");
    }
    
    Ellipse(float cx, float cy, float rx, float ry, float angle = 0)
        : Ellipse(Point2D(cx, cy), rx, ry, angle) {}
    
    // Create from circle (special case)
    static Ellipse from_circle(const Point2D& center, float radius) {
        return Ellipse(center, radius, radius, 0);
    }
    
    // Accessors
    const Point2D& get_center() const { return center_; }
    float get_rx() const { return rx_; }
    float get_ry() const { return ry_; }
    float get_angle() const { return angle_; }
    float get_cos_angle() const { return cos_angle_; }
    float get_sin_angle() const { return sin_angle_; }
    
    // Check if this is actually a circle
    bool is_circle() const {
        return std::abs(rx_ - ry_) < std::numeric_limits<float>::epsilon();
    }
    
    // Modifiers
    void set_center(const Point2D& center) {
        center_ = center;
        invalidate_cache();
    }
    
    void set_center(float x, float y) {
        center_ = Point2D(x, y);
        invalidate_cache();
    }
    
    void set_radii(float rx, float ry) {
        rx_ = rx;
        ry_ = ry;
        invalidate_cache();
    }
    
    void set_angle(float angle) {
        angle_ = angle;
        update_angle_cache();
        invalidate_cache();
    }
    
    // Shape interface implementation
    BoundingBox calculate_bounds() const override {
        if (angle_ == 0) {
            // Fast path for axis-aligned ellipse
            return BoundingBox(
                center_.x - rx_,
                center_.y - ry_,
                center_.x + rx_,
                center_.y + ry_
            );
        }
        
        // For rotated ellipse, calculate the axis-aligned bounding box
        float a = rx_ * cos_angle_;
        float b = rx_ * sin_angle_;
        float c = ry_ * -sin_angle_;
        float d = ry_ * cos_angle_;
        
        float half_width = std::sqrt(a * a + c * c);
        float half_height = std::sqrt(b * b + d * d);
        
        return BoundingBox(
            center_.x - half_width,
            center_.y - half_height,
            center_.x + half_width,
            center_.y + half_height
        );
    }
    
    bool contains_point(const Point2D& point) const override {
        // Transform point to ellipse-local coordinates
        float dx = point.x - center_.x;
        float dy = point.y - center_.y;
        
        if (angle_ != 0) {
            // Rotate point by -angle to align with ellipse axes
            float local_x = dx * cos_angle_ + dy * sin_angle_;
            float local_y = -dx * sin_angle_ + dy * cos_angle_;
            dx = local_x;
            dy = local_y;
        }
        
        // Check if point is inside ellipse using standard form
        float term_x = (dx * dx) / (rx_ * rx_);
        float term_y = (dy * dy) / (ry_ * ry_);
        
        return (term_x + term_y) <= 1.0f;
    }
    
    void transform(const Transform2D& transform) override {
        // Transform center
        center_ = transform.transform_point(center_);
        
        // For general transforms, we need to handle rotation and non-uniform scaling
        // This is a simplified version that handles uniform scaling and rotation
        Point2D rx_vec(rx_ * cos_angle_, rx_ * sin_angle_);
        Point2D ry_vec(-ry_ * sin_angle_, ry_ * cos_angle_);
        
        rx_vec = transform.transform_vector(rx_vec);
        ry_vec = transform.transform_vector(ry_vec);
        
        // Extract new radii and angle
        rx_ = rx_vec.magnitude();
        ry_ = ry_vec.magnitude();
        angle_ = std::atan2(rx_vec.y, rx_vec.x);
        
        update_angle_cache();
        invalidate_cache();
    }
    
    // Ellipse-specific methods
    float area() const {
        return M_PI * rx_ * ry_;
    }
    
    // Approximate perimeter using Ramanujan's formula
    float perimeter() const {
        float a = rx_;
        float b = ry_;
        float h = ((a - b) * (a - b)) / ((a + b) * (a + b));
        return M_PI * (a + b) * (1.0f + (3.0f * h) / (10.0f + std::sqrt(4.0f - 3.0f * h)));
    }
    
    // Get focus points
    std::pair<Point2D, Point2D> get_foci() const {
        float c = std::sqrt(std::abs(rx_ * rx_ - ry_ * ry_));
        
        Point2D f1, f2;
        if (rx_ > ry_) {
            // Foci on x-axis
            f1 = Point2D(center_.x - c, center_.y);
            f2 = Point2D(center_.x + c, center_.y);
        } else {
            // Foci on y-axis
            f1 = Point2D(center_.x, center_.y - c);
            f2 = Point2D(center_.x, center_.y + c);
        }
        
        // Apply rotation if needed
        if (angle_ != 0) {
            Transform2D rot = Transform2D::rotate(angle_);
            rot.translate_by(center_.x, center_.y);
            f1 = rot.transform_point(f1 - center_);
            f2 = rot.transform_point(f2 - center_);
        }
        
        return {f1, f2};
    }
    
    // Fast batch operations support
    static void batch_transform(Ellipse* ellipses, size_t count, const Transform2D& transform) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            ellipses[i].transform(transform);
        }
    }
    
    static void batch_contains(const Ellipse* ellipses, size_t count, 
                              const Point2D* points, bool* results) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = ellipses[i].contains_point(points[i]);
        }
    }
};

/**
 * @brief Factory functions for creating ellipses
 */
inline std::unique_ptr<Ellipse> make_ellipse(const Point2D& center, float rx, float ry, float angle = 0) {
    return std::make_unique<Ellipse>(center, rx, ry, angle);
}

inline std::unique_ptr<Ellipse> make_ellipse(float cx, float cy, float rx, float ry, float angle = 0) {
    return std::make_unique<Ellipse>(cx, cy, rx, ry, angle);
}

} // namespace shapes
} // namespace claude_draw