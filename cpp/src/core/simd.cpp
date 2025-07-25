#include "claude_draw/core/simd.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace claude_draw {

SimdCapabilities::CpuInfo SimdCapabilities::cpu_info_;

void SimdCapabilities::detect_features() noexcept {
    if (cpu_info_.initialized) return;
    
#ifdef _MSC_VER
    int info[4];
    __cpuid(info, 0);
    int max_id = info[0];
    
    if (max_id >= 1) {
        __cpuid(info, 1);
        cpu_info_.sse2 = (info[3] & (1 << 26)) != 0;
        cpu_info_.sse3 = (info[2] & (1 << 0)) != 0;
        cpu_info_.ssse3 = (info[2] & (1 << 9)) != 0;
        cpu_info_.sse41 = (info[2] & (1 << 19)) != 0;
        cpu_info_.sse42 = (info[2] & (1 << 20)) != 0;
        cpu_info_.avx = (info[2] & (1 << 28)) != 0;
        cpu_info_.fma = (info[2] & (1 << 12)) != 0;
    }
    
    if (max_id >= 7) {
        __cpuidex(info, 7, 0);
        cpu_info_.avx2 = (info[1] & (1 << 5)) != 0;
        cpu_info_.avx512f = (info[1] & (1 << 16)) != 0;
        cpu_info_.avx512dq = (info[1] & (1 << 17)) != 0;
        cpu_info_.avx512bw = (info[1] & (1 << 30)) != 0;
    }
#else
    unsigned int eax, ebx, ecx, edx;
    unsigned int max_id;
    
    // Get maximum supported CPUID level
    if (__get_cpuid(0, &max_id, &ebx, &ecx, &edx)) {
        if (max_id >= 1) {
            __get_cpuid(1, &eax, &ebx, &ecx, &edx);
            cpu_info_.sse2 = (edx & (1 << 26)) != 0;
            cpu_info_.sse3 = (ecx & (1 << 0)) != 0;
            cpu_info_.ssse3 = (ecx & (1 << 9)) != 0;
            cpu_info_.sse41 = (ecx & (1 << 19)) != 0;
            cpu_info_.sse42 = (ecx & (1 << 20)) != 0;
            cpu_info_.avx = (ecx & (1 << 28)) != 0;
            cpu_info_.fma = (ecx & (1 << 12)) != 0;
        }
        
        if (max_id >= 7) {
            __get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx);
            cpu_info_.avx2 = (ebx & (1 << 5)) != 0;
            cpu_info_.avx512f = (ebx & (1 << 16)) != 0;
            cpu_info_.avx512dq = (ebx & (1 << 17)) != 0;
            cpu_info_.avx512bw = (ebx & (1 << 30)) != 0;
        }
    }
#endif
    
    // Check OS support for AVX
    if (cpu_info_.avx) {
        // Check if OS supports XSAVE/XRSTOR
        unsigned int xcr0 = 0;
#ifdef _MSC_VER
        xcr0 = static_cast<unsigned int>(_xgetbv(0));
#else
        __asm__("xgetbv" : "=a"(xcr0) : "c"(0) : "%edx");
#endif
        // Check if XMM and YMM state are enabled
        if ((xcr0 & 0x6) != 0x6) {
            cpu_info_.avx = false;
            cpu_info_.avx2 = false;
            cpu_info_.avx512f = false;
            cpu_info_.avx512dq = false;
            cpu_info_.avx512bw = false;
        }
    }
    
    cpu_info_.initialized = true;
}

void SimdCapabilities::initialize() noexcept {
    detect_features();
}

bool SimdCapabilities::has_sse2() noexcept {
    detect_features();
    return cpu_info_.sse2;
}

bool SimdCapabilities::has_sse3() noexcept {
    detect_features();
    return cpu_info_.sse3;
}

bool SimdCapabilities::has_ssse3() noexcept {
    detect_features();
    return cpu_info_.ssse3;
}

bool SimdCapabilities::has_sse41() noexcept {
    detect_features();
    return cpu_info_.sse41;
}

bool SimdCapabilities::has_sse42() noexcept {
    detect_features();
    return cpu_info_.sse42;
}

bool SimdCapabilities::has_avx() noexcept {
    detect_features();
    return cpu_info_.avx;
}

bool SimdCapabilities::has_avx2() noexcept {
    detect_features();
    return cpu_info_.avx2;
}

bool SimdCapabilities::has_avx512f() noexcept {
    detect_features();
    return cpu_info_.avx512f;
}

bool SimdCapabilities::has_avx512dq() noexcept {
    detect_features();
    return cpu_info_.avx512dq;
}

bool SimdCapabilities::has_avx512bw() noexcept {
    detect_features();
    return cpu_info_.avx512bw;
}

bool SimdCapabilities::has_fma() noexcept {
    detect_features();
    return cpu_info_.fma;
}

const char* SimdCapabilities::get_capabilities_string() noexcept {
    detect_features();
    static char buffer[256];
    
    int offset = 0;
    offset += snprintf(buffer + offset, sizeof(buffer) - offset, "CPU Features:");
    
    if (cpu_info_.sse2) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " SSE2");
    if (cpu_info_.sse3) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " SSE3");
    if (cpu_info_.ssse3) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " SSSE3");
    if (cpu_info_.sse41) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " SSE4.1");
    if (cpu_info_.sse42) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " SSE4.2");
    if (cpu_info_.avx) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " AVX");
    if (cpu_info_.avx2) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " AVX2");
    if (cpu_info_.avx512f) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " AVX512F");
    if (cpu_info_.fma) offset += snprintf(buffer + offset, sizeof(buffer) - offset, " FMA");
    
    return buffer;
}

SimdLevel get_simd_level() noexcept {
    if (SimdCapabilities::has_avx512f()) return SimdLevel::AVX512;
    if (SimdCapabilities::has_avx2()) return SimdLevel::AVX2;
    if (SimdCapabilities::has_avx()) return SimdLevel::AVX;
    if (SimdCapabilities::has_sse42()) return SimdLevel::SSE42;
    if (SimdCapabilities::has_sse2()) return SimdLevel::SSE2;
    return SimdLevel::SCALAR;
}

void* simd_aligned_alloc(std::size_t size, std::size_t alignment) noexcept {
#ifdef _MSC_VER
    return _aligned_malloc(size, alignment);
#else
    void* ptr = nullptr;
    if (posix_memalign(&ptr, alignment, size) != 0) {
        return nullptr;
    }
    return ptr;
#endif
}

void simd_aligned_free(void* ptr) noexcept {
#ifdef _MSC_VER
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}

} // namespace claude_draw