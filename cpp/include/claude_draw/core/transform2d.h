#pragma once

#include "point2d.h"
#include <cmath>
#include <ostream>
#include <cstring>

namespace claude_draw {

/**
 * A 2D affine transformation matrix (3x3).
 * 
 * Matrix layout (row-major):
 * | m00  m01  m02 |   | scale_x  shear_y   translate_x |
 * | m10  m11  m12 | = | shear_x  scale_y   translate_y |
 * | m20  m21  m22 |   | 0        0         1           |
 * 
 * This layout is optimized for:
 * - Cache-friendly access patterns
 * - SIMD operations on rows
 * - Direct memory mapping to common graphics APIs
 * 
 * The matrix is stored with 32-byte alignment for optimal SIMD performance.
 */
class alignas(32) Transform2D {
private:
    // Matrix elements in row-major order
    // Padded to 32 bytes for alignment (9 floats + 1 padding)
    float m[9];
    float padding;  // Ensures 32-byte alignment
    
public:
    // Default constructor - identity matrix
    Transform2D() noexcept {
        identity();
    }
    
    // Constructor from matrix elements
    Transform2D(float m00, float m01, float m02,
                float m10, float m11, float m12,
                float m20 = 0.0f, float m21 = 0.0f, float m22 = 1.0f) noexcept {
        m[0] = m00; m[1] = m01; m[2] = m02;
        m[3] = m10; m[4] = m11; m[5] = m12;
        m[6] = m20; m[7] = m21; m[8] = m22;
    }
    
    // Copy constructor
    Transform2D(const Transform2D& other) noexcept {
        std::memcpy(m, other.m, sizeof(m));
    }
    
    // Move constructor
    Transform2D(Transform2D&& other) noexcept {
        std::memcpy(m, other.m, sizeof(m));
    }
    
    // Copy assignment
    Transform2D& operator=(const Transform2D& other) noexcept {
        if (this != &other) {
            std::memcpy(m, other.m, sizeof(m));
        }
        return *this;
    }
    
    // Move assignment
    Transform2D& operator=(Transform2D&& other) noexcept {
        if (this != &other) {
            std::memcpy(m, other.m, sizeof(m));
        }
        return *this;
    }
    
    // Element access (row, column)
    float& operator()(int row, int col) noexcept {
        return m[row * 3 + col];
    }
    
    const float& operator()(int row, int col) const noexcept {
        return m[row * 3 + col];
    }
    
    // Direct element access
    float& at(int index) noexcept {
        return m[index];
    }
    
    const float& at(int index) const noexcept {
        return m[index];
    }
    
    // Get raw pointer to matrix data
    const float* data() const noexcept {
        return m;
    }
    
    float* data() noexcept {
        return m;
    }
    
    // Set to identity matrix
    void identity() noexcept {
        m[0] = 1.0f; m[1] = 0.0f; m[2] = 0.0f;
        m[3] = 0.0f; m[4] = 1.0f; m[5] = 0.0f;
        m[6] = 0.0f; m[7] = 0.0f; m[8] = 1.0f;
    }
    
    // Translation
    static Transform2D translate(float tx, float ty) noexcept {
        return Transform2D(
            1.0f, 0.0f, tx,
            0.0f, 1.0f, ty,
            0.0f, 0.0f, 1.0f
        );
    }
    
    // Rotation (angle in radians)
    static Transform2D rotate(float angle) noexcept {
        float c = std::cos(angle);
        float s = std::sin(angle);
        return Transform2D(
            c, -s, 0.0f,
            s,  c, 0.0f,
            0.0f, 0.0f, 1.0f
        );
    }
    
    // Scale
    static Transform2D scale(float sx, float sy) noexcept {
        return Transform2D(
            sx, 0.0f, 0.0f,
            0.0f, sy, 0.0f,
            0.0f, 0.0f, 1.0f
        );
    }
    
    // Uniform scale
    static Transform2D scale(float s) noexcept {
        return scale(s, s);
    }
    
    // Shear
    static Transform2D shear(float shx, float shy) noexcept {
        return Transform2D(
            1.0f, shy, 0.0f,
            shx, 1.0f, 0.0f,
            0.0f, 0.0f, 1.0f
        );
    }
    
    // Apply translation to this matrix
    Transform2D& translate_by(float tx, float ty) noexcept {
        m[2] += tx * m[0] + ty * m[1];
        m[5] += tx * m[3] + ty * m[4];
        return *this;
    }
    
    // Apply rotation to this matrix
    Transform2D& rotate_by(float angle) noexcept {
        float c = std::cos(angle);
        float s = std::sin(angle);
        
        float new_m0 = c * m[0] - s * m[3];
        float new_m1 = c * m[1] - s * m[4];
        float new_m3 = s * m[0] + c * m[3];
        float new_m4 = s * m[1] + c * m[4];
        
        m[0] = new_m0; m[1] = new_m1;
        m[3] = new_m3; m[4] = new_m4;
        
        return *this;
    }
    
    // Apply scale to this matrix
    Transform2D& scale_by(float sx, float sy) noexcept {
        m[0] *= sx; m[1] *= sx;
        m[3] *= sy; m[4] *= sy;
        return *this;
    }
    
