#include <gtest/gtest.h>
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/simd.h"
#include "../test_utils.h"
#include <vector>
#include <cmath>
#include <limits>
#include <cstddef>
#include <cstdint>

using namespace claude_draw;

class Point2DTest : public ::testing::Test {
protected:
    static constexpr float EPSILON = 1e-6f;
    
    bool nearly_equal(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
};

// Construction and basic properties
TEST_F(Point2DTest, DefaultConstruction) {
    Point2D p;
    EXPECT_FLOAT_EQ(p.x, 0.0f);
    EXPECT_FLOAT_EQ(p.y, 0.0f);
}

TEST_F(Point2DTest, ParameterizedConstruction) {
    Point2D p(3.5f, -2.1f);
    EXPECT_FLOAT_EQ(p.x, 3.5f);
    EXPECT_FLOAT_EQ(p.y, -2.1f);
}

TEST_F(Point2DTest, CopyConstruction) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(p1);
    EXPECT_FLOAT_EQ(p2.x, 1.0f);
    EXPECT_FLOAT_EQ(p2.y, 2.0f);
}

TEST_F(Point2DTest, MoveConstruction) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(std::move(p1));
    EXPECT_FLOAT_EQ(p2.x, 1.0f);
    EXPECT_FLOAT_EQ(p2.y, 2.0f);
}

TEST_F(Point2DTest, CopyAssignment) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2;
    p2 = p1;
    EXPECT_FLOAT_EQ(p2.x, 1.0f);
    EXPECT_FLOAT_EQ(p2.y, 2.0f);
}

TEST_F(Point2DTest, MoveAssignment) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2;
    p2 = std::move(p1);
    EXPECT_FLOAT_EQ(p2.x, 1.0f);
    EXPECT_FLOAT_EQ(p2.y, 2.0f);
}

// Size verification
TEST_F(Point2DTest, SizeVerification) {
    EXPECT_EQ(sizeof(Point2D), 8);
    // Ensure no padding between members
    EXPECT_EQ(offsetof(Point2D, y), sizeof(float));
}

// Arithmetic operations
TEST_F(Point2DTest, Addition) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(3.0f, 4.0f);
    Point2D result = p1 + p2;
    EXPECT_FLOAT_EQ(result.x, 4.0f);
    EXPECT_FLOAT_EQ(result.y, 6.0f);
}

