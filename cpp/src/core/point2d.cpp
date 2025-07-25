#include "claude_draw/core/point2d.h"
#include "claude_draw/core/simd.h"
#include <cmath>

#ifdef HAS_AVX2
#include <immintrin.h>
#elif defined(HAS_SSE2)
#include <emmintrin.h>
#endif

namespace claude_draw {
namespace batch {

// Scalar implementations (fallback)
namespace scalar {

void add(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        result[i].x = a[i].x + b[i].x;
        result[i].y = a[i].y + b[i].y;
    }
}

void subtract(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        result[i].x = a[i].x - b[i].x;
        result[i].y = a[i].y - b[i].y;
    }
}

void scale(const Point2D* points, float scalar, Point2D* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        result[i].x = points[i].x * scalar;
        result[i].y = points[i].y * scalar;
    }
}

void distances(const Point2D* a, const Point2D* b, float* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        float dx = a[i].x - b[i].x;
        float dy = a[i].y - b[i].y;
        result[i] = std::sqrt(dx * dx + dy * dy);
    }
}

} // namespace scalar

#ifdef HAS_AVX2
namespace avx2 {

void add(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
    // Process 4 points at a time (256 bits / 64 bits per point = 4 points)
    size_t simd_count = count & ~3;  // Round down to multiple of 4
    
    for (size_t i = 0; i < simd_count; i += 4) {
        // Load 4 points from a (8 floats)
        __m256 va = _mm256_loadu_ps(reinterpret_cast<const float*>(&a[i]));
        // Load 4 points from b (8 floats)
        __m256 vb = _mm256_loadu_ps(reinterpret_cast<const float*>(&b[i]));
        // Add
        __m256 vr = _mm256_add_ps(va, vb);
        // Store result
        _mm256_storeu_ps(reinterpret_cast<float*>(&result[i]), vr);
    }
    
    // Handle remaining points
    for (size_t i = simd_count; i < count; ++i) {
        result[i].x = a[i].x + b[i].x;
        result[i].y = a[i].y + b[i].y;
    }
}

void subtract(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
    size_t simd_count = count & ~3;
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m256 va = _mm256_loadu_ps(reinterpret_cast<const float*>(&a[i]));
        __m256 vb = _mm256_loadu_ps(reinterpret_cast<const float*>(&b[i]));
        __m256 vr = _mm256_sub_ps(va, vb);
        _mm256_storeu_ps(reinterpret_cast<float*>(&result[i]), vr);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
        result[i].x = a[i].x - b[i].x;
        result[i].y = a[i].y - b[i].y;
    }
}

void scale(const Point2D* points, float scalar, Point2D* result, size_t count) noexcept {
    size_t simd_count = count & ~3;
    __m256 vscalar = _mm256_set1_ps(scalar);
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m256 vp = _mm256_loadu_ps(reinterpret_cast<const float*>(&points[i]));
        __m256 vr = _mm256_mul_ps(vp, vscalar);
        _mm256_storeu_ps(reinterpret_cast<float*>(&result[i]), vr);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
        result[i].x = points[i].x * scalar;
        result[i].y = points[i].y * scalar;
    }
}

void distances(const Point2D* a, const Point2D* b, float* result, size_t count) noexcept {
    size_t simd_count = count & ~3;
    
    for (size_t i = 0; i < simd_count; i += 4) {
        // Load points
        __m256 va = _mm256_loadu_ps(reinterpret_cast<const float*>(&a[i]));
        __m256 vb = _mm256_loadu_ps(reinterpret_cast<const float*>(&b[i]));
        
        // Calculate differences
        __m256 vdiff = _mm256_sub_ps(va, vb);
        
        // Square the differences
        __m256 vsquared = _mm256_mul_ps(vdiff, vdiff);
        
        // Sum x² and y² for each point
        // vsquared = [x0² y0² x1² y1² x2² y2² x3² y3²]
        // We need [x0²+y0² x1²+y1² x2²+y2² x3²+y3²]
        __m256 vshuffled = _mm256_permute_ps(vsquared, 0xB1); // Swap pairs
        __m256 vsums = _mm256_add_ps(vsquared, vshuffled);
        
        // Extract the sums (every other element)
        __m128 vlow = _mm256_extractf128_ps(vsums, 0);
        __m128 vhigh = _mm256_extractf128_ps(vsums, 1);
        __m128 vdist_squared = _mm_shuffle_ps(vlow, vhigh, 0x88); // 10001000b
        
        // Calculate square roots
        __m128 vdist = _mm_sqrt_ps(vdist_squared);
        
        // Store results
        _mm_storeu_ps(&result[i], vdist);
    }
    
    // Handle remaining points
    for (size_t i = simd_count; i < count; ++i) {
        float dx = a[i].x - b[i].x;
        float dy = a[i].y - b[i].y;
        result[i] = std::sqrt(dx * dx + dy * dy);
    }
}

} // namespace avx2
#endif // HAS_AVX2

