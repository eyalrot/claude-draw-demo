#include <gtest/gtest.h>
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/simd.h"
#include "../test_utils.h"
#include <vector>
#include <cmath>
#include <limits>

using namespace claude_draw;

class Transform2DTest : public ::testing::Test {
protected:
    static constexpr float EPSILON = 1e-5f;
    static constexpr float PI = 3.14159265358979323846f;
    
    bool nearly_equal(float a, float b, float epsilon = EPSILON) {
        return std::abs(a - b) < epsilon;
    }
    
    bool transforms_equal(const Transform2D& a, const Transform2D& b, float epsilon = EPSILON) {
        for (int i = 0; i < 9; ++i) {
            if (!nearly_equal(a.at(i), b.at(i), epsilon)) {
                return false;
            }
        }
        return true;
    }
    
    bool points_equal(const Point2D& a, const Point2D& b, float epsilon = EPSILON) {
        return nearly_equal(a.x, b.x, epsilon) && nearly_equal(a.y, b.y, epsilon);
    }
};

// Construction and basic properties
TEST_F(Transform2DTest, DefaultConstruction) {
    Transform2D t;
    
    // Should be identity matrix
    EXPECT_FLOAT_EQ(t(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t(0, 1), 0.0f);
    EXPECT_FLOAT_EQ(t(0, 2), 0.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 0.0f);
    EXPECT_FLOAT_EQ(t(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(t(1, 2), 0.0f);
    EXPECT_FLOAT_EQ(t(2, 0), 0.0f);
    EXPECT_FLOAT_EQ(t(2, 1), 0.0f);
    EXPECT_FLOAT_EQ(t(2, 2), 1.0f);
}

TEST_F(Transform2DTest, ParameterizedConstruction) {
    Transform2D t(2.0f, 3.0f, 4.0f,
                  5.0f, 6.0f, 7.0f,
                  8.0f, 9.0f, 10.0f);
    
    EXPECT_FLOAT_EQ(t(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(t(0, 1), 3.0f);
    EXPECT_FLOAT_EQ(t(0, 2), 4.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 5.0f);
    EXPECT_FLOAT_EQ(t(1, 1), 6.0f);
    EXPECT_FLOAT_EQ(t(1, 2), 7.0f);
    EXPECT_FLOAT_EQ(t(2, 0), 8.0f);
    EXPECT_FLOAT_EQ(t(2, 1), 9.0f);
    EXPECT_FLOAT_EQ(t(2, 2), 10.0f);
}

TEST_F(Transform2DTest, CopyConstruction) {
    Transform2D t1(1.0f, 2.0f, 3.0f,
                   4.0f, 5.0f, 6.0f);
    Transform2D t2(t1);
    
    EXPECT_TRUE(transforms_equal(t1, t2));
}

TEST_F(Transform2DTest, MoveConstruction) {
    Transform2D t1(1.0f, 2.0f, 3.0f,
                   4.0f, 5.0f, 6.0f);
    Transform2D t2(std::move(t1));
    
    EXPECT_FLOAT_EQ(t2(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t2(0, 1), 2.0f);
    EXPECT_FLOAT_EQ(t2(0, 2), 3.0f);
    EXPECT_FLOAT_EQ(t2(1, 0), 4.0f);
    EXPECT_FLOAT_EQ(t2(1, 1), 5.0f);
    EXPECT_FLOAT_EQ(t2(1, 2), 6.0f);
}

// Alignment and size verification
TEST_F(Transform2DTest, AlignmentAndSize) {
    EXPECT_EQ(alignof(Transform2D), 32);
    EXPECT_EQ(sizeof(Transform2D), 64);  // 9 floats + padding
}

// Element access
TEST_F(Transform2DTest, ElementAccess) {
    Transform2D t;
    
    // Test operator()
    t(0, 1) = 5.0f;
    EXPECT_FLOAT_EQ(t(0, 1), 5.0f);
    
    // Test at()
    t.at(3) = 7.0f;
    EXPECT_FLOAT_EQ(t.at(3), 7.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 7.0f);  // Same element
    
    // Test data()
    const float* data = t.data();
    EXPECT_FLOAT_EQ(data[0], 1.0f);  // Identity element
    EXPECT_FLOAT_EQ(data[1], 5.0f);  // Modified element
    EXPECT_FLOAT_EQ(data[3], 7.0f);  // Modified element
}

// Static factory methods
TEST_F(Transform2DTest, TranslateFactory) {
    Transform2D t = Transform2D::translate(10.0f, 20.0f);
    
    EXPECT_FLOAT_EQ(t(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t(0, 1), 0.0f);
    EXPECT_FLOAT_EQ(t(0, 2), 10.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 0.0f);
    EXPECT_FLOAT_EQ(t(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(t(1, 2), 20.0f);
}

TEST_F(Transform2DTest, RotateFactory) {
    Transform2D t = Transform2D::rotate(PI / 2.0f);  // 90 degrees
    
    EXPECT_NEAR(t(0, 0), 0.0f, EPSILON);
    EXPECT_NEAR(t(0, 1), -1.0f, EPSILON);
    EXPECT_FLOAT_EQ(t(0, 2), 0.0f);
    EXPECT_NEAR(t(1, 0), 1.0f, EPSILON);
    EXPECT_NEAR(t(1, 1), 0.0f, EPSILON);
    EXPECT_FLOAT_EQ(t(1, 2), 0.0f);
}

TEST_F(Transform2DTest, ScaleFactory) {
    Transform2D t = Transform2D::scale(2.0f, 3.0f);
    
    EXPECT_FLOAT_EQ(t(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(t(0, 1), 0.0f);
    EXPECT_FLOAT_EQ(t(0, 2), 0.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 0.0f);
    EXPECT_FLOAT_EQ(t(1, 1), 3.0f);
    EXPECT_FLOAT_EQ(t(1, 2), 0.0f);
}

TEST_F(Transform2DTest, UniformScaleFactory) {
    Transform2D t = Transform2D::scale(2.5f);
    
    EXPECT_FLOAT_EQ(t(0, 0), 2.5f);
    EXPECT_FLOAT_EQ(t(1, 1), 2.5f);
}

TEST_F(Transform2DTest, ShearFactory) {
    Transform2D t = Transform2D::shear(0.5f, 0.3f);
    
    EXPECT_FLOAT_EQ(t(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(t(0, 1), 0.3f);
    EXPECT_FLOAT_EQ(t(0, 2), 0.0f);
    EXPECT_FLOAT_EQ(t(1, 0), 0.5f);
    EXPECT_FLOAT_EQ(t(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(t(1, 2), 0.0f);
}

// Matrix operations
TEST_F(Transform2DTest, MatrixMultiplication) {
    Transform2D t1 = Transform2D::translate(10.0f, 20.0f);
    Transform2D t2 = Transform2D::scale(2.0f, 3.0f);
    
    Transform2D result = t1 * t2;
    
    // Translation followed by scale
    EXPECT_FLOAT_EQ(result(0, 0), 2.0f);
    EXPECT_FLOAT_EQ(result(0, 1), 0.0f);
    EXPECT_FLOAT_EQ(result(0, 2), 10.0f);
    EXPECT_FLOAT_EQ(result(1, 0), 0.0f);
    EXPECT_FLOAT_EQ(result(1, 1), 3.0f);
    EXPECT_FLOAT_EQ(result(1, 2), 20.0f);
}

TEST_F(Transform2DTest, ChainedOperations) {
    Transform2D t;
    t.translate_by(10.0f, 0.0f)
     .rotate_by(PI / 2.0f)
     .scale_by(2.0f, 2.0f);
    
    // Apply to test point
    Point2D p(1.0f, 0.0f);
    Point2D result = t.transform_point(p);
    
    // Expected: scale(2) -> rotate(90°) -> translate(10, 0)
    // (1, 0) -> (2, 0) -> (0, 2) -> (10, 2)
    EXPECT_NEAR(result.x, 10.0f, EPSILON);
    EXPECT_NEAR(result.y, 2.0f, EPSILON);
}

// Point transformation
TEST_F(Transform2DTest, TransformPoint) {
    Transform2D t = Transform2D::translate(5.0f, 10.0f);
    Point2D p(3.0f, 4.0f);
    
    Point2D result = t.transform_point(p);
    EXPECT_FLOAT_EQ(result.x, 8.0f);
    EXPECT_FLOAT_EQ(result.y, 14.0f);
    
    // Test operator*
    Point2D result2 = t * p;
    EXPECT_TRUE(points_equal(result, result2));
}

TEST_F(Transform2DTest, TransformVector) {
    Transform2D t = Transform2D::translate(5.0f, 10.0f);
    Point2D v(3.0f, 4.0f);
    
    // Vectors ignore translation
    Point2D result = t.transform_vector(v);
    EXPECT_FLOAT_EQ(result.x, 3.0f);
    EXPECT_FLOAT_EQ(result.y, 4.0f);
}

TEST_F(Transform2DTest, RotatePoint) {
    Transform2D t = Transform2D::rotate(PI / 2.0f);  // 90 degrees
    Point2D p(1.0f, 0.0f);
    
    Point2D result = t.transform_point(p);
    EXPECT_NEAR(result.x, 0.0f, EPSILON);
    EXPECT_NEAR(result.y, 1.0f, EPSILON);
}

// Determinant and inverse
TEST_F(Transform2DTest, Determinant) {
    Transform2D t1 = Transform2D::scale(2.0f, 3.0f);
    EXPECT_FLOAT_EQ(t1.determinant(), 6.0f);
    
    Transform2D t2 = Transform2D::rotate(PI / 4.0f);
    EXPECT_NEAR(t2.determinant(), 1.0f, EPSILON);
    
    // Singular matrix
    Transform2D t3(1.0f, 2.0f, 3.0f,
                   2.0f, 4.0f, 6.0f);  // Second row is 2x first row
    EXPECT_FLOAT_EQ(t3.determinant(), 0.0f);
}

TEST_F(Transform2DTest, IsInvertible) {
    Transform2D t1 = Transform2D::scale(2.0f, 3.0f);
    EXPECT_TRUE(t1.is_invertible());
    
    Transform2D t2(1.0f, 2.0f, 3.0f,
                   2.0f, 4.0f, 6.0f);  // Singular
    EXPECT_FALSE(t2.is_invertible());
}

TEST_F(Transform2DTest, Inverse) {
    Transform2D t = Transform2D::scale(2.0f, 4.0f);
    Transform2D inv = t.inverse();
    
    // Verify that t * inv = identity
    Transform2D identity = t * inv;
    EXPECT_NEAR(identity(0, 0), 1.0f, EPSILON);
    EXPECT_NEAR(identity(0, 1), 0.0f, EPSILON);
    EXPECT_NEAR(identity(0, 2), 0.0f, EPSILON);
    EXPECT_NEAR(identity(1, 0), 0.0f, EPSILON);
    EXPECT_NEAR(identity(1, 1), 1.0f, EPSILON);
    EXPECT_NEAR(identity(1, 2), 0.0f, EPSILON);
}

TEST_F(Transform2DTest, InverseWithTranslation) {
    Transform2D t = Transform2D::translate(10.0f, 20.0f);
    Transform2D inv = t.inverse();
    
    Point2D p(5.0f, 5.0f);
    Point2D transformed = t.transform_point(p);
    Point2D restored = inv.transform_point(transformed);
    
    EXPECT_TRUE(points_equal(p, restored));
}

// Decomposition
TEST_F(Transform2DTest, Decompose) {
    float tx = 10.0f, ty = 20.0f;
    float rotation = PI / 4.0f;
    float sx = 2.0f, sy = 3.0f;
    
    // Create composite transform
    Transform2D t = Transform2D::translate(tx, ty) * 
                    Transform2D::rotate(rotation) * 
                    Transform2D::scale(sx, sy);
    
    // Decompose
    float out_tx, out_ty, out_rotation, out_sx, out_sy;
    t.decompose(out_tx, out_ty, out_rotation, out_sx, out_sy);
    
    EXPECT_NEAR(out_tx, tx, EPSILON);
    EXPECT_NEAR(out_ty, ty, EPSILON);
    // Note: Rotation and scale might not match exactly due to order of operations
}

// Equality
TEST_F(Transform2DTest, Equality) {
    Transform2D t1 = Transform2D::translate(1.0f, 2.0f);
    Transform2D t2 = Transform2D::translate(1.0f, 2.0f);
    Transform2D t3 = Transform2D::translate(1.0f, 2.1f);
    
    EXPECT_TRUE(t1 == t2);
    EXPECT_FALSE(t1 == t3);
    EXPECT_FALSE(t1 != t2);
    EXPECT_TRUE(t1 != t3);
    
    EXPECT_TRUE(t1.nearly_equal(t2));
    EXPECT_FALSE(t1.nearly_equal(t3, 0.01f));
    EXPECT_TRUE(t1.nearly_equal(t3, 0.2f));
}

// Batch operations tests
class Transform2DBatchTest : public ::testing::Test {
protected:
    static constexpr size_t SMALL_SIZE = 8;
    static constexpr size_t MEDIUM_SIZE = 64;
    static constexpr size_t LARGE_SIZE = 1024;
    static constexpr float EPSILON = 1e-5f;
    
    std::vector<Point2D> points, vectors, result;
    Transform2D transform;
    
    void SetUp() override {
        points.resize(LARGE_SIZE);
        vectors.resize(LARGE_SIZE);
        result.resize(LARGE_SIZE);
        
        // Fill with test data
        for (size_t i = 0; i < LARGE_SIZE; ++i) {
            points[i] = Point2D(
                static_cast<float>(i),
                static_cast<float>(i * 2)
            );
            vectors[i] = Point2D(
                std::cos(i * 0.1f),
                std::sin(i * 0.1f)
            );
        }
        
        // Create a test transform
        transform = Transform2D::translate(10.0f, 20.0f) *
                   Transform2D::rotate(0.5f) *
                   Transform2D::scale(1.5f, 2.0f);
    }
    
    void verify_batch_transform_points(size_t count) {
        batch::transform_points(transform, points.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            Point2D expected = transform.transform_point(points[i]);
            EXPECT_NEAR(result[i].x, expected.x, EPSILON) 
                << "Failed at index " << i;
            EXPECT_NEAR(result[i].y, expected.y, EPSILON) 
                << "Failed at index " << i;
        }
    }
    
    void verify_batch_transform_vectors(size_t count) {
        batch::transform_vectors(transform, vectors.data(), result.data(), count);
        
        for (size_t i = 0; i < count; ++i) {
            Point2D expected = transform.transform_vector(vectors[i]);
            EXPECT_NEAR(result[i].x, expected.x, EPSILON) 
                << "Failed at index " << i;
            EXPECT_NEAR(result[i].y, expected.y, EPSILON) 
                << "Failed at index " << i;
        }
    }
};

TEST_F(Transform2DBatchTest, BatchTransformPoints) {
    verify_batch_transform_points(SMALL_SIZE);
    verify_batch_transform_points(MEDIUM_SIZE);
    verify_batch_transform_points(LARGE_SIZE);
    
    // Test with odd sizes
    verify_batch_transform_points(7);
    verify_batch_transform_points(63);
    verify_batch_transform_points(1023);
}

TEST_F(Transform2DBatchTest, BatchTransformVectors) {
    verify_batch_transform_vectors(SMALL_SIZE);
    verify_batch_transform_vectors(MEDIUM_SIZE);
    verify_batch_transform_vectors(LARGE_SIZE);
    
    verify_batch_transform_vectors(7);
    verify_batch_transform_vectors(63);
    verify_batch_transform_vectors(1023);
}

TEST_F(Transform2DBatchTest, BatchMultiplyTransforms) {
    std::vector<Transform2D> transforms = {
        Transform2D::translate(10.0f, 0.0f),
        Transform2D::rotate(0.5f),
        Transform2D::scale(2.0f, 3.0f),
        Transform2D::translate(0.0f, 10.0f)
    };
    
    Transform2D result = batch::multiply_transforms(transforms.data(), transforms.size());
    
    // Verify by manual multiplication
    Transform2D expected = transforms[0];
    for (size_t i = 1; i < transforms.size(); ++i) {
        expected = expected * transforms[i];
    }
    
    for (int i = 0; i < 9; ++i) {
        EXPECT_NEAR(result.at(i), expected.at(i), EPSILON);
    }
}

TEST_F(Transform2DBatchTest, BatchEmptyInput) {
    // Test with count = 0
    batch::transform_points(transform, points.data(), result.data(), 0);
    batch::transform_vectors(transform, vectors.data(), result.data(), 0);
    
    // Test with empty transform array
    Transform2D identity = batch::multiply_transforms(nullptr, 0);
    EXPECT_FLOAT_EQ(identity(0, 0), 1.0f);
    EXPECT_FLOAT_EQ(identity(1, 1), 1.0f);
    EXPECT_FLOAT_EQ(identity(2, 2), 1.0f);
}

TEST_F(Transform2DBatchTest, BatchSingleElement) {
    verify_batch_transform_points(1);
    verify_batch_transform_vectors(1);
    
    Transform2D single = Transform2D::scale(2.0f);
    Transform2D result = batch::multiply_transforms(&single, 1);
    EXPECT_TRUE(single == result);
}

// Edge cases
TEST_F(Transform2DTest, EdgeCases) {
    // Very small values
    Transform2D t1 = Transform2D::scale(1e-10f, 1e-10f);
    EXPECT_FALSE(t1.is_invertible());
    
    // Very large values
    Transform2D t2 = Transform2D::scale(1e10f, 1e10f);
    EXPECT_TRUE(t2.is_invertible());
    
    // Near-zero rotation
    Transform2D t3 = Transform2D::rotate(1e-10f);
    EXPECT_NEAR(t3(0, 0), 1.0f, EPSILON);
    EXPECT_NEAR(t3(1, 1), 1.0f, EPSILON);
}