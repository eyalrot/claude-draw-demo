#pragma once

#include "shape_base_optimized.h"
#include "../core/transform2d.h"
#include <cmath>
#include <memory>
#include <algorithm>

namespace claude_draw {
namespace shapes {

/**
 * @brief Optimized Rectangle class with 32-byte footprint
 * 
 * This class provides a full-featured rectangle implementation while maintaining
 * the compact 32-byte memory layout for performance. The rectangle is stored as
 * two points (top-left and bottom-right) for efficiency.
 */
class Rectangle {
private:
    RectangleShape data_;
    
public:
    // Constructors
    Rectangle() : data_() {}
    
    Rectangle(float x, float y, float width, float height) 
        : data_(x, y, width, height) {}
    
    Rectangle(const Point2D& top_left, const Point2D& bottom_right)
        : data_(top_left.x, top_left.y, 
                bottom_right.x - top_left.x, 
                bottom_right.y - top_left.y) {}
    
    // Constructor from raw data (for container use)
    explicit Rectangle(const RectangleShape& data) : data_(data) {}
    
    // Copy and move constructors
    Rectangle(const Rectangle&) = default;
    Rectangle(Rectangle&&) = default;
    Rectangle& operator=(const Rectangle&) = default;
    Rectangle& operator=(Rectangle&&) = default;
    
    // Accessors
    float get_x() const { return data_.x1; }
    float get_y() const { return data_.y1; }
    float get_width() const { return data_.width(); }
    float get_height() const { return data_.height(); }
    
    Point2D get_top_left() const { return Point2D(data_.x1, data_.y1); }
    Point2D get_top_right() const { return Point2D(data_.x2, data_.y1); }
    Point2D get_bottom_left() const { return Point2D(data_.x1, data_.y2); }
    Point2D get_bottom_right() const { return Point2D(data_.x2, data_.y2); }
    
    Point2D get_center() const {
        return Point2D((data_.x1 + data_.x2) * 0.5f, 
                      (data_.y1 + data_.y2) * 0.5f);
    }
    
    // Setters
    void set_position(float x, float y) {
        float w = data_.width();
        float h = data_.height();
        data_.x1 = x;
        data_.y1 = y;
        data_.x2 = x + w;
        data_.y2 = y + h;
    }
    
    void set_size(float width, float height) {
        data_.x2 = data_.x1 + width;
        data_.y2 = data_.y1 + height;
    }
    
    void set_bounds(float x1, float y1, float x2, float y2) {
        data_.x1 = x1;
        data_.y1 = y1;
        data_.x2 = x2;
        data_.y2 = y2;
    }
    
    void set_bounds(const Point2D& top_left, const Point2D& bottom_right) {
        data_.x1 = top_left.x;
        data_.y1 = top_left.y;
        data_.x2 = bottom_right.x;
        data_.y2 = bottom_right.y;
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
        return data_.width() * data_.height();
    }
    
    float perimeter() const {
        return 2.0f * (data_.width() + data_.height());
    }
    
    // Transformation
    void transform(const Transform2D& transform) {
        // Transform all four corners
        Point2D corners[4] = {
            get_top_left(),
            get_top_right(),
            get_bottom_left(),
            get_bottom_right()
        };
        
        for (int i = 0; i < 4; ++i) {
            corners[i] = transform.transform_point(corners[i]);
        }
        
        // Find new axis-aligned bounding box
        float min_x = corners[0].x;
        float min_y = corners[0].y;
        float max_x = corners[0].x;
        float max_y = corners[0].y;
        
        for (int i = 1; i < 4; ++i) {
            min_x = std::min(min_x, corners[i].x);
            min_y = std::min(min_y, corners[i].y);
            max_x = std::max(max_x, corners[i].x);
            max_y = std::max(max_y, corners[i].y);
        }
        
        data_.x1 = min_x;
        data_.y1 = min_y;
        data_.x2 = max_x;
        data_.y2 = max_y;
    }
    
    // Rectangle-specific operations
    bool intersects_rectangle(const Rectangle& other) const {
        return !(data_.x2 < other.data_.x1 || 
                data_.x1 > other.data_.x2 ||
                data_.y2 < other.data_.y1 || 
                data_.y1 > other.data_.y2);
    }
    
    Rectangle intersection(const Rectangle& other) const {
        if (!intersects_rectangle(other)) {
            return Rectangle();  // Empty rectangle
        }
        
        return Rectangle(
            Point2D(
                std::max(data_.x1, other.data_.x1),
                std::max(data_.y1, other.data_.y1)
            ),
            Point2D(
                std::min(data_.x2, other.data_.x2),
                std::min(data_.y2, other.data_.y2)
            )
        );
    }
    
    Rectangle union_with(const Rectangle& other) const {
        return Rectangle(
            Point2D(
                std::min(data_.x1, other.data_.x1),
                std::min(data_.y1, other.data_.y1)
            ),
            Point2D(
                std::max(data_.x2, other.data_.x2),
                std::max(data_.y2, other.data_.y2)
            )
        );
    }
    
    // Clone operation
    std::unique_ptr<Rectangle> clone() const {
        return std::make_unique<Rectangle>(*this);
    }
    
    // Direct access to underlying data (for batch operations)
    const RectangleShape& data() const { return data_; }
    RectangleShape& data() { return data_; }
    
    // Static batch operations
    static void batch_transform(Rectangle* rectangles, size_t count, const Transform2D& transform);
    static void batch_contains(const Rectangle* rectangles, size_t count, 
                              const Point2D* points, bool* results);
    static void batch_intersects(const Rectangle* rects1, const Rectangle* rects2,
                                size_t count, bool* results);
};

// Batch operation implementations
inline void Rectangle::batch_transform(Rectangle* rectangles, size_t count, const Transform2D& transform) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        rectangles[i].transform(transform);
    }
}

inline void Rectangle::batch_contains(const Rectangle* rectangles, size_t count, 
                                     const Point2D* points, bool* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = rectangles[i].contains_point(points[i]);
    }
}

inline void Rectangle::batch_intersects(const Rectangle* rects1, const Rectangle* rects2,
                                       size_t count, bool* results) {
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        results[i] = rects1[i].intersects_rectangle(rects2[i]);
    }
}

// Factory functions
inline std::unique_ptr<Rectangle> make_rectangle(float x, float y, float width, float height) {
    return std::make_unique<Rectangle>(x, y, width, height);
}

inline std::unique_ptr<Rectangle> make_rectangle(const Point2D& top_left, const Point2D& bottom_right) {
    return std::make_unique<Rectangle>(top_left, bottom_right);
}

// Type trait to identify this as a shape
template<>
struct is_shape<Rectangle> : std::true_type {};

} // namespace shapes
} // namespace claude_draw