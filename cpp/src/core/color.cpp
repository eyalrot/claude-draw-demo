#include "claude_draw/core/color.h"
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

void blend_over(const Color* src, const Color* dst, Color* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        result[i] = src[i].blend_over(dst[i]);
    }
}

void premultiply(const Color* colors, Color* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        result[i] = colors[i].premultiply();
    }
}

void to_grayscale(const Color* colors, Color* result, size_t count) noexcept {
    for (size_t i = 0; i < count; ++i) {
        // Using standard luminance weights: 0.299*R + 0.587*G + 0.114*B
        uint8_t gray = static_cast<uint8_t>(
            0.299f * colors[i].r + 
            0.587f * colors[i].g + 
            0.114f * colors[i].b + 0.5f
        );
        result[i] = Color(gray, gray, gray, colors[i].a);
    }
}

} // namespace scalar

#ifdef HAS_AVX2
namespace avx2 {

void blend_over(const Color* src, const Color* dst, Color* result, size_t count) noexcept {
    // Process 8 colors at a time
    size_t simd_count = count & ~7;
    
    for (size_t i = 0; i < simd_count; i += 8) {
        // Load 8 colors (32 bytes)
        __m256i vsrc = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&src[i]));
        __m256i vdst = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&dst[i]));
        
        // Unpack to 16-bit for calculations
        __m256i vsrc_lo = _mm256_unpacklo_epi8(vsrc, _mm256_setzero_si256());
        __m256i vsrc_hi = _mm256_unpackhi_epi8(vsrc, _mm256_setzero_si256());
        __m256i vdst_lo = _mm256_unpacklo_epi8(vdst, _mm256_setzero_si256());
        __m256i vdst_hi = _mm256_unpackhi_epi8(vdst, _mm256_setzero_si256());
        
        // Extract alpha channels (every 4th byte)
        __m256i valpha_lo = _mm256_shuffle_epi8(vsrc_lo, 
            _mm256_set_epi8(15,15, 15,15, 15,15, 15,15, 7,7, 7,7, 7,7, 7,7,
                           15,15, 15,15, 15,15, 15,15, 7,7, 7,7, 7,7, 7,7));
        __m256i valpha_hi = _mm256_shuffle_epi8(vsrc_hi,
            _mm256_set_epi8(15,15, 15,15, 15,15, 15,15, 7,7, 7,7, 7,7, 7,7,
                           15,15, 15,15, 15,15, 15,15, 7,7, 7,7, 7,7, 7,7));
        
        // inv_alpha = 255 - alpha
        __m256i vinv_alpha_lo = _mm256_sub_epi16(_mm256_set1_epi16(255), valpha_lo);
        __m256i vinv_alpha_hi = _mm256_sub_epi16(_mm256_set1_epi16(255), valpha_hi);
        
        // result = (src * alpha + dst * inv_alpha) / 255
        __m256i vresult_lo = _mm256_add_epi16(
            _mm256_mullo_epi16(vsrc_lo, valpha_lo),
            _mm256_mullo_epi16(vdst_lo, vinv_alpha_lo)
        );
        __m256i vresult_hi = _mm256_add_epi16(
            _mm256_mullo_epi16(vsrc_hi, valpha_hi),
            _mm256_mullo_epi16(vdst_hi, vinv_alpha_hi)
        );
        
        // Divide by 255 (approximation using multiplication by reciprocal)
        const __m256i reciprocal = _mm256_set1_epi16(static_cast<int16_t>(0x8081u));
        vresult_lo = _mm256_mulhi_epu16(vresult_lo, reciprocal);
        vresult_hi = _mm256_mulhi_epu16(vresult_hi, reciprocal);
        vresult_lo = _mm256_srli_epi16(vresult_lo, 7);
        vresult_hi = _mm256_srli_epi16(vresult_hi, 7);
        
        // Pack back to bytes
        __m256i vresult = _mm256_packus_epi16(vresult_lo, vresult_hi);
        
        // Store result
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vresult);
    }
    
    // Handle remaining colors
    for (size_t i = simd_count; i < count; ++i) {
        result[i] = src[i].blend_over(dst[i]);
    }
}

void premultiply(const Color* colors, Color* result, size_t count) noexcept {
    size_t simd_count = count & ~7;
    
    for (size_t i = 0; i < simd_count; i += 8) {
        __m256i vcolors = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&colors[i]));
        
        // Unpack to 16-bit
        __m256i vlo = _mm256_unpacklo_epi8(vcolors, _mm256_setzero_si256());
        __m256i vhi = _mm256_unpackhi_epi8(vcolors, _mm256_setzero_si256());
        
        // Extract alpha
        __m256i valpha_lo = _mm256_shuffle_epi8(vlo,
            _mm256_set_epi8(15,15, 13,13, 11,11, 9,9, 7,7, 5,5, 3,3, 1,1,
                           15,15, 13,13, 11,11, 9,9, 7,7, 5,5, 3,3, 1,1));
        __m256i valpha_hi = _mm256_shuffle_epi8(vhi,
            _mm256_set_epi8(15,15, 13,13, 11,11, 9,9, 7,7, 5,5, 3,3, 1,1,
                           15,15, 13,13, 11,11, 9,9, 7,7, 5,5, 3,3, 1,1));
        
        // Multiply RGB by alpha
        vlo = _mm256_mullo_epi16(vlo, valpha_lo);
        vhi = _mm256_mullo_epi16(vhi, valpha_hi);
        
        // Divide by 255
        const __m256i reciprocal = _mm256_set1_epi16(static_cast<int16_t>(0x8081u));
        vlo = _mm256_mulhi_epu16(vlo, reciprocal);
        vhi = _mm256_mulhi_epu16(vhi, reciprocal);
        vlo = _mm256_srli_epi16(vlo, 7);
        vhi = _mm256_srli_epi16(vhi, 7);
        
        // Pack back
        __m256i vresult = _mm256_packus_epi16(vlo, vhi);
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vresult);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
        result[i] = colors[i].premultiply();
    }
}

