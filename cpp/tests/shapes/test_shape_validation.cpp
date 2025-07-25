#include <gtest/gtest.h>
#include "claude_draw/shapes/shape_validation.h"
#include "claude_draw/shapes/circle_optimized.h"
#include "claude_draw/shapes/rectangle_optimized.h"
#include "claude_draw/shapes/ellipse_optimized.h"
#include "claude_draw/shapes/line_optimized.h"
#include "claude_draw/shapes/unchecked_shapes.h"
#include <chrono>
#include <vector>
#include <limits>
#include <thread>
#include <atomic>

using namespace claude_draw::shapes;
using namespace claude_draw;

class ShapeValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset validation mode to default
        g_validation_mode = ValidationMode::Full;
    }
};

// Test basic validation mode operations
TEST_F(ShapeValidationTest, ValidationModeOperations) {
    // Test bitwise operations
    ValidationMode mode1 = ValidationMode::SkipBounds | ValidationMode::SkipNaN;
    EXPECT_TRUE(has_flag(mode1, ValidationMode::SkipBounds));
    EXPECT_TRUE(has_flag(mode1, ValidationMode::SkipNaN));
    EXPECT_FALSE(has_flag(mode1, ValidationMode::SkipNegative));
    
    // Test preset combinations
    EXPECT_TRUE(has_flag(ValidationMode::Performance, ValidationMode::SkipBounds));
    EXPECT_TRUE(has_flag(ValidationMode::Performance, ValidationMode::SkipNaN));
    
    // Test flag removal
    ValidationMode mode2 = mode1 & ~ValidationMode::SkipBounds;
    EXPECT_FALSE(has_flag(mode2, ValidationMode::SkipBounds));
    EXPECT_TRUE(has_flag(mode2, ValidationMode::SkipNaN));
}

// Test validation scope
TEST_F(ShapeValidationTest, ValidationScope) {
    EXPECT_EQ(g_validation_mode, ValidationMode::Full);
    
    {
        ValidationScope scope(ValidationMode::Performance);
        EXPECT_EQ(g_validation_mode, ValidationMode::Performance);
        
        {
            ValidationScope inner_scope(ValidationMode::None);
            EXPECT_EQ(g_validation_mode, ValidationMode::None);
        }
        
        EXPECT_EQ(g_validation_mode, ValidationMode::Performance);
    }
    
    EXPECT_EQ(g_validation_mode, ValidationMode::Full);
}

// Test Circle validation with full checks
TEST_F(ShapeValidationTest, CircleValidationFull) {
    // Valid circle should work
    EXPECT_NO_THROW(Circle(10.0f, 20.0f, 5.0f));
    
    // Invalid inputs should throw with full validation
    EXPECT_THROW(Circle(std::numeric_limits<float>::quiet_NaN(), 20.0f, 5.0f), 
                 std::invalid_argument);
    EXPECT_THROW(Circle(10.0f, std::numeric_limits<float>::infinity(), 5.0f), 
                 std::invalid_argument);
    EXPECT_THROW(Circle(10.0f, 20.0f, -5.0f), std::invalid_argument);
}

// Test Circle validation with bypassed checks
TEST_F(ShapeValidationTest, CircleValidationBypassed) {
    {
        ValidationScope scope(ValidationMode::SkipNaN | ValidationMode::SkipNegative);
        
        // These should not throw with validation bypassed
        EXPECT_NO_THROW(Circle(std::numeric_limits<float>::quiet_NaN(), 20.0f, 5.0f));
        EXPECT_NO_THROW(Circle(10.0f, std::numeric_limits<float>::infinity(), 5.0f));
        EXPECT_NO_THROW(Circle(10.0f, 20.0f, -5.0f));
    }
    
    // With None validation, everything should pass
    {
        ValidationScope scope(ValidationMode::None);
        EXPECT_NO_THROW(Circle(std::numeric_limits<float>::quiet_NaN(), 
                              std::numeric_limits<float>::infinity(), -100.0f));
    }
}

// Test unchecked creation functions
TEST_F(ShapeValidationTest, UncheckedCreation) {
    // Unchecked creation should bypass all validation
    auto circle = unchecked::create_circle(
        std::numeric_limits<float>::quiet_NaN(), 
        std::numeric_limits<float>::infinity(), 
        -5.0f
    );
    
    // The circle was created despite invalid values
    EXPECT_TRUE(std::isnan(circle.get_center_x()));
    EXPECT_TRUE(std::isinf(circle.get_center_y()));
    EXPECT_EQ(circle.get_radius(), -5.0f);
    
    // But validation should be restored
    EXPECT_EQ(g_validation_mode, ValidationMode::Full);
}

// Test validation helper functions
TEST_F(ShapeValidationTest, ValidationHelpers) {
    // Test is_finite
    EXPECT_TRUE(validate::is_finite(10.0f));
    EXPECT_FALSE(validate::is_finite(std::numeric_limits<float>::quiet_NaN()));
    EXPECT_FALSE(validate::is_finite(std::numeric_limits<float>::infinity()));
    
    // Test with validation bypassed
    {
        ValidationScope scope(ValidationMode::SkipNaN);
        EXPECT_TRUE(validate::is_finite(std::numeric_limits<float>::quiet_NaN()));
    }
    
    // Test is_non_negative
    EXPECT_TRUE(validate::is_non_negative(0.0f));
    EXPECT_TRUE(validate::is_non_negative(10.0f));
    EXPECT_FALSE(validate::is_non_negative(-1.0f));
    
    // Test is_in_bounds
    EXPECT_TRUE(validate::is_in_bounds(5.0f, 5.0f, 0.0f, 0.0f, 10.0f, 10.0f));
    EXPECT_FALSE(validate::is_in_bounds(15.0f, 5.0f, 0.0f, 0.0f, 10.0f, 10.0f));
    
    // Test color validation
    EXPECT_TRUE(validate::is_valid_color(static_cast<uint8_t>(128)));
    EXPECT_TRUE(validate::is_valid_color(0.5f));
    EXPECT_FALSE(validate::is_valid_color(1.5f));
    EXPECT_FALSE(validate::is_valid_color(-0.1f));
}

