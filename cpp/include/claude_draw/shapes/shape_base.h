#pragma once

#include <cstdint>
#include <memory>
#include <variant>
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/bounding_box.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/color.h"

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
 * @brief Base shape flags for common properties
 */
enum class ShapeFlags : uint8_t {
    None = 0,
    Visible = 1 << 0,
    Filled = 1 << 1,
    Stroked = 1 << 2,
    Transformed = 1 << 3,
    Clipped = 1 << 4,
    CacheDirty = 1 << 5
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
 * @brief Style information for shapes (compact representation)
 * Size: 16 bytes (cache-aligned)
 */
struct alignas(16) ShapeStyle {
    Color fill_color{uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(0)};      // 4 bytes
    Color stroke_color{uint8_t(0), uint8_t(0), uint8_t(0), uint8_t(255)};  // 4 bytes
    float stroke_width{1.0f};          // 4 bytes
    float opacity{1.0f};               // 4 bytes
    
    ShapeStyle() = default;
    ShapeStyle(const Color& fill) : fill_color(fill) {}
    ShapeStyle(const Color& fill, const Color& stroke, float width) 
        : fill_color(fill), stroke_color(stroke), stroke_width(width) {}
};

/**
 * @brief Base class for all shapes with minimal overhead
 * Size: 64 bytes (one cache line) for optimal performance
 * 
 * Memory layout:
 * - 8 bytes: vtable pointer
 * - 1 byte: shape type
 * - 1 byte: flags
 * - 2 bytes: padding
 * - 4 bytes: ID
 * - 16 bytes: style
 * - 32 bytes: cached bounding box (can be invalid)
 */
class ShapeBase {
protected:
    // Core properties (12 bytes + vtable)
    ShapeType type_;
    ShapeFlags flags_;
    uint16_t padding_;  // Explicit padding for alignment
    uint32_t id_;       // Compact ID instead of UUID
    
    // Style information (16 bytes)
    ShapeStyle style_;
    
    // Cached bounding box (32 bytes)
    mutable BoundingBox cached_bounds_;
    mutable bool bounds_valid_;
    uint8_t padding2_[7];  // Padding to reach 64 bytes
    
public:
    ShapeBase(ShapeType type) 
        : type_(type)
        , flags_(ShapeFlags::Visible | ShapeFlags::Filled)
        , padding_(0)
        , id_(0)
        , style_()
        , cached_bounds_()
        , bounds_valid_(false) {
    }
    
    virtual ~ShapeBase() = default;
    
    // Type information
    ShapeType get_type() const { return type_; }
    bool is_type(ShapeType type) const { return type_ == type; }
    
    // ID management
    uint32_t get_id() const { return id_; }
    void set_id(uint32_t id) { id_ = id; }
    
    // Flag management
    ShapeFlags get_flags() const { return flags_; }
    void set_flags(ShapeFlags flags) { flags_ = flags; invalidate_cache(); }
    void add_flags(ShapeFlags flags) { flags_ = flags_ | flags; invalidate_cache(); }
    void remove_flags(ShapeFlags flags) { 
        flags_ = static_cast<ShapeFlags>(static_cast<uint8_t>(flags_) & ~static_cast<uint8_t>(flags));
        invalidate_cache();
    }
    bool has_flag(ShapeFlags flag) const { return ::claude_draw::shapes::has_flag(flags_, flag); }
    
    // Style management
    const ShapeStyle& get_style() const { return style_; }
    ShapeStyle& get_style() { return style_; }
    void set_style(const ShapeStyle& style) { style_ = style; invalidate_cache(); }
    
    // Visibility
    bool is_visible() const { return has_flag(ShapeFlags::Visible); }
    void set_visible(bool visible) {
        if (visible) add_flags(ShapeFlags::Visible);
        else remove_flags(ShapeFlags::Visible);
    }
    
    // Fill and stroke
    bool is_filled() const { return has_flag(ShapeFlags::Filled); }
    void set_filled(bool filled) {
        if (filled) add_flags(ShapeFlags::Filled);
        else remove_flags(ShapeFlags::Filled);
    }
    
    bool is_stroked() const { return has_flag(ShapeFlags::Stroked); }
    void set_stroked(bool stroked) {
        if (stroked) add_flags(ShapeFlags::Stroked);
        else remove_flags(ShapeFlags::Stroked);
    }
    
    // Abstract methods that must be implemented by derived classes
    virtual BoundingBox calculate_bounds() const = 0;
    virtual bool contains_point(const Point2D& point) const = 0;
    virtual void transform(const Transform2D& transform) = 0;
    virtual std::unique_ptr<ShapeBase> clone() const = 0;
    
    // Bounds management with caching
    const BoundingBox& get_bounds() const {
        if (!bounds_valid_) {
            cached_bounds_ = calculate_bounds();
            bounds_valid_ = true;
        }
        return cached_bounds_;
    }
    
    // Fast bounds check without virtual call if cached
    bool bounds_contains(const Point2D& point) const {
        return get_bounds().contains(point);
    }
    
protected:
    void invalidate_cache() const {
        bounds_valid_ = false;
        add_flags(ShapeFlags::CacheDirty);
    }
    
private:
    // Make sure we maintain cache line alignment
    void add_flags(ShapeFlags flags) const {
        const_cast<ShapeBase*>(this)->flags_ = flags_ | flags;
    }
};

/**
 * @brief CRTP base for shapes to avoid virtual function overhead where possible
 */
template<typename Derived>
class Shape : public ShapeBase {
public:
    using ShapeBase::ShapeBase;
    
    std::unique_ptr<ShapeBase> clone() const override {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }
};

} // namespace shapes
} // namespace claude_draw