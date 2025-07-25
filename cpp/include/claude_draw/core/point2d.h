#pragma once

#include <cmath>
#include <ostream>
#include <type_traits>

namespace claude_draw {

/**
 * A 2D point with float coordinates.
 * 
 * This is a POD (Plain Old Data) struct optimized for:
 * - Memory efficiency (8 bytes total)
 * - SIMD operations (can be loaded as 64-bit value)
 * - Cache performance (no padding)
 * - Zero-cost abstractions
 */
struct Point2D {
    float x;
    float y;
    
    // Default constructor - zero initialization
    constexpr Point2D() noexcept : x(0.0f), y(0.0f) {}
    
    // Constructor with coordinates
    constexpr Point2D(float x_, float y_) noexcept : x(x_), y(y_) {}
    
    // Copy constructor (default)
    constexpr Point2D(const Point2D&) noexcept = default;
    
    // Move constructor (default)
    constexpr Point2D(Point2D&&) noexcept = default;
    
    // Copy assignment (default)
    Point2D& operator=(const Point2D&) noexcept = default;
    
    // Move assignment (default)
    Point2D& operator=(Point2D&&) noexcept = default;
    
    // Arithmetic operators
    constexpr Point2D operator+(const Point2D& other) const noexcept {
        return Point2D(x + other.x, y + other.y);
    }
    
    constexpr Point2D operator-(const Point2D& other) const noexcept {
        return Point2D(x - other.x, y - other.y);
    }
    
    constexpr Point2D operator*(float scalar) const noexcept {
        return Point2D(x * scalar, y * scalar);
    }
    
    constexpr Point2D operator/(float scalar) const noexcept {
        return Point2D(x / scalar, y / scalar);
    }
    
    // Compound assignment operators
    Point2D& operator+=(const Point2D& other) noexcept {
        x += other.x;
        y += other.y;
        return *this;
    }
    
    Point2D& operator-=(const Point2D& other) noexcept {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    
    Point2D& operator*=(float scalar) noexcept {
        x *= scalar;
        y *= scalar;
        return *this;
    }
    
    Point2D& operator/=(float scalar) noexcept {
        x /= scalar;
        y /= scalar;
        return *this;
    }
    
    // Equality operators
    constexpr bool operator==(const Point2D& other) const noexcept {
        return x == other.x && y == other.y;
    }
    
    constexpr bool operator!=(const Point2D& other) const noexcept {
        return !(*this == other);
    }
    
    // Unary negation
    constexpr Point2D operator-() const noexcept {
        return Point2D(-x, -y);
    }
    
    // Distance calculations
    float distance_to(const Point2D& other) const noexcept {
        float dx = x - other.x;
        float dy = y - other.y;
        return std::sqrt(dx * dx + dy * dy);
    }
    
    float distance_squared_to(const Point2D& other) const noexcept {
        float dx = x - other.x;
        float dy = y - other.y;
        return dx * dx + dy * dy;
    }
    
    // Magnitude (distance from origin)
    float magnitude() const noexcept {
        return std::sqrt(x * x + y * y);
    }
    
    float magnitude_squared() const noexcept {
        return x * x + y * y;
    }
    
    // Normalize (return unit vector)
    Point2D normalized() const noexcept {
        float mag = magnitude();
        if (mag > 0.0f) {
            return Point2D(x / mag, y / mag);
        }
        return *this;
    }
    
    // Dot product
    constexpr float dot(const Point2D& other) const noexcept {
        return x * other.x + y * other.y;
    }
    
    // Cross product (2D returns scalar)
    constexpr float cross(const Point2D& other) const noexcept {
        return x * other.y - y * other.x;
    }
    
    // Angle to another point (in radians)
    float angle_to(const Point2D& other) const noexcept {
        return std::atan2(other.y - y, other.x - x);
    }
    
    // Linear interpolation
    Point2D lerp(const Point2D& other, float t) const noexcept {
        return Point2D(
            x + (other.x - x) * t,
            y + (other.y - y) * t
        );
    }
    
    // Check if point is within epsilon of another
    bool nearly_equal(const Point2D& other, float epsilon = 1e-6f) const noexcept {
        return std::abs(x - other.x) < epsilon && 
               std::abs(y - other.y) < epsilon;
    }
    
    // Static factory methods
    static constexpr Point2D zero() noexcept {
        return Point2D(0.0f, 0.0f);
    }
    
    static constexpr Point2D one() noexcept {
        return Point2D(1.0f, 1.0f);
    }
    
    static constexpr Point2D unit_x() noexcept {
        return Point2D(1.0f, 0.0f);
    }
    
    static constexpr Point2D unit_y() noexcept {
        return Point2D(0.0f, 1.0f);
    }
};

// Verify Point2D size (POD-like properties are ensured by simple member layout)
static_assert(sizeof(Point2D) == 8, "Point2D must be exactly 8 bytes");

// Non-member operators
inline constexpr Point2D operator*(float scalar, const Point2D& point) noexcept {
    return point * scalar;
}

// Stream output
inline std::ostream& operator<<(std::ostream& os, const Point2D& point) {
    return os << "Point2D(" << point.x << ", " << point.y << ")";
}

// Batch operations for SIMD optimization
namespace batch {

/**
 * Add two arrays of points element-wise.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void add(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept;

/**
 * Subtract two arrays of points element-wise.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void subtract(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept;

/**
 * Scale an array of points by a scalar.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void scale(const Point2D* points, float scalar, Point2D* result, size_t count) noexcept;

/**
 * Calculate distances between corresponding points in two arrays.
 * Arrays must be 16-byte aligned for optimal SIMD performance.
 */
void distances(const Point2D* a, const Point2D* b, float* result, size_t count) noexcept;

} // namespace batch

} // namespace claude_draw