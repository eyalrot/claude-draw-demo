#include "claude_draw/core/bounding_box.h"
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

void contains_points(const BoundingBox& bb, const Point2D* points, 
                    uint8_t* results, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        results[i] = bb.contains(points[i]) ? 1 : 0;
    }
}

void intersects_boxes(const BoundingBox& bb, const BoundingBox* boxes, 
                     uint8_t* results, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        results[i] = bb.intersects(boxes[i]) ? 1 : 0;
    }
}

BoundingBox union_boxes(const BoundingBox* boxes, size_t count) noexcept {
    if (count == 0) {
        return BoundingBox();
    }
    
    BoundingBox result = boxes[0];
    for (size_t i = 1; i < count; ++i) {
        result.expand(boxes[i]);
    }
    
    return result;
}

void inflate_boxes(const BoundingBox* boxes, BoundingBox* results, 
                   float amount, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        results[i] = boxes[i];
        results[i].inflate(amount);
    }
}

} // namespace scalar

#ifdef HAS_AVX2
namespace avx2 {

void contains_points(const BoundingBox& bb, const Point2D* points, 
                    uint8_t* results, size_t count) noexcept {
    const __m256 min_x = _mm256_set1_ps(bb.min_x);
    const __m256 min_y = _mm256_set1_ps(bb.min_y);
    const __m256 max_x = _mm256_set1_ps(bb.max_x);
    const __m256 max_y = _mm256_set1_ps(bb.max_y);
    
    size_t i = 0;
    
    // Process 8 points at a time
    for (; i + 7 < count; i += 8) {
        // Load 8 points (16 floats total)
        __m256 x_vals = _mm256_set_ps(
            points[i+7].x, points[i+6].x, points[i+5].x, points[i+4].x,
            points[i+3].x, points[i+2].x, points[i+1].x, points[i+0].x
        );
        __m256 y_vals = _mm256_set_ps(
            points[i+7].y, points[i+6].y, points[i+5].y, points[i+4].y,
            points[i+3].y, points[i+2].y, points[i+1].y, points[i+0].y
        );
        
        // Test containment: x >= min_x && x <= max_x && y >= min_y && y <= max_y
        __m256 x_ge_min = _mm256_cmp_ps(x_vals, min_x, _CMP_GE_OQ);
        __m256 x_le_max = _mm256_cmp_ps(x_vals, max_x, _CMP_LE_OQ);
        __m256 y_ge_min = _mm256_cmp_ps(y_vals, min_y, _CMP_GE_OQ);
        __m256 y_le_max = _mm256_cmp_ps(y_vals, max_y, _CMP_LE_OQ);
        
        __m256 x_in_range = _mm256_and_ps(x_ge_min, x_le_max);
        __m256 y_in_range = _mm256_and_ps(y_ge_min, y_le_max);
        __m256 contained = _mm256_and_ps(x_in_range, y_in_range);
        
        // Convert to byte results
        int mask = _mm256_movemask_ps(contained);
        for (int j = 0; j < 8; ++j) {
            results[i + j] = (mask & (1 << j)) ? 1 : 0;
        }
    }
    
    // Handle remaining points
    for (; i < count; ++i) {
        results[i] = bb.contains(points[i]) ? 1 : 0;
    }
}

BoundingBox union_boxes(const BoundingBox* boxes, size_t count) noexcept {
    if (count == 0) {
        return BoundingBox();
    }
    
    // Initialize with first box
    __m256 min_vec = _mm256_set_ps(0, 0, 0, 0, boxes[0].max_y, boxes[0].max_x, boxes[0].min_y, boxes[0].min_x);
    __m256 max_vec = min_vec;
    
    size_t i = 1;
    
    // Process 2 boxes at a time (each box is 4 floats)
    for (; i + 1 < count; i += 2) {
        __m256 box_data = _mm256_set_ps(
            boxes[i+1].max_y, boxes[i+1].max_x, boxes[i+1].min_y, boxes[i+1].min_x,
            boxes[i].max_y, boxes[i].max_x, boxes[i].min_y, boxes[i].min_x
        );
        
        // Update min and max
        min_vec = _mm256_min_ps(min_vec, box_data);
        max_vec = _mm256_max_ps(max_vec, box_data);
    }
    
    // Extract results
    float result[8];
    _mm256_storeu_ps(result, min_vec);
    float result_max[8];
    _mm256_storeu_ps(result_max, max_vec);
    
    BoundingBox final_result(
        std::min(result[0], result[4]),  // min_x
        std::min(result[1], result[5]),  // min_y
        std::max(result_max[2], result_max[6]),  // max_x
        std::max(result_max[3], result_max[7])   // max_y
    );
    
    // Handle remaining boxes
    for (; i < count; ++i) {
        final_result.expand(boxes[i]);
    }
    
    return final_result;
}

} // namespace avx2
#endif // HAS_AVX2

