#pragma once

#include <cstdint>
#include <algorithm>
#include <ostream>
#include <cmath>

namespace claude_draw {

/**
 * A color with RGBA components packed in a 32-bit integer.
 * 
 * Memory layout (little-endian):
 * - Bits 0-7:   Red channel (0-255)
 * - Bits 8-15:  Green channel (0-255)
 * - Bits 16-23: Blue channel (0-255)
 * - Bits 24-31: Alpha channel (0-255)
 * 
 * This layout is optimized for:
 * - Efficient storage (4 bytes per color)
 * - Fast SIMD operations on color arrays
 * - Direct memory mapping to common pixel formats
 */
struct Color {
    union {
        uint32_t rgba;  // Packed RGBA value
        #pragma GCC diagnostic push
        #pragma GCC diagnostic ignored "-Wpedantic"
        struct {
            uint8_t r;  // Red component
            uint8_t g;  // Green component
            uint8_t b;  // Blue component
            uint8_t a;  // Alpha component
        };
        #pragma GCC diagnostic pop
    };
    
    // Default constructor - opaque black
    constexpr Color() noexcept : rgba(0xFF000000) {}
    
    // Constructor from packed RGBA
    constexpr explicit Color(uint32_t packed) noexcept : rgba(packed) {}
    
    // Constructor from individual components
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) noexcept
        : r(r_), g(g_), b(b_), a(a_) {}
    
