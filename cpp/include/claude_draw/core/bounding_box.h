#pragma once

#include "point2d.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <ostream>
#include <type_traits>
#include <cmath>

namespace claude_draw {

/**
 * Axis-aligned bounding box (AABB) for 2D geometry.
 * 
 * Designed for efficiency:
 * - 16-byte size (4 floats)
 * - POD type for efficient copying
 * - SIMD-friendly layout
 * - Fast intersection and containment tests
 * 
 * The bounding box is represented by its minimum and maximum corners.
 * An empty bounding box has min > max.
 */
struct BoundingBox {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
    
    // Default constructor - creates an empty bounding box
    BoundingBox() noexcept
        : min_x(std::numeric_limits<float>::max())
        , min_y(std::numeric_limits<float>::max())
        , max_x(std::numeric_limits<float>::lowest())
        , max_y(std::numeric_limits<float>::lowest()) {}
    
    // Constructor from min/max values
    BoundingBox(float min_x, float min_y, float max_x, float max_y) noexcept
        : min_x(min_x), min_y(min_y), max_x(max_x), max_y(max_y) {}
    
    // Constructor from two points
    BoundingBox(const Point2D& p1, const Point2D& p2) noexcept
        : min_x(std::min(p1.x, p2.x))
        , min_y(std::min(p1.y, p2.y))
        , max_x(std::max(p1.x, p2.x))
        , max_y(std::max(p1.y, p2.y)) {}
    
    // Create from center and half-extents
    static BoundingBox from_center_half_extents(const Point2D& center, float half_width, float half_height) noexcept {
        return {
            center.x - half_width,
            center.y - half_height,
            center.x + half_width,
            center.y + half_height
        };
    }
    
    // Create from center and size
    static BoundingBox from_center_size(const Point2D& center, float width, float height) noexcept {
        float half_width = width * 0.5F;
        float half_height = height * 0.5F;
        return from_center_half_extents(center, half_width, half_height);
    }
    
    // Check if the bounding box is empty
    [[nodiscard]] bool is_empty() const noexcept {
        return min_x > max_x || min_y > max_y;
    }
    
    // Check if the bounding box is valid (non-empty)
    [[nodiscard]] bool is_valid() const noexcept {
        return !is_empty();
    }
    
    // Get width
    [[nodiscard]] float width() const noexcept {
        return is_empty() ? 0.0F : max_x - min_x;
    }
    
    // Get height
    [[nodiscard]] float height() const noexcept {
        return is_empty() ? 0.0F : max_y - min_y;
    }
    
    // Get area
    [[nodiscard]] float area() const noexcept {
        return is_empty() ? 0.0F : width() * height();
    }
    
    // Get perimeter
    [[nodiscard]] float perimeter() const noexcept {
        return is_empty() ? 0.0F : 2.0F * (width() + height());
    }
    
    // Get center point
    [[nodiscard]] Point2D center() const noexcept {
        return {
            (min_x + max_x) * 0.5F,
            (min_y + max_y) * 0.5F
        };
    }
    
    // Get center coordinates
    [[nodiscard]] float center_x() const noexcept {
        return (min_x + max_x) * 0.5F;
    }
    
    [[nodiscard]] float center_y() const noexcept {
        return (min_y + max_y) * 0.5F;
    }
    
    // Get corner points
    [[nodiscard]] Point2D min_corner() const noexcept {
        return {min_x, min_y};
    }
    
    [[nodiscard]] Point2D max_corner() const noexcept {
        return {max_x, max_y};
    }
    
    [[nodiscard]] Point2D top_left() const noexcept {
        return {min_x, max_y};
    }
    
    [[nodiscard]] Point2D top_right() const noexcept {
        return {max_x, max_y};
    }
    
    [[nodiscard]] Point2D bottom_left() const noexcept {
        return {min_x, min_y};
    }
    
    [[nodiscard]] Point2D bottom_right() const noexcept {
        return {max_x, min_y};
    }
    
    // Check if a point is contained within the bounding box
    [[nodiscard]] bool contains(const Point2D& p) const noexcept {
        return p.x >= min_x && p.x <= max_x &&
               p.y >= min_y && p.y <= max_y;
    }
    
    // Check if another bounding box is fully contained
    [[nodiscard]] bool contains(const BoundingBox& other) const noexcept {
        if (is_empty() || other.is_empty()) return false;
        return other.min_x >= min_x && other.max_x <= max_x &&
               other.min_y >= min_y && other.max_y <= max_y;
    }
    
    // Check if two bounding boxes intersect
    [[nodiscard]] bool intersects(const BoundingBox& other) const noexcept {
        if (is_empty() || other.is_empty()) return false;
        return min_x <= other.max_x && max_x >= other.min_x &&
               min_y <= other.max_y && max_y >= other.min_y;
    }
    
    // Calculate minimum distance from a point to the bounding box
    [[nodiscard]] float distance_to_point(float x, float y) const noexcept {
        if (is_empty()) return std::numeric_limits<float>::infinity();
        
        // If point is inside, distance is 0
        if (x >= min_x && x <= max_x && y >= min_y && y <= max_y) {
            return 0.0f;
        }
        
        // Calculate squared distance to avoid sqrt
        float dx = 0.0f;
        float dy = 0.0f;
        
        if (x < min_x) dx = min_x - x;
        else if (x > max_x) dx = x - max_x;
        
        if (y < min_y) dy = min_y - y;
        else if (y > max_y) dy = y - max_y;
        
        return std::sqrt(dx * dx + dy * dy);
    }
    
