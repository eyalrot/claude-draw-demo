#pragma once

#include <cstdint>
#include <cstddef>

namespace claude_draw {

// CPU feature detection
class SimdCapabilities {
public:
    // Runtime detection of CPU features
    static bool has_sse2() noexcept;
    static bool has_sse3() noexcept;
    static bool has_ssse3() noexcept;
    static bool has_sse41() noexcept;
    static bool has_sse42() noexcept;
    static bool has_avx() noexcept;
    static bool has_avx2() noexcept;
    static bool has_avx512f() noexcept;
    static bool has_avx512dq() noexcept;
    static bool has_avx512bw() noexcept;
    static bool has_fma() noexcept;
    
    // Get a summary string of available features
    static const char* get_capabilities_string() noexcept;
    
    // Initialize and cache CPU capabilities (called automatically)
    static void initialize() noexcept;
    
private:
    struct CpuInfo {
        bool sse2 = false;
        bool sse3 = false;
        bool ssse3 = false;
        bool sse41 = false;
        bool sse42 = false;
        bool avx = false;
        bool avx2 = false;
        bool avx512f = false;
        bool avx512dq = false;
        bool avx512bw = false;
        bool fma = false;
        bool initialized = false;
    };
    
    static CpuInfo cpu_info_;
    static void detect_features() noexcept;
};

// SIMD dispatch helpers
enum class SimdLevel {
    SCALAR = 0,
    SSE2 = 1,
    SSE42 = 2,
    AVX = 3,
    AVX2 = 4,
    AVX512 = 5
};

// Get the best available SIMD level
SimdLevel get_simd_level() noexcept;

// Alignment helpers
constexpr std::size_t SIMD_ALIGNMENT = 32;  // AVX2 alignment

template<typename T>
inline bool is_aligned(const T* ptr, std::size_t alignment = SIMD_ALIGNMENT) noexcept {
    return reinterpret_cast<std::uintptr_t>(ptr) % alignment == 0;
}

// Aligned allocation helpers (avoid name collision with C11 aligned_alloc)
void* simd_aligned_alloc(std::size_t size, std::size_t alignment = SIMD_ALIGNMENT) noexcept;
void simd_aligned_free(void* ptr) noexcept;

} // namespace claude_draw