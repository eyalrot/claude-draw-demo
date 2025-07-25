#pragma once

#include "claude_draw/shapes/shape_base.h"
#include <algorithm>

namespace claude_draw {
namespace shapes {

/**
 * @brief Rectangle shape with optimized storage (only two points)
 * Total size: 96 bytes (64 base + 32 data)
 * 
 * Data layout (32 bytes):
 * - 16 bytes: min and max points (top-left and bottom-right)
 * - 16 bytes: cached width, height, and area for fast access
 */
class alignas(32) Rectangle : public Shape<Rectangle> {
private:
    // Rectangle-specific data (32 bytes total)
    Point2D min_point_;       // 8 bytes (top-left)
    Point2D max_point_;       // 8 bytes (bottom-right)
    float width_;            // 4 bytes (cached)
    float height_;           // 4 bytes (cached)
    float area_;             // 4 bytes (cached)
    float padding_;          // 4 bytes padding
    
    void update_cache() {
        width_ = max_point_.x - min_point_.x;
        height_ = max_point_.y - min_point_.y;
        area_ = width_ * height_;
    }
    
public:
    // Constructors
    Rectangle() : Rectangle(0, 0, 0, 0) {}
    
    Rectangle(float x, float y, float width, float height)
        : Shape(ShapeType::Rectangle)
        , min_point_(x, y)
        , max_point_(x + width, y + height)
        , width_(width)
        , height_(height)
        , area_(width * height) {
        static_assert(sizeof(Rectangle) == 96, "Rectangle must be exactly 96 bytes");
    }
    
    Rectangle(const Point2D& min_point, const Point2D& max_point)
        : Shape(ShapeType::Rectangle)
        , min_point_(min_point)
        , max_point_(max_point) {
        update_cache();
    }
    
    // Accessors
    const Point2D& get_min_point() const { return min_point_; }
    const Point2D& get_max_point() const { return max_point_; }
    float get_x() const { return min_point_.x; }
    float get_y() const { return min_point_.y; }
    float get_width() const { return width_; }
    float get_height() const { return height_; }
    float get_area() const { return area_; }
    
    // Corner accessors
    Point2D get_top_left() const { return min_point_; }
    Point2D get_top_right() const { return Point2D(max_point_.x, min_point_.y); }
    Point2D get_bottom_left() const { return Point2D(min_point_.x, max_point_.y); }
    Point2D get_bottom_right() const { return max_point_; }
    
    Point2D get_center() const {
        return Point2D(
            (min_point_.x + max_point_.x) * 0.5f,
            (min_point_.y + max_point_.y) * 0.5f
        );
    }
    
    // Modifiers
    void set_position(float x, float y) {
        float dx = x - min_point_.x;
        float dy = y - min_point_.y;
        min_point_.x = x;
        min_point_.y = y;
        max_point_.x += dx;
        max_point_.y += dy;
        invalidate_cache();
    }
    
    void set_size(float width, float height) {
        max_point_.x = min_point_.x + width;
        max_point_.y = min_point_.y + height;
        width_ = width;
        height_ = height;
        area_ = width * height;
        invalidate_cache();
    }
    
    void set_bounds(const Point2D& min_point, const Point2D& max_point) {
        min_point_ = min_point;
        max_point_ = max_point;
        update_cache();
        invalidate_cache();
    }
    
    // Shape interface implementation
    BoundingBox calculate_bounds() const override {
        return BoundingBox(min_point_.x, min_point_.y, max_point_.x, max_point_.y);
    }
    
    bool contains_point(const Point2D& point) const override {
        return point.x >= min_point_.x && point.x <= max_point_.x &&
               point.y >= min_point_.y && point.y <= max_point_.y;
    }
    
    void transform(const Transform2D& transform) override {
        // Transform all four corners and find new bounds
        Point2D corners[4] = {
            get_top_left(),
            get_top_right(),
            get_bottom_left(),
            get_bottom_right()
        };
        
        for (int i = 0; i < 4; ++i) {
            corners[i] = transform.transform_point(corners[i]);
        }
        
        // Find new min/max
        min_point_ = corners[0];
        max_point_ = corners[0];
        
        for (int i = 1; i < 4; ++i) {
            min_point_.x = std::min(min_point_.x, corners[i].x);
            min_point_.y = std::min(min_point_.y, corners[i].y);
            max_point_.x = std::max(max_point_.x, corners[i].x);
            max_point_.y = std::max(max_point_.y, corners[i].y);
        }
        
        update_cache();
        invalidate_cache();
    }
    
    // Rectangle-specific methods
    float perimeter() const {
        return 2.0f * (width_ + height_);
    }
    
    bool intersects_rectangle(const Rectangle& other) const {
        return !(max_point_.x < other.min_point_.x || 
                min_point_.x > other.max_point_.x ||
                max_point_.y < other.min_point_.y || 
                min_point_.y > other.max_point_.y);
    }
    
    Rectangle intersection(const Rectangle& other) const {
        if (!intersects_rectangle(other)) {
            return Rectangle();  // Empty rectangle
        }
        
        return Rectangle(
            Point2D(
                std::max(min_point_.x, other.min_point_.x),
                std::max(min_point_.y, other.min_point_.y)
            ),
            Point2D(
                std::min(max_point_.x, other.max_point_.x),
                std::min(max_point_.y, other.max_point_.y)
            )
        );
    }
    
    Rectangle union_with(const Rectangle& other) const {
        return Rectangle(
            Point2D(
                std::min(min_point_.x, other.min_point_.x),
                std::min(min_point_.y, other.min_point_.y)
            ),
            Point2D(
                std::max(max_point_.x, other.max_point_.x),
                std::max(max_point_.y, other.max_point_.y)
            )
        );
    }
    
    // Fast batch operations support
    static void batch_transform(Rectangle* rectangles, size_t count, const Transform2D& transform) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            rectangles[i].transform(transform);
        }
    }
    
    static void batch_contains(const Rectangle* rectangles, size_t count, 
                              const Point2D* points, bool* results) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = rectangles[i].contains_point(points[i]);
        }
    }
    
    static void batch_intersects(const Rectangle* rects1, const Rectangle* rects2, 
                                size_t count, bool* results) {
        #pragma omp parallel for
        for (size_t i = 0; i < count; ++i) {
            results[i] = rects1[i].intersects_rectangle(rects2[i]);
        }
    }
};

/**
 * @brief Factory functions for creating rectangles
 */
inline std::unique_ptr<Rectangle> make_rectangle(float x, float y, float width, float height) {
    return std::make_unique<Rectangle>(x, y, width, height);
}

inline std::unique_ptr<Rectangle> make_rectangle(const Point2D& min_point, const Point2D& max_point) {
    return std::make_unique<Rectangle>(min_point, max_point);
}

} // namespace shapes
} // namespace claude_draw