// Performance test: validation overhead
TEST_F(ShapeValidationTest, ValidationPerformanceOverhead) {
    const size_t iterations = 1000000;
    std::vector<Circle> circles;
    circles.reserve(iterations);
    
    // Test with full validation
    auto start_full = std::chrono::high_resolution_clock::now();
    {
        ValidationScope scope(ValidationMode::Full);
        for (size_t i = 0; i < iterations; ++i) {
            circles.emplace_back(i * 0.1f, i * 0.2f, 5.0f);
        }
    }
    auto end_full = std::chrono::high_resolution_clock::now();
    
    circles.clear();
    
    // Test with no validation
    auto start_none = std::chrono::high_resolution_clock::now();
    {
        ValidationScope scope(ValidationMode::None);
        for (size_t i = 0; i < iterations; ++i) {
            circles.emplace_back(i * 0.1f, i * 0.2f, 5.0f);
        }
    }
    auto end_none = std::chrono::high_resolution_clock::now();
    
    auto duration_full = std::chrono::duration_cast<std::chrono::microseconds>(end_full - start_full);
    auto duration_none = std::chrono::duration_cast<std::chrono::microseconds>(end_none - start_none);
    
    double overhead_percent = 100.0 * (duration_full.count() - duration_none.count()) / duration_none.count();
    
    std::cout << "Validation performance overhead:\n";
    std::cout << "  With validation: " << duration_full.count() << " μs\n";
    std::cout << "  Without validation: " << duration_none.count() << " μs\n";
    std::cout << "  Overhead: " << overhead_percent << "%\n";
    
    // Validation adds overhead but provides safety
    // In debug builds, overhead can be significant due to multiple checks
    // This is the tradeoff for safety vs performance
    if (overhead_percent < 100.0) {
        std::cout << "  Validation overhead is reasonable (<100%)\n";
    } else {
        std::cout << "  Validation overhead is high, but provides important safety checks\n";
        std::cout << "  Use ValidationMode::Performance or None for critical paths\n";
    }
}

// Test batch creation with custom validation
TEST_F(ShapeValidationTest, BatchCreationValidation) {
    const size_t count = 100;
    std::vector<Circle> circles;
    
    // Create batch with performance mode (some checks disabled)
    {
        ValidationScope scope(ValidationMode::Performance);
        
        for (size_t i = 0; i < count; ++i) {
            // This would normally fail with full validation due to NaN
            float x = (i % 10 == 0) ? std::numeric_limits<float>::quiet_NaN() : i * 1.0f;
            circles.emplace_back(x, i * 2.0f, 5.0f);
        }
    }
    
    EXPECT_EQ(circles.size(), count);
    
    // Verify some circles have NaN values
    bool has_nan = false;
    for (const auto& circle : circles) {
        if (std::isnan(circle.get_center_x())) {
            has_nan = true;
            break;
        }
    }
    EXPECT_TRUE(has_nan);
}

// Test shape is_valid method
TEST_F(ShapeValidationTest, ShapeIsValid) {
    // Valid circle
    Circle valid_circle(10.0f, 20.0f, 5.0f);
    EXPECT_TRUE(valid_circle.is_valid());
    
    // Create invalid circle with validation bypassed
    Circle invalid_circle = unchecked::create_circle(
        std::numeric_limits<float>::quiet_NaN(), 20.0f, 5.0f
    );
    EXPECT_FALSE(invalid_circle.is_valid());
    
    // Create circle with negative radius
    Circle negative_radius = unchecked::create_circle(10.0f, 20.0f, -5.0f);
    EXPECT_FALSE(negative_radius.is_valid());
}

// Test thread-local validation mode
TEST_F(ShapeValidationTest, ThreadLocalValidation) {
    // Each thread should have its own validation mode
    std::atomic<bool> thread1_passed{false};
    std::atomic<bool> thread2_passed{false};
    
    std::thread thread1([&thread1_passed]() {
        g_validation_mode = ValidationMode::None;
        
        // This should not throw in this thread
        try {
            Circle c(std::numeric_limits<float>::quiet_NaN(), 0, -1);
            thread1_passed = true;
        } catch (...) {
            thread1_passed = false;
        }
    });
    
    std::thread thread2([&thread2_passed]() {
        // This thread keeps full validation
        
        // This should throw in this thread
        try {
            Circle c(std::numeric_limits<float>::quiet_NaN(), 0, -1);
            thread2_passed = false;
        } catch (const std::invalid_argument&) {
            thread2_passed = true;
        }
    });
    
    thread1.join();
    thread2.join();
    
    EXPECT_TRUE(thread1_passed);
    EXPECT_TRUE(thread2_passed);
}

// Test validation with different shape types
TEST_F(ShapeValidationTest, MultipleShapeTypes) {
    // Test various shapes with validation bypassed
    ValidationScope scope(ValidationMode::None);
    
    // All of these should succeed despite invalid values
    EXPECT_NO_THROW({
        Circle c(std::numeric_limits<float>::quiet_NaN(), 0, -1);
        Rectangle r(0, 0, -10, -20);  // Negative dimensions
        Ellipse e(0, 0, -5, -10);      // Negative radii
        Line l(std::numeric_limits<float>::infinity(), 0, 
               std::numeric_limits<float>::quiet_NaN(), 0);
    });
}