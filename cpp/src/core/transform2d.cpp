#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/simd.h"

#ifdef HAS_AVX2
#include <immintrin.h>
#elif defined(HAS_SSE2)
#include <emmintrin.h>
#endif

namespace claude_draw {
namespace batch {

// Scalar implementations (fallback)
namespace scalar {

void transform_points(const Transform2D& transform, const Point2D* points, 
                     Point2D* result, size_t count) noexcept {
    const float* m = transform.data();
    
    for (size_t i = 0; i < count; ++i) {
        result[i].x = m[0] * points[i].x + m[1] * points[i].y + m[2];
        result[i].y = m[3] * points[i].x + m[4] * points[i].y + m[5];
    }
}

void transform_vectors(const Transform2D& transform, const Point2D* vectors, 
                      Point2D* result, size_t count) noexcept {
    const float* m = transform.data();
    
    for (size_t i = 0; i < count; ++i) {
        result[i].x = m[0] * vectors[i].x + m[1] * vectors[i].y;
        result[i].y = m[3] * vectors[i].x + m[4] * vectors[i].y;
    }
}

Transform2D multiply_transforms(const Transform2D* transforms, size_t count) noexcept {
    if (count == 0) {
        return Transform2D();  // Identity
    }
    
    Transform2D result = transforms[0];
    for (size_t i = 1; i < count; ++i) {
        result = result * transforms[i];
    }
    
    return result;
}

} // namespace scalar

#ifdef HAS_AVX2
namespace avx2 {

void transform_points(const Transform2D& transform, const Point2D* points, 
                     Point2D* result, size_t count) noexcept {
    // Use scalar implementation for now
    scalar::transform_points(transform, points, result, count);
}

void transform_vectors(const Transform2D& transform, const Point2D* vectors, 
                      Point2D* result, size_t count) noexcept {
    // Use scalar implementation for now
    scalar::transform_vectors(transform, vectors, result, count);
}

} // namespace avx2
#endif // HAS_AVX2

#ifdef HAS_SSE2
namespace sse2 {

void transform_points(const Transform2D& transform, const Point2D* points, 
                     Point2D* result, size_t count) noexcept {
    // Use scalar implementation for now
    scalar::transform_points(transform, points, result, count);
}

void transform_vectors(const Transform2D& transform, const Point2D* vectors, 
                      Point2D* result, size_t count) noexcept {
    // Use scalar implementation for now
    scalar::transform_vectors(transform, vectors, result, count);
}

} // namespace sse2
#endif // HAS_SSE2

// Dispatch to best available implementation
void transform_points(const Transform2D& transform, const Point2D* points, 
                     Point2D* result, size_t count) noexcept {
    // TODO: Fix SIMD implementations - they need proper optimization
    // For now, use scalar implementation
    scalar::transform_points(transform, points, result, count);
}

void transform_vectors(const Transform2D& transform, const Point2D* vectors, 
                      Point2D* result, size_t count) noexcept {
    // TODO: Fix SIMD implementations - they need proper optimization
    // For now, use scalar implementation
    scalar::transform_vectors(transform, vectors, result, count);
}

Transform2D multiply_transforms(const Transform2D* transforms, size_t count) noexcept {
    // For now, use scalar implementation
    // TODO: Implement SIMD version for matrix multiplication
    return scalar::multiply_transforms(transforms, count);
}

} // namespace batch
} // namespace claude_draw