TEST_F(Point2DTest, Subtraction) {
    Point2D p1(5.0f, 7.0f);
    Point2D p2(2.0f, 3.0f);
    Point2D result = p1 - p2;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST_F(Point2DTest, ScalarMultiplication) {
    Point2D p(2.0f, 3.0f);
    Point2D result1 = p * 2.5f;
    EXPECT_FLOAT_EQ(result1.x, 5.0f);
    EXPECT_FLOAT_EQ(result1.y, 7.5f);
    
    Point2D result2 = 2.5f * p;  // Non-member operator
    EXPECT_FLOAT_EQ(result2.x, 5.0f);
    EXPECT_FLOAT_EQ(result2.y, 7.5f);
}

TEST_F(Point2DTest, ScalarDivision) {
    Point2D p(6.0f, 8.0f);
    Point2D result = p / 2.0f;
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST_F(Point2DTest, UnaryNegation) {
    Point2D p(3.0f, -4.0f);
    Point2D result = -p;
    EXPECT_FLOAT_EQ(result.x, -3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

// Compound assignment operators
TEST_F(Point2DTest, CompoundAddition) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(3.0f, 4.0f);
    p1 += p2;
    EXPECT_FLOAT_EQ(p1.x, 4.0f);
    EXPECT_FLOAT_EQ(p1.y, 6.0f);
}

TEST_F(Point2DTest, CompoundSubtraction) {
    Point2D p1(5.0f, 7.0f);
    Point2D p2(2.0f, 3.0f);
    p1 -= p2;
    EXPECT_FLOAT_EQ(p1.x, 3.0f);
    EXPECT_FLOAT_EQ(p1.y, 4.0f);
}

TEST_F(Point2DTest, CompoundScalarMultiplication) {
    Point2D p(2.0f, 3.0f);
    p *= 2.5f;
    EXPECT_FLOAT_EQ(p.x, 5.0f);
    EXPECT_FLOAT_EQ(p.y, 7.5f);
}

TEST_F(Point2DTest, CompoundScalarDivision) {
    Point2D p(6.0f, 8.0f);
    p /= 2.0f;
    EXPECT_FLOAT_EQ(p.x, 3.0f);
    EXPECT_FLOAT_EQ(p.y, 4.0f);
}

// Equality operators
TEST_F(Point2DTest, Equality) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(1.0f, 2.0f);
    Point2D p3(1.1f, 2.0f);
    
    EXPECT_TRUE(p1 == p2);
    EXPECT_FALSE(p1 == p3);
}

TEST_F(Point2DTest, Inequality) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(1.0f, 2.0f);
    Point2D p3(1.1f, 2.0f);
    
    EXPECT_FALSE(p1 != p2);
    EXPECT_TRUE(p1 != p3);
}

// Distance calculations
TEST_F(Point2DTest, DistanceTo) {
    Point2D p1(0.0f, 0.0f);
    Point2D p2(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(p1.distance_to(p2), 5.0f);
    
    Point2D p3(1.0f, 1.0f);
    Point2D p4(4.0f, 5.0f);
    EXPECT_FLOAT_EQ(p3.distance_to(p4), 5.0f);
}

TEST_F(Point2DTest, DistanceSquaredTo) {
    Point2D p1(0.0f, 0.0f);
    Point2D p2(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(p1.distance_squared_to(p2), 25.0f);
}

// Magnitude calculations
TEST_F(Point2DTest, Magnitude) {
    Point2D p1(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(p1.magnitude(), 5.0f);
    
    Point2D p2(-3.0f, -4.0f);
    EXPECT_FLOAT_EQ(p2.magnitude(), 5.0f);
}

TEST_F(Point2DTest, MagnitudeSquared) {
    Point2D p(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(p.magnitude_squared(), 25.0f);
}

// Normalization
TEST_F(Point2DTest, Normalized) {
    Point2D p(3.0f, 4.0f);
    Point2D n = p.normalized();
    EXPECT_NEAR(n.x, 0.6f, EPSILON);
    EXPECT_NEAR(n.y, 0.8f, EPSILON);
    EXPECT_NEAR(n.magnitude(), 1.0f, EPSILON);
}

TEST_F(Point2DTest, NormalizedZeroVector) {
    Point2D p(0.0f, 0.0f);
    Point2D n = p.normalized();
    EXPECT_FLOAT_EQ(n.x, 0.0f);
    EXPECT_FLOAT_EQ(n.y, 0.0f);
}

// Dot and cross products
TEST_F(Point2DTest, DotProduct) {
    Point2D p1(2.0f, 3.0f);
    Point2D p2(4.0f, 5.0f);
    EXPECT_FLOAT_EQ(p1.dot(p2), 23.0f);  // 2*4 + 3*5 = 8 + 15 = 23
}

TEST_F(Point2DTest, CrossProduct) {
    Point2D p1(2.0f, 3.0f);
    Point2D p2(4.0f, 5.0f);
    EXPECT_FLOAT_EQ(p1.cross(p2), -2.0f);  // 2*5 - 3*4 = 10 - 12 = -2
}

// Angle calculation
TEST_F(Point2DTest, AngleTo) {
    Point2D p1(0.0f, 0.0f);
    Point2D p2(1.0f, 0.0f);
    EXPECT_FLOAT_EQ(p1.angle_to(p2), 0.0f);
    
    Point2D p3(0.0f, 1.0f);
    EXPECT_FLOAT_EQ(p1.angle_to(p3), M_PI / 2.0f);
    
    Point2D p4(-1.0f, 0.0f);
    EXPECT_FLOAT_EQ(p1.angle_to(p4), M_PI);
}

// Linear interpolation
TEST_F(Point2DTest, Lerp) {
    Point2D p1(0.0f, 0.0f);
    Point2D p2(10.0f, 20.0f);
    
    Point2D r1 = p1.lerp(p2, 0.0f);
    EXPECT_FLOAT_EQ(r1.x, 0.0f);
    EXPECT_FLOAT_EQ(r1.y, 0.0f);
    
    Point2D r2 = p1.lerp(p2, 0.5f);
    EXPECT_FLOAT_EQ(r2.x, 5.0f);
    EXPECT_FLOAT_EQ(r2.y, 10.0f);
    
    Point2D r3 = p1.lerp(p2, 1.0f);
    EXPECT_FLOAT_EQ(r3.x, 10.0f);
    EXPECT_FLOAT_EQ(r3.y, 20.0f);
}

// Nearly equal comparison
TEST_F(Point2DTest, NearlyEqual) {
    Point2D p1(1.0f, 2.0f);
    Point2D p2(1.0f + 1e-7f, 2.0f + 1e-7f);
    Point2D p3(1.1f, 2.0f);
    
    EXPECT_TRUE(p1.nearly_equal(p2));
    EXPECT_FALSE(p1.nearly_equal(p3));
    EXPECT_TRUE(p1.nearly_equal(p3, 0.2f));
}

// Static factory methods
TEST_F(Point2DTest, FactoryMethods) {
    Point2D zero = Point2D::zero();
    EXPECT_FLOAT_EQ(zero.x, 0.0f);
    EXPECT_FLOAT_EQ(zero.y, 0.0f);
    
    Point2D one = Point2D::one();
    EXPECT_FLOAT_EQ(one.x, 1.0f);
    EXPECT_FLOAT_EQ(one.y, 1.0f);
    
    Point2D unit_x = Point2D::unit_x();
    EXPECT_FLOAT_EQ(unit_x.x, 1.0f);
    EXPECT_FLOAT_EQ(unit_x.y, 0.0f);
    
    Point2D unit_y = Point2D::unit_y();
    EXPECT_FLOAT_EQ(unit_y.x, 0.0f);
    EXPECT_FLOAT_EQ(unit_y.y, 1.0f);
}

// Edge cases
TEST_F(Point2DTest, EdgeCases) {
    // Division by zero
    Point2D p(1.0f, 2.0f);
    Point2D r = p / 0.0f;
    EXPECT_TRUE(std::isinf(r.x));
    EXPECT_TRUE(std::isinf(r.y));
    
    // Very large values
    float large = std::numeric_limits<float>::max() / 2.0f;
    Point2D p_large(large, large);
    EXPECT_FLOAT_EQ(p_large.x, large);
    EXPECT_FLOAT_EQ(p_large.y, large);
    
    // Very small values
    float small = std::numeric_limits<float>::min();
    Point2D p_small(small, small);
    EXPECT_FLOAT_EQ(p_small.x, small);
    EXPECT_FLOAT_EQ(p_small.y, small);
}

// Batch operations tests
class Point2DBatchTest : public ::testing::Test {
protected:
    static constexpr size_t SMALL_SIZE = 8;
    static constexpr size_t MEDIUM_SIZE = 64;
    static constexpr size_t LARGE_SIZE = 1024;
    static constexpr float EPSILON = 1e-5f;
    
    std::vector<Point2D> a, b, result;
    std::vector<float> distances;
    
    void SetUp() override {
        // Initialize test data
        a.resize(LARGE_SIZE);
        b.resize(LARGE_SIZE);
        result.resize(LARGE_SIZE);
        distances.resize(LARGE_SIZE);
        
        // Fill with test data
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            a[i] = Point2D(static_cast<float>(i), static_cast<float>(i * 2));
            b[i] = Point2D(static_cast<float>(i * 0.5f), static_cast<float>(i));
        }
    }
    
    void verify_batch_add(size_t count) {
        batch::add(a.data(), b.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            EXPECT_NEAR(result[i].x, a[i].x + b[i].x, EPSILON) 
                << "Failed at index " << i;
            EXPECT_NEAR(result[i].y, a[i].y + b[i].y, EPSILON) 
                << "Failed at index " << i;
        }
    }
    
    void verify_batch_subtract(size_t count) {
        batch::subtract(a.data(), b.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            EXPECT_NEAR(result[i].x, a[i].x - b[i].x, EPSILON) 
                << "Failed at index " << i;
            EXPECT_NEAR(result[i].y, a[i].y - b[i].y, EPSILON) 
                << "Failed at index " << i;
        }
    }
    
    void verify_batch_scale(size_t count, float scalar) {
        batch::scale(a.data(), scalar, result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            EXPECT_NEAR(result[i].x, a[i].x * scalar, EPSILON) 
                << "Failed at index " << i;
            EXPECT_NEAR(result[i].y, a[i].y * scalar, EPSILON) 
                << "Failed at index " << i;
        }
    }
    
    void verify_batch_distances(size_t count) {
        batch::distances(a.data(), b.data(), distances.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            float expected = a[i].distance_to(b[i]);
            EXPECT_NEAR(distances[i], expected, EPSILON) 
                << "Failed at index " << i;
        }
    }
};

TEST_F(Point2DBatchTest, BatchAdd) {
    verify_batch_add(SMALL_SIZE);
    verify_batch_add(MEDIUM_SIZE);
    verify_batch_add(LARGE_SIZE);
    
    // Test with odd sizes (to test remainder handling)
    verify_batch_add(7);
    verify_batch_add(63);
    verify_batch_add(1023);
}

TEST_F(Point2DBatchTest, BatchSubtract) {
    verify_batch_subtract(SMALL_SIZE);
    verify_batch_subtract(MEDIUM_SIZE);
    verify_batch_subtract(LARGE_SIZE);
    
    verify_batch_subtract(7);
    verify_batch_subtract(63);
    verify_batch_subtract(1023);
}

TEST_F(Point2DBatchTest, BatchScale) {
    verify_batch_scale(SMALL_SIZE, 2.5f);
    verify_batch_scale(MEDIUM_SIZE, -1.5f);
    verify_batch_scale(LARGE_SIZE, 0.5f);
    
    verify_batch_scale(7, 3.0f);
    verify_batch_scale(63, -2.0f);
    verify_batch_scale(1023, 0.25f);
}

TEST_F(Point2DBatchTest, BatchDistances) {
    verify_batch_distances(SMALL_SIZE);
    verify_batch_distances(MEDIUM_SIZE);
    verify_batch_distances(LARGE_SIZE);
    
    verify_batch_distances(7);
    verify_batch_distances(63);
    verify_batch_distances(1023);
}

TEST_F(Point2DBatchTest, BatchAlignedMemory) {
    // Test with aligned memory
    constexpr size_t alignment = 32;
    const size_t size_bytes = MEDIUM_SIZE * sizeof(Point2D);
    
    void* aligned_a = simd_aligned_alloc(size_bytes, alignment);
    void* aligned_b = simd_aligned_alloc(size_bytes, alignment);
    void* aligned_result = simd_aligned_alloc(size_bytes, alignment);
    
    ASSERT_NE(aligned_a, nullptr);
    ASSERT_NE(aligned_b, nullptr);
    ASSERT_NE(aligned_result, nullptr);
    
    // Verify alignment
    ASSERT_EQ(reinterpret_cast<uintptr_t>(aligned_a) % alignment, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(aligned_b) % alignment, 0);
    ASSERT_EQ(reinterpret_cast<uintptr_t>(aligned_result) % alignment, 0);
    
    Point2D* pa = static_cast<Point2D*>(aligned_a);
    Point2D* pb = static_cast<Point2D*>(aligned_b);
    Point2D* pr = static_cast<Point2D*>(aligned_result);
    
    // Fill with test data
    for (size_t i = 0; i < MEDIUM_SIZE; ++i) {
        pa[i] = Point2D(static_cast<float>(i), static_cast<float>(i * 2));
        pb[i] = Point2D(static_cast<float>(i * 0.5f), static_cast<float>(i));
    }
    
    // Test batch operations with aligned memory
    batch::add(pa, pb, pr, MEDIUM_SIZE);
    
    for (size_t i = 0; i < MEDIUM_SIZE; ++i) {
        EXPECT_NEAR(pr[i].x, pa[i].x + pb[i].x, EPSILON);
        EXPECT_NEAR(pr[i].y, pa[i].y + pb[i].y, EPSILON);
    }
    
    simd_aligned_free(aligned_a);
    simd_aligned_free(aligned_b);
    simd_aligned_free(aligned_result);
}

TEST_F(Point2DBatchTest, BatchEmptyInput) {
    // Test with count = 0
    batch::add(a.data(), b.data(), result.data(), 0);
    batch::subtract(a.data(), b.data(), result.data(), 0);
    batch::scale(a.data(), 2.0f, result.data(), 0);
    batch::distances(a.data(), b.data(), distances.data(), 0);
    // Should not crash
}

TEST_F(Point2DBatchTest, BatchSingleElement) {
    // Test with count = 1
    verify_batch_add(1);
    verify_batch_subtract(1);
    verify_batch_scale(1, 2.0f);
    verify_batch_distances(1);
}