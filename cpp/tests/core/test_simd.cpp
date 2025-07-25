#include <gtest/gtest.h>
#include "claude_draw/core/simd.h"
#include <iostream>

using namespace claude_draw;

TEST(SimdCapabilities, Detection) {
    // Initialize SIMD detection
    SimdCapabilities::initialize();
    
    // Print capabilities for information
    std::cout << SimdCapabilities::get_capabilities_string() << std::endl;
    
    // At minimum, we should have SSE2 on x86_64
#if defined(__x86_64__) || defined(_M_X64)
    EXPECT_TRUE(SimdCapabilities::has_sse2());
#endif
    
    // Test that we can query all features without crashes
    SimdCapabilities::has_sse3();
    SimdCapabilities::has_ssse3();
    SimdCapabilities::has_sse41();
    SimdCapabilities::has_sse42();
    SimdCapabilities::has_avx();
    SimdCapabilities::has_avx2();
    SimdCapabilities::has_avx512f();
    SimdCapabilities::has_fma();
}

TEST(SimdCapabilities, SimdLevel) {
    SimdLevel level = get_simd_level();
    
    // Should at least have scalar
    EXPECT_GE(static_cast<int>(level), static_cast<int>(SimdLevel::SCALAR));
    
    // If we have AVX2, we should also have AVX
    if (level == SimdLevel::AVX2) {
        EXPECT_TRUE(SimdCapabilities::has_avx());
    }
    
    // If we have AVX, we should also have SSE
    if (static_cast<int>(level) >= static_cast<int>(SimdLevel::AVX)) {
        EXPECT_TRUE(SimdCapabilities::has_sse2());
    }
}

TEST(SimdAlignment, AlignmentCheck) {
    // Test alignment detection
    alignas(32) int aligned_array[8];
    
    EXPECT_TRUE(is_aligned(aligned_array, 32));
    
    // Test with different alignments
    EXPECT_TRUE(is_aligned(aligned_array, 16));
    EXPECT_TRUE(is_aligned(aligned_array, 8));
    EXPECT_TRUE(is_aligned(aligned_array, 4));
}

TEST(SimdAlignment, AlignedAllocation) {
    // Test aligned allocation
    void* ptr = simd_aligned_alloc(1024, 32);
    ASSERT_NE(ptr, nullptr);
    EXPECT_TRUE(is_aligned(ptr, 32));
    
    // Test that we can write to the memory
    float* fptr = static_cast<float*>(ptr);
    for (int i = 0; i < 256; ++i) {
        fptr[i] = static_cast<float>(i);
    }
    
    simd_aligned_free(ptr);
    
    // Test with different alignments
    void* ptr64 = simd_aligned_alloc(512, 64);
    ASSERT_NE(ptr64, nullptr);
    EXPECT_TRUE(is_aligned(ptr64, 64));
    simd_aligned_free(ptr64);
}

TEST(SimdCapabilities, MultipleInitialization) {
    // Test that multiple initializations don't cause issues
    SimdCapabilities::initialize();
    SimdCapabilities::initialize();
    SimdCapabilities::initialize();
    
    // Should still work correctly
    const char* caps1 = SimdCapabilities::get_capabilities_string();
    const char* caps2 = SimdCapabilities::get_capabilities_string();
    EXPECT_STREQ(caps1, caps2);
}