#pragma once

#include <cstdint>
#include <cmath>
#include <stdexcept>

namespace claude_draw {
namespace shapes {

// Forward declarations
class Circle;
class Rectangle;
class Ellipse;
class Line;

/**
 * @brief Shape validation control flags
 * 
 * These flags allow bypassing various validation checks for performance-critical
 * code paths. Use with caution - invalid data may cause undefined behavior when
 * validation is disabled.
 */
enum class ValidationMode : uint32_t {
    // Full validation (default)
    Full = 0x0000,
    
    // Skip individual checks
    SkipBounds = 0x0001,        // Skip bounds checking
    SkipNaN = 0x0002,           // Skip NaN/Inf checks
    SkipNegative = 0x0004,      // Skip negative dimension checks
    SkipColorRange = 0x0008,    // Skip color value range checks
    SkipTransform = 0x0010,     // Skip transform matrix validation
    SkipIntersection = 0x0020,  // Skip intersection validation
    
    // Preset combinations
    Minimal = SkipColorRange | SkipTransform,  // Skip non-critical checks
    Performance = SkipBounds | SkipNaN | SkipColorRange | SkipTransform,  // Maximum performance
    None = 0xFFFF  // Skip all validation (dangerous!)
};

// Bitwise operators for ValidationMode
inline ValidationMode operator|(ValidationMode a, ValidationMode b) {
    return static_cast<ValidationMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ValidationMode operator&(ValidationMode a, ValidationMode b) {
    return static_cast<ValidationMode>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline ValidationMode operator~(ValidationMode a) {
    return static_cast<ValidationMode>(~static_cast<uint32_t>(a));
}

inline bool has_flag(ValidationMode mode, ValidationMode flag) {
    return (mode & flag) == flag;
}

/**
 * @brief Global validation mode for shape operations
 * 
 * This can be set per-thread or globally to control validation behavior.
 * Default is Full validation.
 */
thread_local inline ValidationMode g_validation_mode = ValidationMode::Full;

/**
 * @brief RAII helper to temporarily change validation mode
 */
class ValidationScope {
private:
    ValidationMode previous_mode_;
    
public:
    explicit ValidationScope(ValidationMode mode) 
        : previous_mode_(g_validation_mode) {
        g_validation_mode = mode;
    }
    
    ~ValidationScope() {
        g_validation_mode = previous_mode_;
    }
    
    // Non-copyable
    ValidationScope(const ValidationScope&) = delete;
    ValidationScope& operator=(const ValidationScope&) = delete;
};

/**
 * @brief Validation helper functions
 */
namespace validate {

// Check if a value is finite (not NaN or Inf)
template<typename T>
inline bool is_finite(T value) {
    if (has_flag(g_validation_mode, ValidationMode::SkipNaN)) {
        return true;
    }
    return std::isfinite(value);
}

// Check if a dimension is non-negative
template<typename T>
inline bool is_non_negative(T value) {
    if (has_flag(g_validation_mode, ValidationMode::SkipNegative)) {
        return true;
    }
    return value >= T(0);
}

// Check if a point is within bounds
inline bool is_in_bounds(float x, float y, float min_x, float min_y, float max_x, float max_y) {
    if (has_flag(g_validation_mode, ValidationMode::SkipBounds)) {
        return true;
    }
    return x >= min_x && x <= max_x && y >= min_y && y <= max_y;
}

// Check if a color component is valid (0-255 for uint8_t)
inline bool is_valid_color(uint8_t value) {
    if (has_flag(g_validation_mode, ValidationMode::SkipColorRange)) {
        return true;
    }
    return true;  // uint8_t is always in valid range
}

// Check if a color component is valid (0.0-1.0 for float)
inline bool is_valid_color(float value) {
    if (has_flag(g_validation_mode, ValidationMode::SkipColorRange)) {
        return true;
    }
    return value >= 0.0f && value <= 1.0f;
}

// Validate all components of a shape
template<typename ShapeT>
inline bool validate_shape(const ShapeT& shape) {
    if (g_validation_mode == ValidationMode::None) {
        return true;
    }
    
    // Shape-specific validation would be implemented here
    return shape.is_valid();
}

} // namespace validate

/**
 * @brief Macro for conditional validation
 * 
 * Usage: VALIDATE_IF_ENABLED(validate::is_finite(x), "Invalid x coordinate");
 */
#define VALIDATE_IF_ENABLED(condition, message) \
    do { \
        if (g_validation_mode != ValidationMode::None && !(condition)) { \
            throw std::invalid_argument(message); \
        } \
    } while(0)

/**
 * @brief Create shapes without validation
 * 
 * These factory functions create shapes with validation temporarily disabled
 * for maximum performance. Use only with known-good data.
 * 
 * Note: The actual implementation must be in a source file or after the full
 * shape definitions are available.
 */
namespace unchecked {

// Template function for creating any shape without validation
// Implementation must be instantiated where full shape definitions are available
template<typename ShapeT, typename... Args>
inline ShapeT create(Args&&... args) {
    ValidationScope scope(ValidationMode::None);
    return ShapeT(std::forward<Args>(args)...);
}

} // namespace unchecked

/**
 * @brief Batch operations with custom validation mode
 */
namespace batch_validated {

template<typename ShapeT>
void create_batch(ShapeT* output, const float* coords, size_t count, 
                  ValidationMode mode = ValidationMode::Performance) {
    ValidationScope scope(mode);
    
    #pragma omp parallel for
    for (size_t i = 0; i < count; ++i) {
        // Shape-specific batch creation would be implemented here
        // This is a placeholder showing the pattern
    }
}

} // namespace batch_validated

} // namespace shapes
} // namespace claude_draw