void to_grayscale(const Color* colors, Color* result, size_t count) noexcept {
    size_t simd_count = count & ~7;
    
    // Luminance weights as 16-bit integers (scaled by 32768)
    const __m256i weight_r = _mm256_set1_epi16(9798);   // 0.299 * 32768
    const __m256i weight_g = _mm256_set1_epi16(19235);  // 0.587 * 32768
    const __m256i weight_b = _mm256_set1_epi16(3735);   // 0.114 * 32768
    
    for (size_t i = 0; i < simd_count; i += 8) {
        __m256i vcolors = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&colors[i]));
        
        // Separate RGBA channels
        __m256i vr = _mm256_and_si256(vcolors, _mm256_set1_epi32(0x000000FF));
        __m256i vg = _mm256_and_si256(_mm256_srli_epi32(vcolors, 8), _mm256_set1_epi32(0x000000FF));
        __m256i vb = _mm256_and_si256(_mm256_srli_epi32(vcolors, 16), _mm256_set1_epi32(0x000000FF));
        __m256i va = _mm256_and_si256(_mm256_srli_epi32(vcolors, 24), _mm256_set1_epi32(0x000000FF));
        
        // Convert to 16-bit for multiplication
        __m256i vr16 = _mm256_packus_epi32(vr, _mm256_setzero_si256());
        __m256i vg16 = _mm256_packus_epi32(vg, _mm256_setzero_si256());
        __m256i vb16 = _mm256_packus_epi32(vb, _mm256_setzero_si256());
        
        // Calculate grayscale: 0.299*R + 0.587*G + 0.114*B
        __m256i vgray = _mm256_add_epi16(
            _mm256_mulhi_epu16(vr16, weight_r),
            _mm256_add_epi16(
                _mm256_mulhi_epu16(vg16, weight_g),
                _mm256_mulhi_epu16(vb16, weight_b)
            )
        );
        
        // Scale back and pack as RGBA with gray in RGB channels
        vgray = _mm256_srli_epi16(vgray, 7);
        __m256i vgray32 = _mm256_unpacklo_epi16(vgray, _mm256_setzero_si256());
        
        // Combine gray value in RGB channels with original alpha
        __m256i vresult = _mm256_or_si256(
            _mm256_or_si256(vgray32, _mm256_slli_epi32(vgray32, 8)),
            _mm256_or_si256(_mm256_slli_epi32(vgray32, 16), _mm256_slli_epi32(va, 24))
        );
        
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(&result[i]), vresult);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
        uint8_t gray = static_cast<uint8_t>(
            0.299f * colors[i].r + 
            0.587f * colors[i].g + 
            0.114f * colors[i].b + 0.5f
        );
        result[i] = Color(gray, gray, gray, colors[i].a);
    }
}

} // namespace avx2
#endif // HAS_AVX2

#ifdef HAS_SSE2
namespace sse2 {

void blend_over(const Color* src, const Color* dst, Color* result, size_t count) noexcept {
    // Process 4 colors at a time
    size_t simd_count = count & ~3;
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m128i vsrc = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&src[i]));
        __m128i vdst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&dst[i]));
        
        // Unpack to 16-bit
        __m128i vsrc_lo = _mm_unpacklo_epi8(vsrc, _mm_setzero_si128());
        __m128i vsrc_hi = _mm_unpackhi_epi8(vsrc, _mm_setzero_si128());
        __m128i vdst_lo = _mm_unpacklo_epi8(vdst, _mm_setzero_si128());
        __m128i vdst_hi = _mm_unpackhi_epi8(vdst, _mm_setzero_si128());
        
        // Simple approximation for SSE2
        // TODO: Implement proper alpha blending for SSE2
        
        _mm_storeu_si128(reinterpret_cast<__m128i*>(&result[i]), vsrc);
    }
    
    for (size_t i = simd_count; i < count; ++i) {
        result[i] = src[i].blend_over(dst[i]);
    }
}

void premultiply(const Color* colors, Color* result, size_t count) noexcept {
    size_t simd_count = count & ~3;
    
    for (size_t i = 0; i < simd_count; i += 4) {
        __m128i vcolors = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&colors[i]));
        
        // For now, use scalar fallback for SSE2
        // TODO: Implement SSE2 version
        for (size_t j = 0; j < 4; ++j) {
            result[i + j] = colors[i + j].premultiply();
        }
    }
    
    for (size_t i = simd_count; i < count; ++i) {
        result[i] = colors[i].premultiply();
    }
}

void to_grayscale(const Color* colors, Color* result, size_t count) noexcept {
    // Use scalar implementation for SSE2
    scalar::to_grayscale(colors, result, count);
}

} // namespace sse2
#endif // HAS_SSE2

// Dispatch to best available implementation
void blend_over(const Color* src, const Color* dst, Color* result, size_t count) noexcept {
    // TODO: Fix SIMD implementations - they have precision issues
    // For now, use scalar implementation
    scalar::blend_over(src, dst, result, count);
}

void premultiply(const Color* colors, Color* result, size_t count) noexcept {
    // TODO: Fix SIMD implementations - they have precision issues
    // For now, use scalar implementation
    scalar::premultiply(colors, result, count);
}

void to_grayscale(const Color* colors, Color* result, size_t count) noexcept {
    // TODO: Fix SIMD implementations - they have precision issues
    // For now, use scalar implementation
    scalar::to_grayscale(colors, result, count);
}

} // namespace batch
} // namespace claude_draw