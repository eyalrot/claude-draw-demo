#pragma once

#include <benchmark/benchmark.h>
#include <random>
#include <vector>
#include <memory>

namespace claude_draw::bench {

// Random data generator for benchmarks
class BenchmarkDataGenerator {
public:
    BenchmarkDataGenerator(unsigned seed = std::random_device{}()) 
        : rng_(seed) {}
    
    std::vector<float> generate_floats(size_t count, float min = -100.0f, float max = 100.0f) {
        std::uniform_real_distribution<float> dist(min, max);
        std::vector<float> data(count);
        for (auto& val : data) {
            val = dist(rng_);
        }
        return data;
    }
    
    std::vector<double> generate_doubles(size_t count, double min = -100.0, double max = 100.0) {
        std::uniform_real_distribution<double> dist(min, max);
        std::vector<double> data(count);
        for (auto& val : data) {
            val = dist(rng_);
        }
        return data;
    }
    
    std::vector<int> generate_ints(size_t count, int min = -1000, int max = 1000) {
        std::uniform_int_distribution<int> dist(min, max);
        std::vector<int> data(count);
        for (auto& val : data) {
            val = dist(rng_);
        }
        return data;
    }
    
private:
    std::mt19937 rng_;
};

// Custom benchmark counters
inline void SetCommonCounters(benchmark::State& state, size_t bytes_processed) {
    state.SetBytesProcessed(bytes_processed);
    state.SetItemsProcessed(state.iterations());
    
    // Add custom counters
    state.counters["ops_per_sec"] = benchmark::Counter(
        state.iterations(), benchmark::Counter::kIsRate);
    state.counters["bytes_per_sec"] = benchmark::Counter(
        bytes_processed, benchmark::Counter::kIsRate);
}

// Memory bandwidth counter
inline void SetMemoryBandwidth(benchmark::State& state, size_t bytes_per_iteration) {
    size_t total_bytes = state.iterations() * bytes_per_iteration;
    state.SetBytesProcessed(total_bytes);
    
    // Calculate bandwidth in GB/s
    double seconds = state.seconds_elapsed();
    double gb_per_sec = (total_bytes / 1e9) / seconds;
    state.counters["bandwidth_GB/s"] = gb_per_sec;
}

// SIMD efficiency counter (operations per cycle estimate)
inline void SetSimdEfficiency(benchmark::State& state, size_t operations_per_iteration, 
                              size_t vector_width = 8) {  // 8 floats for AVX2
    size_t total_ops = state.iterations() * operations_per_iteration;
    double ops_per_sec = total_ops / state.seconds_elapsed();
    
    // Assume 3GHz CPU for rough estimate
    const double assumed_ghz = 3.0;
    double ops_per_cycle = ops_per_sec / (assumed_ghz * 1e9);
    
    state.counters["ops_per_cycle"] = ops_per_cycle;
    state.counters["simd_efficiency_%"] = (ops_per_cycle / vector_width) * 100.0;
}

// Benchmark fixture with data pre-generation
template<typename T>
class BenchmarkFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        size_ = state.range(0);
        generator_ = std::make_unique<BenchmarkDataGenerator>();
    }
    
    void TearDown(const ::benchmark::State& state) override {
        // Cleanup if needed
    }
    
protected:
    size_t size_;
    std::unique_ptr<BenchmarkDataGenerator> generator_;
};

// Common benchmark sizes
constexpr int64_t TINY_SIZE = 16;
constexpr int64_t SMALL_SIZE = 128;
constexpr int64_t MEDIUM_SIZE = 1024;
constexpr int64_t LARGE_SIZE = 16384;
constexpr int64_t HUGE_SIZE = 1048576;

// Standard benchmark ranges
inline void StandardSizeRange(benchmark::internal::Benchmark* b) {
    b->Range(TINY_SIZE, HUGE_SIZE)->RangeMultiplier(8);
}

inline void MemorySizeRange(benchmark::internal::Benchmark* b) {
    // Powers of 2 from 1KB to 16MB
    for (int64_t size = 1024; size <= 16 * 1024 * 1024; size *= 2) {
        b->Arg(size);
    }
}

} // namespace claude_draw::bench