    // Constructor from normalized float components [0.0, 1.0]
    Color(float r_, float g_, float b_, float a_ = 1.0f) noexcept {
        r = static_cast<uint8_t>(std::clamp(r_ * 255.0f + 0.5f, 0.0f, 255.0f));
        g = static_cast<uint8_t>(std::clamp(g_ * 255.0f + 0.5f, 0.0f, 255.0f));
        b = static_cast<uint8_t>(std::clamp(b_ * 255.0f + 0.5f, 0.0f, 255.0f));
        a = static_cast<uint8_t>(std::clamp(a_ * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    
    // Get components as normalized floats
    float red_f() const noexcept { return r / 255.0f; }
    float green_f() const noexcept { return g / 255.0f; }
    float blue_f() const noexcept { return b / 255.0f; }
    float alpha_f() const noexcept { return a / 255.0f; }
    
    // Set components from normalized floats
    void set_red_f(float val) noexcept {
        r = static_cast<uint8_t>(std::clamp(val * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    void set_green_f(float val) noexcept {
        g = static_cast<uint8_t>(std::clamp(val * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    void set_blue_f(float val) noexcept {
        b = static_cast<uint8_t>(std::clamp(val * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    void set_alpha_f(float val) noexcept {
        a = static_cast<uint8_t>(std::clamp(val * 255.0f + 0.5f, 0.0f, 255.0f));
    }
    
    // Equality operators
    constexpr bool operator==(const Color& other) const noexcept {
        return rgba == other.rgba;
    }
    
    constexpr bool operator!=(const Color& other) const noexcept {
        return rgba != other.rgba;
    }
    
    // Alpha blending with another color
    Color blend_over(const Color& other) const noexcept {
        if (a == 255) return *this;  // Fully opaque
        if (a == 0) return other;     // Fully transparent
        
        float alpha = a / 255.0f;
        float inv_alpha = 1.0f - alpha;
        
        return Color(
            static_cast<uint8_t>(r * alpha + other.r * inv_alpha),
            static_cast<uint8_t>(g * alpha + other.g * inv_alpha),
            static_cast<uint8_t>(b * alpha + other.b * inv_alpha),
            static_cast<uint8_t>(a + other.a * inv_alpha)
        );
    }
    
    // Premultiply alpha (for more efficient blending)
    Color premultiply() const noexcept {
        if (a == 255) return *this;
        if (a == 0) return Color(static_cast<uint8_t>(0), static_cast<uint8_t>(0), 
                               static_cast<uint8_t>(0), static_cast<uint8_t>(0));
        
        float alpha = a / 255.0f;
        return Color(
            static_cast<uint8_t>(r * alpha),
            static_cast<uint8_t>(g * alpha),
            static_cast<uint8_t>(b * alpha),
            a
        );
    }
    
    // Convert to/from hex string
    uint32_t to_hex() const noexcept {
        return (r << 16) | (g << 8) | b;
    }
    
    static Color from_hex(uint32_t hex, uint8_t alpha = 255) noexcept {
        return Color(
            static_cast<uint8_t>((hex >> 16) & 0xFF),
            static_cast<uint8_t>((hex >> 8) & 0xFF),
            static_cast<uint8_t>(hex & 0xFF),
            alpha
        );
    }
    
    // Convert to HSL (Hue, Saturation, Lightness)
    void to_hsl(float& h, float& s, float& l) const noexcept {
        float rf = red_f();
        float gf = green_f();
        float bf = blue_f();
        
        float max_val = std::max({rf, gf, bf});
        float min_val = std::min({rf, gf, bf});
        float delta = max_val - min_val;
        
        l = (max_val + min_val) / 2.0f;
        
        if (delta < 0.001f) {
            h = 0.0f;
            s = 0.0f;
        } else {
            s = l > 0.5f ? delta / (2.0f - max_val - min_val) : delta / (max_val + min_val);
            
            if (max_val == rf) {
                h = (gf - bf) / delta + (gf < bf ? 6.0f : 0.0f);
            } else if (max_val == gf) {
                h = (bf - rf) / delta + 2.0f;
            } else {
                h = (rf - gf) / delta + 4.0f;
            }
            h /= 6.0f;
        }
    }
    
    // Create from HSL
    static Color from_hsl(float h, float s, float l, uint8_t alpha = 255) noexcept {
        if (s < 0.001f) {
            uint8_t gray = static_cast<uint8_t>(l * 255.0f + 0.5f);
            return Color(gray, gray, gray, alpha);
        }
        
        auto hue_to_rgb = [](float p, float q, float t) {
            if (t < 0.0f) t += 1.0f;
            if (t > 1.0f) t -= 1.0f;
            if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
            if (t < 1.0f/2.0f) return q;
            if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
            return p;
        };
        
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        
        return Color(
            hue_to_rgb(p, q, h + 1.0f/3.0f),
            hue_to_rgb(p, q, h),
            hue_to_rgb(p, q, h - 1.0f/3.0f),
            alpha / 255.0f
        );
    }
    
    // Common color constants
    static constexpr Color transparent() noexcept { return Color(0x00000000); }
    static constexpr Color black() noexcept { return Color(0xFF000000); }
    static constexpr Color white() noexcept { return Color(0xFFFFFFFF); }
    static constexpr Color red() noexcept { return Color(0xFF0000FF); }
    static constexpr Color green() noexcept { return Color(0xFF00FF00); }
    static constexpr Color blue() noexcept { return Color(0xFFFF0000); }
    static constexpr Color yellow() noexcept { return Color(0xFF00FFFF); }
    static constexpr Color cyan() noexcept { return Color(0xFFFFFF00); }
    static constexpr Color magenta() noexcept { return Color(0xFFFF00FF); }
};

// Verify Color is efficient
static_assert(sizeof(Color) == 4, "Color must be exactly 4 bytes");

// Stream output
inline std::ostream& operator<<(std::ostream& os, const Color& color) {
    return os << "Color(r=" << static_cast<int>(color.r) 
              << ", g=" << static_cast<int>(color.g)
              << ", b=" << static_cast<int>(color.b)
              << ", a=" << static_cast<int>(color.a) << ")";
}

// Batch operations for SIMD optimization
namespace batch {

/**
 * Blend source colors over destination colors.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void blend_over(const Color* src, const Color* dst, Color* result, size_t count) noexcept;

/**
 * Premultiply alpha for an array of colors.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void premultiply(const Color* colors, Color* result, size_t count) noexcept;

/**
 * Convert an array of colors to grayscale.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void to_grayscale(const Color* colors, Color* result, size_t count) noexcept;

} // namespace batch

} // namespace claude_draw