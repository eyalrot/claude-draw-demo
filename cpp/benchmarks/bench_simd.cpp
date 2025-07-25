#include <benchmark/benchmark.h>
#include "claude_draw/core/simd.h"
#include <vector>
#include <random>
#include <cstring>

using namespace claude_draw;

// Benchmark aligned memory allocation
static void BM_AlignedAlloc(benchmark::State& state) {
    const size_t size = state.range(0);
    const size_t alignment = 32;
    
    for (auto _ : state) {
        void* ptr = simd_aligned_alloc(size, alignment);
        benchmark::DoNotOptimize(ptr);
        simd_aligned_free(ptr);
    }
    
    state.SetBytesProcessed(state.iterations() * size);
}
BENCHMARK(BM_AlignedAlloc)->Range(64, 1024*1024);

// Benchmark aligned vs unaligned memory copy
static void BM_MemcpyAligned(benchmark::State& state) {
    const size_t size = state.range(0);
    
    void* src = simd_aligned_alloc(size, 32);
    void* dst = simd_aligned_alloc(size, 32);
    
    // Fill source with data
    std::memset(src, 0xAB, size);
    
    for (auto _ : state) {
        std::memcpy(dst, src, size);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    simd_aligned_free(src);
    simd_aligned_free(dst);
}
BENCHMARK(BM_MemcpyAligned)->Range(64, 1024*1024);

static void BM_MemcpyUnaligned(benchmark::State& state) {
    const size_t size = state.range(0);
    
    // Allocate with 1-byte offset to ensure misalignment
    char* src_buffer = new char[size + 32];
    char* dst_buffer = new char[size + 32];
    
    void* src = src_buffer + 1;  // Misaligned
    void* dst = dst_buffer + 1;  // Misaligned
    
    // Fill source with data
    std::memset(src, 0xAB, size);
    
    for (auto _ : state) {
        std::memcpy(dst, src, size);
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * size);
    
    delete[] src_buffer;
    delete[] dst_buffer;
}
BENCHMARK(BM_MemcpyUnaligned)->Range(64, 1024*1024);

// Benchmark SIMD detection overhead
static void BM_SimdDetection(benchmark::State& state) {
    for (auto _ : state) {
        bool has_avx2 = SimdCapabilities::has_avx2();
        benchmark::DoNotOptimize(has_avx2);
    }
}
BENCHMARK(BM_SimdDetection);

// Benchmark getting SIMD level
static void BM_GetSimdLevel(benchmark::State& state) {
    for (auto _ : state) {
        SimdLevel level = get_simd_level();
        benchmark::DoNotOptimize(level);
    }
}
BENCHMARK(BM_GetSimdLevel);

// Benchmark float array operations (baseline for future SIMD comparisons)
static void BM_FloatArrayAdd_Scalar(benchmark::State& state) {
    const size_t count = state.range(0);
    
    std::vector<float> a(count);
    std::vector<float> b(count);
    std::vector<float> c(count);
    
    // Initialize with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-100.0f, 100.0f);
    
    for (size_t i = 0; i < count; ++i) {
        a[i] = dist(gen);
        b[i] = dist(gen);
    }
    
    for (auto _ : state) {
        for (size_t i = 0; i < count; ++i) {
            c[i] = a[i] + b[i];
        }
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * count * sizeof(float) * 3);
}
BENCHMARK(BM_FloatArrayAdd_Scalar)->Range(128, 65536);

// Benchmark float array multiplication (baseline for future SIMD comparisons)
static void BM_FloatArrayMul_Scalar(benchmark::State& state) {
    const size_t count = state.range(0);
    
    std::vector<float> a(count);
    std::vector<float> b(count);
    std::vector<float> c(count);
    
    // Initialize with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-10.0f, 10.0f);
    
    for (size_t i = 0; i < count; ++i) {
        a[i] = dist(gen);
        b[i] = dist(gen);
    }
    
    for (auto _ : state) {
        for (size_t i = 0; i < count; ++i) {
            c[i] = a[i] * b[i];
        }
        benchmark::ClobberMemory();
    }
    
    state.SetBytesProcessed(state.iterations() * count * sizeof(float) * 3);
}
BENCHMARK(BM_FloatArrayMul_Scalar)->Range(128, 65536);

// Benchmark dot product (baseline for future SIMD comparisons)
static void BM_DotProduct_Scalar(benchmark::State& state) {
    const size_t count = state.range(0);
    
    std::vector<float> a(count);
    std::vector<float> b(count);
    
    // Initialize with random data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    for (size_t i = 0; i < count; ++i) {
        a[i] = dist(gen);
        b[i] = dist(gen);
    }
    
    for (auto _ : state) {
        float sum = 0.0f;
        for (size_t i = 0; i < count; ++i) {
            sum += a[i] * b[i];
        }
        benchmark::DoNotOptimize(sum);
    }
    
    state.SetBytesProcessed(state.iterations() * count * sizeof(float) * 2);
}
BENCHMARK(BM_DotProduct_Scalar)->Range(128, 65536);