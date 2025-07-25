#pragma once

#include <cstdint>
#include <memory>
#include <cstring>
#include <cmath>
#include "../core/point2d.h"
#include "../core/bounding_box.h"
#include "../core/transform2d.h"
#include "../core/color.h"

namespace claude_draw {
namespace shapes {

/**
 * @brief Shape type enumeration for fast type checking
 */
enum class ShapeType : uint8_t {
    Circle = 0,
    Rectangle = 1,
    Ellipse = 2,
    Line = 3,
    Polygon = 4,
    Path = 5,
    Text = 6,
    Image = 7
};

/**
 * @brief Shape flags for common properties
 */
enum class ShapeFlags : uint8_t {
    None = 0,
    Visible = 1 << 0,
    Filled = 1 << 1,
    Stroked = 1 << 2,
    HasTransform = 1 << 3,
    HasStyle = 1 << 4
};

inline ShapeFlags operator|(ShapeFlags a, ShapeFlags b) {
    return static_cast<ShapeFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

inline ShapeFlags operator&(ShapeFlags a, ShapeFlags b) {
    return static_cast<ShapeFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline bool has_flag(ShapeFlags flags, ShapeFlags flag) {
    return (flags & flag) == flag;
}

/**
 * @brief Compact Circle shape - 32 bytes total
 * Memory layout:
 * - 8 bytes: center (x, y)
 * - 4 bytes: radius
 * - 4 bytes: fill color (RGBA)
 * - 4 bytes: stroke color (RGBA)
 * - 4 bytes: stroke width
 * - 1 byte: type
 * - 1 byte: flags
 * - 2 bytes: reserved
 * - 4 bytes: id
 */
struct alignas(32) CircleShape {
    // Geometric data (12 bytes)
    float center_x;
    float center_y;
    float radius;
    
    // Style data (12 bytes)
    uint32_t fill_color;    // RGBA packed
    uint32_t stroke_color;  // RGBA packed
    float stroke_width;
    
    // Metadata (8 bytes)
    ShapeType type;
    ShapeFlags flags;
    uint16_t reserved;
    uint32_t id;
    
    CircleShape() 
        : center_x(0), center_y(0), radius(0)
        , fill_color(0xFF000000), stroke_color(0xFF000000), stroke_width(1.0f)
        , type(ShapeType::Circle), flags(ShapeFlags::Visible | ShapeFlags::Filled)
        , reserved(0), id(0) {
        static_assert(sizeof(CircleShape) == 32, "CircleShape must be exactly 32 bytes");
    }
    
    CircleShape(float x, float y, float r)
        : center_x(x), center_y(y), radius(r)
        , fill_color(0xFF000000), stroke_color(0xFF000000), stroke_width(1.0f)
        , type(ShapeType::Circle), flags(ShapeFlags::Visible | ShapeFlags::Filled)
        , reserved(0), id(0) {}
    
    BoundingBox bounds() const {
        return BoundingBox(center_x - radius, center_y - radius, 
                          center_x + radius, center_y + radius);
    }
    
    bool contains(float x, float y) const {
        float dx = x - center_x;
        float dy = y - center_y;
        return (dx * dx + dy * dy) <= (radius * radius);
    }
    
    void set_fill_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        fill_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
    
    void set_stroke_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        stroke_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
};

/**
 * @brief Compact Rectangle shape - 32 bytes total
 * Memory layout:
 * - 16 bytes: two points (x1, y1, x2, y2)
 * - 4 bytes: fill color (RGBA)
 * - 4 bytes: stroke color (RGBA)
 * - 4 bytes: stroke width
 * - 4 bytes: metadata (type, flags, id)
 */
struct alignas(32) RectangleShape {
    // Geometric data (16 bytes)
    float x1, y1;  // Top-left
    float x2, y2;  // Bottom-right
    
    // Style data (12 bytes)
    uint32_t fill_color;    // RGBA packed
    uint32_t stroke_color;  // RGBA packed
    float stroke_width;
    
    // Metadata (4 bytes)
    uint8_t type;
    uint8_t flags;
    uint16_t id;
    
    RectangleShape()
        : x1(0), y1(0), x2(0), y2(0)
        , fill_color(0xFF000000), stroke_color(0xFF000000), stroke_width(1.0f)
        , type(static_cast<uint8_t>(ShapeType::Rectangle))
        , flags(static_cast<uint8_t>(ShapeFlags::Visible | ShapeFlags::Filled))
        , id(0) {
        static_assert(sizeof(RectangleShape) == 32, "RectangleShape must be exactly 32 bytes");
    }
    
    RectangleShape(float x, float y, float width, float height)
        : x1(x), y1(y), x2(x + width), y2(y + height)
        , fill_color(0xFF000000), stroke_color(0xFF000000), stroke_width(1.0f)
        , type(static_cast<uint8_t>(ShapeType::Rectangle))
        , flags(static_cast<uint8_t>(ShapeFlags::Visible | ShapeFlags::Filled))
        , id(0) {}
    
    float width() const { return x2 - x1; }
    float height() const { return y2 - y1; }
    
    BoundingBox bounds() const {
        return BoundingBox(x1, y1, x2, y2);
    }
    
    bool contains(float x, float y) const {
        return x >= x1 && x <= x2 && y >= y1 && y <= y2;
    }
    
    void set_fill_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        fill_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
    
    void set_stroke_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        stroke_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
};

/**
 * @brief Compact Ellipse shape - 32 bytes total
 * Memory layout:
 * - 16 bytes: center and radii (cx, cy, rx, ry)
 * - 4 bytes: fill color (RGBA)
 * - 4 bytes: stroke color (RGBA)
 * - 4 bytes: stroke width
 * - 4 bytes: metadata
 */
struct alignas(32) EllipseShape {
    // Geometric data (16 bytes)
    float center_x, center_y;
    float radius_x, radius_y;
    
    // Style data (12 bytes)
    uint32_t fill_color;    // RGBA packed
    uint32_t stroke_color;  // RGBA packed
    float stroke_width;
    
    // Metadata (4 bytes)
    uint8_t type;
    uint8_t flags;
    uint16_t id;
    
    EllipseShape()
        : center_x(0), center_y(0), radius_x(0), radius_y(0)
        , fill_color(0xFF000000), stroke_color(0xFF000000), stroke_width(1.0f)
        , type(static_cast<uint8_t>(ShapeType::Ellipse))
        , flags(static_cast<uint8_t>(ShapeFlags::Visible | ShapeFlags::Filled))
        , id(0) {
        static_assert(sizeof(EllipseShape) == 32, "EllipseShape must be exactly 32 bytes");
    }
    
    EllipseShape(float cx, float cy, float rx, float ry)
        : center_x(cx), center_y(cy), radius_x(rx), radius_y(ry)
        , fill_color(0xFF000000), stroke_color(0xFF000000), stroke_width(1.0f)
        , type(static_cast<uint8_t>(ShapeType::Ellipse))
        , flags(static_cast<uint8_t>(ShapeFlags::Visible | ShapeFlags::Filled))
        , id(0) {}
    
    BoundingBox bounds() const {
        return BoundingBox(center_x - radius_x, center_y - radius_y, 
                          center_x + radius_x, center_y + radius_y);
    }
    
    bool contains(float x, float y) const {
        float dx = (x - center_x) / radius_x;
        float dy = (y - center_y) / radius_y;
        return (dx * dx + dy * dy) <= 1.0f;
    }
    
    void set_fill_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        fill_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
    
    void set_stroke_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        stroke_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
};

/**
 * @brief Compact Line shape - 32 bytes total
 * Memory layout:
 * - 16 bytes: two points (x1, y1, x2, y2)
 * - 4 bytes: stroke color (RGBA)
 * - 4 bytes: stroke width
 * - 4 bytes: line cap and join style
 * - 4 bytes: metadata
 */
struct alignas(32) LineShape {
    // Geometric data (16 bytes)
    float x1, y1;  // Start point
    float x2, y2;  // End point
    
    // Style data (12 bytes)
    uint32_t stroke_color;  // RGBA packed
    float stroke_width;
    uint16_t line_cap;      // 0=butt, 1=round, 2=square
    uint16_t line_join;     // 0=miter, 1=round, 2=bevel
    
    // Metadata (4 bytes)
    uint8_t type;
    uint8_t flags;
    uint16_t id;
    
    LineShape()
        : x1(0), y1(0), x2(0), y2(0)
        , stroke_color(0xFF000000), stroke_width(1.0f)
        , line_cap(0), line_join(0)
        , type(static_cast<uint8_t>(ShapeType::Line))
        , flags(static_cast<uint8_t>(ShapeFlags::Visible | ShapeFlags::Stroked))
        , id(0) {
        static_assert(sizeof(LineShape) == 32, "LineShape must be exactly 32 bytes");
    }
    
    LineShape(float x1_, float y1_, float x2_, float y2_)
        : x1(x1_), y1(y1_), x2(x2_), y2(y2_)
        , stroke_color(0xFF000000), stroke_width(1.0f)
        , line_cap(0), line_join(0)
        , type(static_cast<uint8_t>(ShapeType::Line))
        , flags(static_cast<uint8_t>(ShapeFlags::Visible | ShapeFlags::Stroked))
        , id(0) {}
    
    float length() const {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    BoundingBox bounds() const {
        return BoundingBox(
            std::min(x1, x2), std::min(y1, y2),
            std::max(x1, x2), std::max(y1, y2)
        );
    }
    
    void set_stroke_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        stroke_color = (a << 24) | (b << 16) | (g << 8) | r;
    }
};

/**
 * @brief Union of all shape types for compact storage
 */
union ShapeUnion {
    CircleShape circle;
    RectangleShape rectangle;
    EllipseShape ellipse;
    LineShape line;
    uint8_t raw[32];
    
    ShapeUnion() { std::memset(raw, 0, sizeof(raw)); }
};

static_assert(sizeof(ShapeUnion) == 32, "ShapeUnion must be exactly 32 bytes");

/**
 * @brief Type trait to identify shapes
 */
template<typename T>
struct is_shape : std::false_type {};

} // namespace shapes
} // namespace claude_draw