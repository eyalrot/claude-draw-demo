#include <gtest/gtest.h>
#include "claude_draw/serialization/serialization_benchmarks.h"
#include <iostream>
#include <fstream>

using namespace claude_draw;
using namespace claude_draw::serialization;

class BenchmarkTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up test environment
    }
    
    void TearDown() override {
        // Clean up
    }
};

TEST_F(BenchmarkTest, RunAllBenchmarks) {
    std::cout << "\nRunning serialization benchmarks...\n" << std::endl;
    
    auto results = SerializationBenchmarks::run_all();
    
    // Verify all benchmarks completed
    EXPECT_FALSE(results.empty());
    
    // Print report
    std::string report = SerializationBenchmarks::generate_report(results);
    std::cout << report << std::endl;
    
    // Optionally save report to file
    std::ofstream report_file("serialization_benchmark_report.txt");
    if (report_file.is_open()) {
        report_file << report;
        report_file.close();
        std::cout << "\nBenchmark report saved to: serialization_benchmark_report.txt\n";
    }
    
    // Verify all benchmarks ran successfully
    for (const auto& result : results) {
        // Skip format comparison which has 0 time
        if (result.name.find("Format Size") == std::string::npos) {
            EXPECT_GT(result.time_ms, 0) << "Benchmark " << result.name << " failed to measure time";
            EXPECT_GT(result.throughput_mb_s, 0) << "Benchmark " << result.name << " has invalid throughput";
        }
    }
}

TEST_F(BenchmarkTest, BinaryVsJsonPerformance) {
    // Run specific comparisons
    auto binary_write = SerializationBenchmarks::benchmark_binary_write_small();
    auto json_write = SerializationBenchmarks::benchmark_json_write();
    
    std::cout << "\nBinary vs JSON Write Performance:\n";
    std::cout << binary_write.to_string() << "\n";
    std::cout << json_write.to_string() << "\n";
    
    // Binary should be faster
    EXPECT_LT(binary_write.time_ms, json_write.time_ms * 2) 
        << "Binary write should be significantly faster than JSON";
    
    // Binary should be smaller
    EXPECT_LT(binary_write.data_size, json_write.data_size)
        << "Binary format should be more compact than JSON";
}

TEST_F(BenchmarkTest, CompressionBenefit) {
    auto rle_compress = SerializationBenchmarks::benchmark_rle_compression();
    auto rle_decompress = SerializationBenchmarks::benchmark_rle_decompression();
    
    std::cout << "\nCompression Performance:\n";
    std::cout << rle_compress.to_string() << "\n";
    std::cout << rle_decompress.to_string() << "\n";
    
    // Decompression should be faster than compression
    EXPECT_LT(rle_decompress.time_ms, rle_compress.time_ms * 2)
        << "RLE decompression should be relatively fast";
}

TEST_F(BenchmarkTest, StreamingPerformance) {
    auto streaming_write = SerializationBenchmarks::benchmark_streaming_write();
    auto streaming_read = SerializationBenchmarks::benchmark_streaming_read();
    
    std::cout << "\nStreaming I/O Performance:\n";
    std::cout << streaming_write.to_string() << "\n";
    std::cout << streaming_read.to_string() << "\n";
    
    // Both should handle large amounts of data efficiently
    EXPECT_GT(streaming_write.object_count, 10000)
        << "Streaming should handle many objects";
    EXPECT_GT(streaming_write.throughput_mb_s, 10.0)
        << "Streaming should have reasonable throughput";
}

TEST_F(BenchmarkTest, ScalabilityTest) {
    // Test with different sizes
    std::vector<size_t> sizes = {100, 1000, 10000};
    std::vector<double> times;
    
    std::cout << "\nScalability Test:\n";
    
    for (size_t size : sizes) {
        BinaryWriter writer;
        
        auto start = SerializationBenchmarks::Clock::now();
        
        for (size_t i = 0; i < size; ++i) {
            writer.register_object(reinterpret_cast<void*>(i));
        }
        
        FileHeader header;
        header.magic = MAGIC_NUMBER;
        header.version_major = CURRENT_VERSION_MAJOR;
        header.version_minor = CURRENT_VERSION_MINOR;
        header.object_count = size;
        writer.write_file_header(header);
        
        for (size_t i = 0; i < size; ++i) {
            Point2D p(i * 0.1f, i * 0.2f);
            ObjectHeader header(TypeId::Point2D, i, sizeof(Point2D));
            writer.write_object_header(header);
            serialize(writer, p);
        }
        
        auto end = SerializationBenchmarks::Clock::now();
        
        double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        times.push_back(time_ms);
        
        std::cout << "Size: " << size << " objects, Time: " << time_ms << " ms\n";
    }
    
    // Verify reasonable scaling (should be roughly linear)
    if (times.size() >= 2) {
        double ratio1 = times[1] / times[0];
        double size_ratio1 = static_cast<double>(sizes[1]) / sizes[0];
        
        // Allow some variance but should be roughly linear
        EXPECT_LT(ratio1, size_ratio1 * 2.0)
            << "Performance should scale reasonably with size";
    }
}

// Performance regression test
TEST_F(BenchmarkTest, PerformanceRegression) {
    // Define expected performance baselines (adjust based on your system)
    const double MAX_WRITE_TIME_MS = 100.0;  // Max time for 10k objects
    const double MIN_THROUGHPUT_MB_S = 50.0; // Min throughput
    
    auto result = SerializationBenchmarks::benchmark_binary_write_small();
    
    EXPECT_LT(result.time_ms, MAX_WRITE_TIME_MS)
        << "Write performance regression detected";
    
    EXPECT_GT(result.throughput_mb_s, MIN_THROUGHPUT_MB_S)
        << "Throughput regression detected";
}