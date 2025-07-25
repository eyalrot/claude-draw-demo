#include "claude_draw/serialization/serialization_benchmarks.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>

using namespace claude_draw;
using namespace claude_draw::serialization;

/**
 * @brief Standalone benchmark executable
 * 
 * Usage: ./serialization_benchmark [options]
 * Options:
 *   --full        Run full benchmark suite
 *   --quick       Run quick benchmarks only
 *   --json        Output results in JSON format
 *   --csv         Output results in CSV format
 *   --output FILE Save results to file
 */
int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool full_suite = false;
    bool json_output = false;
    bool csv_output = false;
    std::string output_file;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--full") {
            full_suite = true;
        } else if (arg == "--quick") {
            full_suite = false;
        } else if (arg == "--json") {
            json_output = true;
        } else if (arg == "--csv") {
            csv_output = true;
        } else if (arg == "--output" && i + 1 < argc) {
            output_file = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --full        Run full benchmark suite\n";
            std::cout << "  --quick       Run quick benchmarks only (default)\n";
            std::cout << "  --json        Output results in JSON format\n";
            std::cout << "  --csv         Output results in CSV format\n";
            std::cout << "  --output FILE Save results to file\n";
            std::cout << "  --help        Show this help message\n";
            return 0;
        }
    }
    
    std::cout << "Claude Draw Serialization Benchmark\n";
    std::cout << "===================================\n\n";
    
    // Run benchmarks
    std::vector<BenchmarkResult> results;
    
    if (full_suite) {
        std::cout << "Running full benchmark suite...\n\n";
        results = SerializationBenchmarks::run_all();
    } else {
        std::cout << "Running quick benchmarks...\n\n";
        // Run subset of benchmarks
        results.push_back(SerializationBenchmarks::benchmark_binary_write_small());
        results.push_back(SerializationBenchmarks::benchmark_binary_read_small());
        results.push_back(SerializationBenchmarks::benchmark_json_write());
        results.push_back(SerializationBenchmarks::benchmark_format_size_comparison());
    }
    
    // Prepare output
    std::ostringstream output;
    
    if (json_output) {
        // JSON format
        output << "{\n";
        output << "  \"benchmarks\": [\n";
        
        for (size_t i = 0; i < results.size(); ++i) {
            if (i > 0) output << ",\n";
            const auto& r = results[i];
            output << "    {\n";
            output << "      \"name\": \"" << r.name << "\",\n";
            output << "      \"time_ms\": " << r.time_ms << ",\n";
            output << "      \"data_size\": " << r.data_size << ",\n";
            output << "      \"object_count\": " << r.object_count << ",\n";
            output << "      \"throughput_mb_s\": " << r.throughput_mb_s << ",\n";
            output << "      \"objects_per_second\": " << r.objects_per_second << "\n";
            output << "    }";
        }
        
        output << "\n  ]\n";
        output << "}\n";
    } else if (csv_output) {
        // CSV format
        output << "Name,Time (ms),Data Size (bytes),Object Count,Throughput (MB/s),Objects/s\n";
        
        for (const auto& r : results) {
            output << "\"" << r.name << "\","
                   << r.time_ms << ","
                   << r.data_size << ","
                   << r.object_count << ","
                   << r.throughput_mb_s << ","
                   << r.objects_per_second << "\n";
        }
    } else {
        // Human-readable format
        output << SerializationBenchmarks::generate_report(results);
        
        // Add performance analysis
        output << "\nPerformance Analysis:\n";
        output << "--------------------\n";
        
        // Find binary vs JSON comparison
        auto binary_it = std::find_if(results.begin(), results.end(),
            [](const BenchmarkResult& r) { return r.name.find("Binary Write") != std::string::npos; });
        auto json_it = std::find_if(results.begin(), results.end(),
            [](const BenchmarkResult& r) { return r.name.find("JSON Write") != std::string::npos; });
        
        if (binary_it != results.end() && json_it != results.end()) {
            double speedup = json_it->time_ms / binary_it->time_ms;
            double size_reduction = 1.0 - (static_cast<double>(binary_it->data_size) / json_it->data_size);
            
            output << std::fixed << std::setprecision(2);
            output << "Binary vs JSON:\n";
            output << "  - Binary is " << speedup << "x faster\n";
            output << "  - Binary uses " << (size_reduction * 100) << "% less space\n\n";
        }
        
        // System info
        output << "System Information:\n";
        output << "  - Platform: " << 
            #ifdef _WIN32
                "Windows"
            #elif __APPLE__
                "macOS"
            #else
                "Linux"
            #endif
            << "\n";
        output << "  - Compiler: " <<
            #ifdef __clang__
                "Clang " __clang_version__
            #elif __GNUC__
                "GCC " __VERSION__
            #else
                "Unknown"
            #endif
            << "\n";
        output << "  - Build Type: " <<
            #ifdef NDEBUG
                "Release"
            #else
                "Debug"
            #endif
            << "\n";
    }
    
    // Output results
    std::string result_str = output.str();
    
    if (!output_file.empty()) {
        std::ofstream file(output_file);
        if (file.is_open()) {
            file << result_str;
            file.close();
            std::cout << "Results saved to: " << output_file << "\n";
        } else {
            std::cerr << "Error: Could not open output file: " << output_file << "\n";
            return 1;
        }
    } else {
        std::cout << result_str;
    }
    
    return 0;
}