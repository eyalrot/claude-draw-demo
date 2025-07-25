#include <benchmark/benchmark.h>
#include "claude_draw/core/simd.h"
#include <iostream>

// Initialize benchmark environment
int main(int argc, char** argv) {
    // Initialize SIMD capabilities
    claude_draw::SimdCapabilities::initialize();
    
    // Print system information
    std::cout << "Claude Draw C++ Benchmarks\n";
    std::cout << "==========================\n";
    std::cout << claude_draw::SimdCapabilities::get_capabilities_string() << "\n";
    std::cout << "SIMD Level: ";
    
    switch (claude_draw::get_simd_level()) {
        case claude_draw::SimdLevel::AVX512:
            std::cout << "AVX512\n";
            break;
        case claude_draw::SimdLevel::AVX2:
            std::cout << "AVX2\n";
            break;
        case claude_draw::SimdLevel::AVX:
            std::cout << "AVX\n";
            break;
        case claude_draw::SimdLevel::SSE42:
            std::cout << "SSE4.2\n";
            break;
        case claude_draw::SimdLevel::SSE2:
            std::cout << "SSE2\n";
            break;
        case claude_draw::SimdLevel::SCALAR:
            std::cout << "SCALAR\n";
            break;
    }
    
    std::cout << "==========================\n\n";
    
    // Initialize Google Benchmark
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) {
        return 1;
    }
    
    // Run benchmarks
    ::benchmark::RunSpecifiedBenchmarks();
    
    // Shutdown
    ::benchmark::Shutdown();
    
    return 0;
}