#ifdef HAS_SSE2
namespace sse2 {

void add(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
    // Process 2 points at a time (128 bits / 64 bits per point = 2 points)
    size_t simd_count = count & ~1;  // Round down to multiple of 2
    
    for (size_t i = 0; i < simd_count; i += 2) {
        // Load 2 points from a (4 floats)
        __m128 va = _mm_loadu_ps(reinterpret_cast<const float*>(&a[i]));
        // Load 2 points from b (4 floats)
        __m128 vb = _mm_loadu_ps(reinterpret_cast<const float*>(&b[i]));
        // Add
        __m128 vr = _mm_add_ps(va, vb);
        // Store result
        _mm_storeu_ps(reinterpret_cast<float*>(&result[i]), vr);
    }
    
    // Handle remaining point
    if (count & 1) {
        size_t i = count - 1;
        result[i].x = a[i].x + b[i].x;
        result[i].y = a[i].y + b[i].y;
    }
}

void subtract(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
    size_t simd_count = count & ~1;
    
    for (size_t i = 0; i < simd_count; i += 2) {
        __m128 va = _mm_loadu_ps(reinterpret_cast<const float*>(&a[i]));
        __m128 vb = _mm_loadu_ps(reinterpret_cast<const float*>(&b[i]));
        __m128 vr = _mm_sub_ps(va, vb);
        _mm_storeu_ps(reinterpret_cast<float*>(&result[i]), vr);
    }
    
    if (count & 1) {
        size_t i = count - 1;
        result[i].x = a[i].x - b[i].x;
        result[i].y = a[i].y - b[i].y;
    }
}

void scale(const Point2D* points, float scalar, Point2D* result, size_t count) noexcept {
    size_t simd_count = count & ~1;
    __m128 vscalar = _mm_set1_ps(scalar);
    
    for (size_t i = 0; i < simd_count; i += 2) {
        __m128 vp = _mm_loadu_ps(reinterpret_cast<const float*>(&points[i]));
        __m128 vr = _mm_mul_ps(vp, vscalar);
        _mm_storeu_ps(reinterpret_cast<float*>(&result[i]), vr);
    }
    
    if (count & 1) {
        size_t i = count - 1;
        result[i].x = points[i].x * scalar;
        result[i].y = points[i].y * scalar;
    }
}

void distances(const Point2D* a, const Point2D* b, float* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        __m128 va = _mm_set_ps(0, 0, a[i].y, a[i].x);
        __m128 vb = _mm_set_ps(0, 0, b[i].y, b[i].x);
        __m128 vdiff = _mm_sub_ps(va, vb);
        __m128 vsquared = _mm_mul_ps(vdiff, vdiff);
        
        // Sum x² and y²
        __m128 vshuffled = _mm_shuffle_ps(vsquared, vsquared, 0x01);
        __m128 vsum = _mm_add_ss(vsquared, vshuffled);
        __m128 vdist = _mm_sqrt_ss(vsum);
        
        _mm_store_ss(&result[i], vdist);
    }
}

} // namespace sse2
#endif // HAS_SSE2

// Dispatch to best available implementation
void add(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
#ifdef HAS_AVX2
    if (SimdCapabilities::has_avx2()) {
        avx2::add(a, b, result, count);
        return;
    }
#endif
#ifdef HAS_SSE2
    if (SimdCapabilities::has_sse2()) {
        sse2::add(a, b, result, count);
        return;
    }
#endif
    scalar::add(a, b, result, count);
}

void subtract(const Point2D* a, const Point2D* b, Point2D* result, size_t count) noexcept {
#ifdef HAS_AVX2
    if (SimdCapabilities::has_avx2()) {
        avx2::subtract(a, b, result, count);
        return;
    }
#endif
#ifdef HAS_SSE2
    if (SimdCapabilities::has_sse2()) {
        sse2::subtract(a, b, result, count);
        return;
    }
#endif
    scalar::subtract(a, b, result, count);
}

void scale(const Point2D* points, float scalar, Point2D* result, size_t count) noexcept {
#ifdef HAS_AVX2
    if (SimdCapabilities::has_avx2()) {
        avx2::scale(points, scalar, result, count);
        return;
    }
#endif
#ifdef HAS_SSE2
    if (SimdCapabilities::has_sse2()) {
        sse2::scale(points, scalar, result, count);
        return;
    }
#endif
    scalar::scale(points, scalar, result, count);
}

void distances(const Point2D* a, const Point2D* b, float* result, size_t count) noexcept {
#ifdef HAS_AVX2
    if (SimdCapabilities::has_avx2()) {
        avx2::distances(a, b, result, count);
        return;
    }
#endif
#ifdef HAS_SSE2
    if (SimdCapabilities::has_sse2()) {
        sse2::distances(a, b, result, count);
        return;
    }
#endif
    scalar::distances(a, b, result, count);
}

} // namespace batch
} // namespace claude_draw