#ifdef HAS_SSE2
namespace sse2 {

void contains_points(const BoundingBox& bb, const Point2D* points, 
                    uint8_t* results, size_t count) noexcept {
    const __m128 min_x = _mm_set1_ps(bb.min_x);
    const __m128 min_y = _mm_set1_ps(bb.min_y);
    const __m128 max_x = _mm_set1_ps(bb.max_x);
    const __m128 max_y = _mm_set1_ps(bb.max_y);
    
    size_t i = 0;
    
    // Process 4 points at a time
    for (; i + 3 < count; i += 4) {
        // Load 4 points
        __m128 x_vals = _mm_set_ps(
            points[i+3].x, points[i+2].x, points[i+1].x, points[i+0].x
        );
        __m128 y_vals = _mm_set_ps(
            points[i+3].y, points[i+2].y, points[i+1].y, points[i+0].y
        );
        
        // Test containment
        __m128 x_ge_min = _mm_cmpge_ps(x_vals, min_x);
        __m128 x_le_max = _mm_cmple_ps(x_vals, max_x);
        __m128 y_ge_min = _mm_cmpge_ps(y_vals, min_y);
        __m128 y_le_max = _mm_cmple_ps(y_vals, max_y);
        
        __m128 x_in_range = _mm_and_ps(x_ge_min, x_le_max);
        __m128 y_in_range = _mm_and_ps(y_ge_min, y_le_max);
        __m128 contained = _mm_and_ps(x_in_range, y_in_range);
        
        // Convert to byte results
        int mask = _mm_movemask_ps(contained);
        for (int j = 0; j < 4; ++j) {
            results[i + j] = (mask & (1 << j)) ? 1 : 0;
        }
    }
    
    // Handle remaining points
    for (; i < count; ++i) {
        results[i] = bb.contains(points[i]) ? 1 : 0;
    }
}

} // namespace sse2
#endif // HAS_SSE2

// Dispatch to best available implementation
void contains_points(const BoundingBox& bb, const Point2D* points, 
                    uint8_t* results, size_t count) noexcept {
#ifdef HAS_AVX2
    if (SimdCapabilities::has_avx2()) {
        avx2::contains_points(bb, points, results, count);
        return;
    }
#endif
#ifdef HAS_SSE2
    if (SimdCapabilities::has_sse2()) {
        sse2::contains_points(bb, points, results, count);
        return;
    }
#endif
    scalar::contains_points(bb, points, results, count);
}

void intersects_boxes(const BoundingBox& bb, const BoundingBox* boxes, 
                     uint8_t* results, size_t count) noexcept {
    // For now, use scalar implementation
    // TODO: Implement SIMD version
    scalar::intersects_boxes(bb, boxes, results, count);
}

BoundingBox union_boxes(const BoundingBox* boxes, size_t count) noexcept {
#ifdef HAS_AVX2
    if (SimdCapabilities::has_avx2()) {
        return avx2::union_boxes(boxes, count);
    }
#endif
    return scalar::union_boxes(boxes, count);
}

void inflate_boxes(const BoundingBox* boxes, BoundingBox* results, 
                   float amount, size_t count) noexcept {
    // For now, use scalar implementation
    // TODO: Implement SIMD version
    scalar::inflate_boxes(boxes, results, amount, count);
}

} // namespace batch
} // namespace claude_draw