    // Matrix multiplication (this * other)
    Transform2D operator*(const Transform2D& other) const noexcept {
        Transform2D result;
        
        // Row 0
        result.m[0] = m[0] * other.m[0] + m[1] * other.m[3] + m[2] * other.m[6];
        result.m[1] = m[0] * other.m[1] + m[1] * other.m[4] + m[2] * other.m[7];
        result.m[2] = m[0] * other.m[2] + m[1] * other.m[5] + m[2] * other.m[8];
        
        // Row 1
        result.m[3] = m[3] * other.m[0] + m[4] * other.m[3] + m[5] * other.m[6];
        result.m[4] = m[3] * other.m[1] + m[4] * other.m[4] + m[5] * other.m[7];
        result.m[5] = m[3] * other.m[2] + m[4] * other.m[5] + m[5] * other.m[8];
        
        // Row 2 (always [0, 0, 1] for affine transforms)
        result.m[6] = 0.0f;
        result.m[7] = 0.0f;
        result.m[8] = 1.0f;
        
        return result;
    }
    
    // Compound assignment
    Transform2D& operator*=(const Transform2D& other) noexcept {
        *this = *this * other;
        return *this;
    }
    
    // Transform a point
    Point2D transform_point(const Point2D& p) const noexcept {
        return Point2D(
            m[0] * p.x + m[1] * p.y + m[2],
            m[3] * p.x + m[4] * p.y + m[5]
        );
    }
    
    // Transform a vector (ignores translation)
    Point2D transform_vector(const Point2D& v) const noexcept {
        return Point2D(
            m[0] * v.x + m[1] * v.y,
            m[3] * v.x + m[4] * v.y
        );
    }
    
    // Operator for transforming points
    Point2D operator*(const Point2D& p) const noexcept {
        return transform_point(p);
    }
    
    // Calculate determinant
    float determinant() const noexcept {
        return m[0] * m[4] - m[1] * m[3];
    }
    
    // Check if transform is invertible
    bool is_invertible() const noexcept {
        return std::abs(determinant()) > 1e-10f;
    }
    
    // Calculate inverse transform
    Transform2D inverse() const noexcept {
        float det = determinant();
        if (std::abs(det) < 1e-10f) {
            // Return identity for non-invertible matrices
            return Transform2D();
        }
        
        float inv_det = 1.0f / det;
        
        return Transform2D(
            m[4] * inv_det, -m[1] * inv_det, (m[1] * m[5] - m[2] * m[4]) * inv_det,
            -m[3] * inv_det, m[0] * inv_det, (m[2] * m[3] - m[0] * m[5]) * inv_det,
            0.0f, 0.0f, 1.0f
        );
    }
    
    // Decompose into translation, rotation, and scale
    void decompose(float& tx, float& ty, float& rotation, float& sx, float& sy) const noexcept {
        // Translation
        tx = m[2];
        ty = m[5];
        
        // Scale
        sx = std::sqrt(m[0] * m[0] + m[3] * m[3]);
        sy = std::sqrt(m[1] * m[1] + m[4] * m[4]);
        
        // Rotation
        rotation = std::atan2(m[3], m[0]);
    }
    
    // Check if transforms are equal (within epsilon)
    bool nearly_equal(const Transform2D& other, float epsilon = 1e-6f) const noexcept {
        for (int i = 0; i < 9; ++i) {
            if (std::abs(m[i] - other.m[i]) > epsilon) {
                return false;
            }
        }
        return true;
    }
    
    // Equality operators
    bool operator==(const Transform2D& other) const noexcept {
        return nearly_equal(other);
    }
    
    bool operator!=(const Transform2D& other) const noexcept {
        return !nearly_equal(other);
    }
};

// Verify Transform2D alignment and size
static_assert(alignof(Transform2D) == 32, "Transform2D must be 32-byte aligned");
static_assert(sizeof(Transform2D) == 64, "Transform2D must be 64 bytes (padded for alignment)");

// Stream output
inline std::ostream& operator<<(std::ostream& os, const Transform2D& t) {
    os << "Transform2D[\n";
    for (int i = 0; i < 3; ++i) {
        os << "  [";
        for (int j = 0; j < 3; ++j) {
            os << t(i, j);
            if (j < 2) os << ", ";
        }
        os << "]\n";
    }
    os << "]";
    return os;
}

// Batch operations for SIMD optimization
namespace batch {

/**
 * Transform multiple points by the same transform matrix.
 * Points array must be 16-byte aligned for optimal SIMD performance.
 */
void transform_points(const Transform2D& transform, const Point2D* points, 
                     Point2D* result, size_t count) noexcept;

/**
 * Transform multiple vectors by the same transform matrix (ignores translation).
 * Vectors array must be 16-byte aligned for optimal SIMD performance.
 */
void transform_vectors(const Transform2D& transform, const Point2D* vectors, 
                      Point2D* result, size_t count) noexcept;

/**
 * Multiply a series of transform matrices together.
 * Matrices array must be 32-byte aligned for optimal SIMD performance.
 */
Transform2D multiply_transforms(const Transform2D* transforms, size_t count) noexcept;

} // namespace batch

} // namespace claude_draw