    // Compute intersection of two bounding boxes
    [[nodiscard]] BoundingBox intersection(const BoundingBox& other) const noexcept {
        if (!intersects(other)) return BoundingBox();
        
        return {
            std::max(min_x, other.min_x),
            std::max(min_y, other.min_y),
            std::min(max_x, other.max_x),
            std::min(max_y, other.max_y)
        };
    }
    
    // Compute union of two bounding boxes
    [[nodiscard]] BoundingBox union_with(const BoundingBox& other) const noexcept {
        if (is_empty()) return other;
        if (other.is_empty()) return *this;
        
        return {
            std::min(min_x, other.min_x),
            std::min(min_y, other.min_y),
            std::max(max_x, other.max_x),
            std::max(max_y, other.max_y)
        };
    }
    
    // Alias for union_with for compatibility
    [[nodiscard]] BoundingBox merge(const BoundingBox& other) const noexcept {
        return union_with(other);
    }
    
    // Expand to include a point
    void expand(const Point2D& p) noexcept {
        min_x = std::min(min_x, p.x);
        min_y = std::min(min_y, p.y);
        max_x = std::max(max_x, p.x);
        max_y = std::max(max_y, p.y);
    }
    
    // Expand to include another bounding box
    void expand(const BoundingBox& other) noexcept {
        if (other.is_empty()) return;
        
        min_x = std::min(min_x, other.min_x);
        min_y = std::min(min_y, other.min_y);
        max_x = std::max(max_x, other.max_x);
        max_y = std::max(max_y, other.max_y);
    }
    
    // Inflate the bounding box by a given amount
    void inflate(float amount) noexcept {
        if (is_empty()) return;
        
        min_x -= amount;
        min_y -= amount;
        max_x += amount;
        max_y += amount;
    }
    
    // Inflate with different amounts for x and y
    void inflate(float dx, float dy) noexcept {
        if (is_empty()) return;
        
        min_x -= dx;
        min_y -= dy;
        max_x += dx;
        max_y += dy;
    }
    
    // Scale the bounding box around its center
    void scale(float factor) noexcept {
        if (is_empty()) return;
        
        Point2D c = center();
        float hw = width() * 0.5F * factor;
        float hh = height() * 0.5F * factor;
        
        min_x = c.x - hw;
        min_y = c.y - hh;
        max_x = c.x + hw;
        max_y = c.y + hh;
    }
    
    // Translate the bounding box
    void translate(float dx, float dy) noexcept {
        min_x += dx;
        min_y += dy;
        max_x += dx;
        max_y += dy;
    }
    
    void translate(const Point2D& offset) noexcept {
        translate(offset.x, offset.y);
    }
    
    // Reset to empty state
    void reset() noexcept {
        *this = BoundingBox();
    }
    
    // Equality operators
    [[nodiscard]] bool operator==(const BoundingBox& other) const noexcept {
        return min_x == other.min_x && min_y == other.min_y &&
               max_x == other.max_x && max_y == other.max_y;
    }
    
    [[nodiscard]] bool operator!=(const BoundingBox& other) const noexcept {
        return !(*this == other);
    }
    
    // Check near equality with epsilon
    [[nodiscard]] bool nearly_equal(const BoundingBox& other, float epsilon = 1e-6F) const noexcept {
        return std::abs(min_x - other.min_x) < epsilon &&
               std::abs(min_y - other.min_y) < epsilon &&
               std::abs(max_x - other.max_x) < epsilon &&
               std::abs(max_y - other.max_y) < epsilon;
    }
};

// Verify BoundingBox is POD
static_assert(std::is_trivially_copyable<BoundingBox>::value, "BoundingBox must be trivially copyable");
static_assert(std::is_standard_layout<BoundingBox>::value, "BoundingBox must have standard layout");
static_assert(sizeof(BoundingBox) == 16, "BoundingBox must be 16 bytes");

// Stream output
inline std::ostream& operator<<(std::ostream& os, const BoundingBox& bb) {
    os << "BoundingBox(min: (" << bb.min_x << ", " << bb.min_y 
       << "), max: (" << bb.max_x << ", " << bb.max_y << "))";
    return os;
}

// Batch operations for SIMD optimization
namespace batch {

/**
 * Test multiple points against a bounding box.
 * Results array will contain 1 for points inside, 0 for points outside.
 */
void contains_points(const BoundingBox& bb, const Point2D* points, 
                    uint8_t* results, size_t count) noexcept;

/**
 * Test multiple bounding boxes for intersection with a reference box.
 * Results array will contain 1 for intersecting boxes, 0 for non-intersecting.
 */
void intersects_boxes(const BoundingBox& bb, const BoundingBox* boxes, 
                     uint8_t* results, size_t count) noexcept;

/**
 * Compute the union of multiple bounding boxes.
 * Returns an empty box if count is 0.
 */
BoundingBox union_boxes(const BoundingBox* boxes, size_t count) noexcept;

/**
 * Expand multiple bounding boxes by the same amount.
 */
void inflate_boxes(const BoundingBox* boxes, BoundingBox* results, 
                   float amount, size_t count) noexcept;

} // namespace batch

} // namespace claude_draw