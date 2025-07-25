#pragma once

#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/serialization/simple_serializers.h"
#include "claude_draw/serialization/compression.h"
#include "claude_draw/serialization/streaming.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>

// For JSON comparison
#include <iostream>

namespace claude_draw {
namespace serialization {

/**
 * @brief Benchmark result structure
 */
struct BenchmarkResult {
    std::string name;
    double time_ms;
    size_t data_size;
    size_t object_count;
    double throughput_mb_s;
    double objects_per_second;
    
    void calculate_metrics() {
        if (time_ms > 0) {
            throughput_mb_s = (data_size / (1024.0 * 1024.0)) / (time_ms / 1000.0);
            objects_per_second = (object_count * 1000.0) / time_ms;
        }
    }
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << name << ": "
            << time_ms << " ms, "
            << data_size << " bytes, "
            << throughput_mb_s << " MB/s, "
            << objects_per_second << " obj/s";
        return oss.str();
    }
};

/**
 * @brief Benchmark suite for serialization
 */
class SerializationBenchmarks {
public:
    using Clock = std::chrono::high_resolution_clock;
    using Duration = std::chrono::duration<double, std::milli>;
    
    /**
     * @brief Run all benchmarks
     */
    static std::vector<BenchmarkResult> run_all() {
        std::vector<BenchmarkResult> results;
        
        // Binary format benchmarks
        results.push_back(benchmark_binary_write_small());
        results.push_back(benchmark_binary_read_small());
        results.push_back(benchmark_binary_write_large());
        results.push_back(benchmark_binary_read_large());
        
        // Compression benchmarks
        results.push_back(benchmark_rle_compression());
        results.push_back(benchmark_rle_decompression());
        
        // Streaming benchmarks
        results.push_back(benchmark_streaming_write());
        results.push_back(benchmark_streaming_read());
        
        // JSON comparison
        results.push_back(benchmark_json_write());
        results.push_back(benchmark_json_read());
        
        // Format size comparison
        results.push_back(benchmark_format_size_comparison());
        
        return results;
    }
    
    /**
     * @brief Benchmark binary writing of small objects
     */
    static BenchmarkResult benchmark_binary_write_small() {
        BenchmarkResult result;
        result.name = "Binary Write (Small Objects)";
        
        const size_t count = 10000;
        std::vector<Point2D> points;
        std::vector<Color> colors;
        
        // Generate test data
        for (size_t i = 0; i < count; ++i) {
            points.emplace_back(i * 0.1f, i * 0.2f);
            colors.emplace_back(static_cast<uint8_t>(i % 256), 
                              static_cast<uint8_t>((i * 2) % 256), 
                              static_cast<uint8_t>((i * 3) % 256), 
                              static_cast<uint8_t>(255));
        }
        
        auto start = Clock::now();
        
        BinaryWriter writer;
        for (const auto& p : points) writer.register_object(&p);
        for (const auto& c : colors) writer.register_object(&c);
        
        writer.write_file_header(create_test_header(count * 2));
        
        for (size_t i = 0; i < count; ++i) {
            ObjectHeader p_header(TypeId::Point2D, i, sizeof(Point2D));
            writer.write_object_header(p_header);
            serialize(writer, points[i]);
            
            ObjectHeader c_header(TypeId::Color, count + i, sizeof(Color));
            writer.write_object_header(c_header);
            serialize(writer, colors[i]);
        }
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = writer.get_buffer().size();
        result.object_count = count * 2;
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Benchmark binary reading of small objects
     */
    static BenchmarkResult benchmark_binary_read_small() {
        BenchmarkResult result;
        result.name = "Binary Read (Small Objects)";
        
        // First create data to read
        const size_t count = 10000;
        BinaryWriter writer;
        
        for (size_t i = 0; i < count * 2; ++i) {
            writer.register_object(reinterpret_cast<void*>(i));
        }
        
        writer.write_file_header(create_test_header(count * 2));
        
        for (size_t i = 0; i < count; ++i) {
            Point2D p(i * 0.1f, i * 0.2f);
            Color c(static_cast<uint8_t>(i % 256), 
                   static_cast<uint8_t>((i * 2) % 256), 
                   static_cast<uint8_t>((i * 3) % 256), 
                   static_cast<uint8_t>(255));
            
            ObjectHeader p_header(TypeId::Point2D, i, sizeof(Point2D));
            writer.write_object_header(p_header);
            serialize(writer, p);
            
            ObjectHeader c_header(TypeId::Color, count + i, sizeof(Color));
            writer.write_object_header(c_header);
            serialize(writer, c);
        }
        
        auto buffer = writer.get_buffer();
        
        auto start = Clock::now();
        
        BinaryReader reader(buffer);
        reader.read_file_header();  // Header read but not used in benchmark
        
        std::vector<Point2D> points;
        std::vector<Color> colors;
        
        while (!reader.eof()) {
            auto obj_header = reader.read_object_header();
            
            if (obj_header.type == TypeId::Point2D) {
                Point2D p = deserialize_point(reader);
                points.push_back(p);
            } else if (obj_header.type == TypeId::Color) {
                Color c = deserialize_color(reader);
                colors.push_back(c);
            }
        }
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = buffer.size();
        result.object_count = points.size() + colors.size();
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Benchmark binary writing of large objects
     */
    static BenchmarkResult benchmark_binary_write_large() {
        BenchmarkResult result;
        result.name = "Binary Write (Large Objects)";
        
        const size_t count = 1000;
        std::vector<Transform2D> transforms;
        std::vector<BoundingBox> bboxes;
        
        // Generate test data
        for (size_t i = 0; i < count; ++i) {
            transforms.emplace_back();
            transforms.back().translate(i * 10.0f, i * 20.0f);
            transforms.back().rotate(i * 0.1f);
            
            bboxes.emplace_back(i * 1.0f, i * 2.0f, i * 3.0f + 10.0f, i * 4.0f + 20.0f);
        }
        
        auto start = Clock::now();
        
        BinaryWriter writer;
        for (const auto& t : transforms) writer.register_object(&t);
        for (const auto& b : bboxes) writer.register_object(&b);
        
        writer.write_file_header(create_test_header(count * 2));
        
        for (size_t i = 0; i < count; ++i) {
            ObjectHeader t_header(TypeId::Transform2D, i, sizeof(Transform2D));
            writer.write_object_header(t_header);
            serialize(writer, transforms[i]);
            
            ObjectHeader b_header(TypeId::BoundingBox, count + i, sizeof(BoundingBox));
            writer.write_object_header(b_header);
            serialize(writer, bboxes[i]);
        }
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = writer.get_buffer().size();
        result.object_count = count * 2;
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Benchmark binary reading of large objects
     */
    static BenchmarkResult benchmark_binary_read_large() {
        BenchmarkResult result;
        result.name = "Binary Read (Large Objects)";
        
        // First create data to read
        const size_t count = 1000;
        BinaryWriter writer;
        
        for (size_t i = 0; i < count * 2; ++i) {
            writer.register_object(reinterpret_cast<void*>(i));
        }
        
        writer.write_file_header(create_test_header(count * 2));
        
        for (size_t i = 0; i < count; ++i) {
            Transform2D t;
            t.translate(i * 10.0f, i * 20.0f);
            t.rotate(i * 0.1f);
            
            BoundingBox b(i * 1.0f, i * 2.0f, i * 3.0f + 10.0f, i * 4.0f + 20.0f);
            
            ObjectHeader t_header(TypeId::Transform2D, i, sizeof(Transform2D));
            writer.write_object_header(t_header);
            serialize(writer, t);
            
            ObjectHeader b_header(TypeId::BoundingBox, count + i, sizeof(BoundingBox));
            writer.write_object_header(b_header);
            serialize(writer, b);
        }
        
        auto buffer = writer.get_buffer();
        
        auto start = Clock::now();
        
        BinaryReader reader(buffer);
        reader.read_file_header();  // Header read but not used in benchmark
        
        std::vector<Transform2D> transforms;
        std::vector<BoundingBox> bboxes;
        
        while (!reader.eof()) {
            auto obj_header = reader.read_object_header();
            
            if (obj_header.type == TypeId::Transform2D) {
                Transform2D t = deserialize_transform(reader);
                transforms.push_back(t);
            } else if (obj_header.type == TypeId::BoundingBox) {
                BoundingBox b = deserialize_bbox(reader);
                bboxes.push_back(b);
            }
        }
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = buffer.size();
        result.object_count = transforms.size() + bboxes.size();
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Benchmark RLE compression
     */
    static BenchmarkResult benchmark_rle_compression() {
        BenchmarkResult result;
        result.name = "RLE Compression";
        
        // Create data with runs
        std::vector<uint8_t> data;
        for (int i = 0; i < 1000; ++i) {
            // Add runs of same value
            uint8_t value = i % 10;
            size_t run_length = 10 + (i % 20);
            for (size_t j = 0; j < run_length; ++j) {
                data.push_back(value);
            }
        }
        
        auto start = Clock::now();
        
        RLECompressor compressor;
        std::vector<uint8_t> compressed;
        auto stats = compressor.compress(data.data(), data.size(), compressed, CompressionLevel::Default);
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = data.size();
        result.object_count = 1;
        result.calculate_metrics();
        
        // Add compression ratio info
        double ratio = stats.compression_ratio;
        result.name += " (ratio: " + std::to_string(ratio).substr(0, 4) + ")";
        
        return result;
    }
    
    /**
     * @brief Benchmark RLE decompression
     */
    static BenchmarkResult benchmark_rle_decompression() {
        BenchmarkResult result;
        result.name = "RLE Decompression";
        
        // Create and compress data
        std::vector<uint8_t> data;
        for (int i = 0; i < 1000; ++i) {
            uint8_t value = i % 10;
            size_t run_length = 10 + (i % 20);
            for (size_t j = 0; j < run_length; ++j) {
                data.push_back(value);
            }
        }
        
        RLECompressor compressor;
        std::vector<uint8_t> compressed;
        auto compress_stats = compressor.compress(data.data(), data.size(), compressed, CompressionLevel::Default);
        
        auto start = Clock::now();
        
        std::vector<uint8_t> decompressed;
        auto decompress_size = compressor.decompress(compressed.data(), compressed.size(), decompressed, data.size());
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = compressed.size();
        result.object_count = 1;
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Benchmark streaming write
     */
    static BenchmarkResult benchmark_streaming_write() {
        BenchmarkResult result;
        result.name = "Streaming Write (1MB chunks)";
        
        const size_t total_objects = 50000;
        std::string filename = "benchmark_streaming.bin";
        
        auto start = Clock::now();
        
        StreamingConfig config;
        config.chunk_size = 1024 * 1024; // 1MB chunks
        StreamingWriter writer(filename, config);
        
        // StreamingWriter handles header internally
        
        for (size_t i = 0; i < total_objects; ++i) {
            Point2D p(i * 0.1f, i * 0.2f);
            writer.write_object(TypeId::Point2D, i, p);
        }
        
        writer.finalize();
        
        auto end = Clock::now();
        
        // Get file size
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        size_t file_size = file.tellg();
        file.close();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = file_size;
        result.object_count = total_objects;
        result.calculate_metrics();
        
        // Clean up
        std::remove(filename.c_str());
        
        return result;
    }
    
    /**
     * @brief Benchmark streaming read
     */
    static BenchmarkResult benchmark_streaming_read() {
        BenchmarkResult result;
        result.name = "Streaming Read (1MB chunks)";
        
        const size_t total_objects = 50000;
        std::string filename = "benchmark_streaming.bin";
        
        // First create the file
        {
            StreamingConfig config;
            config.chunk_size = 1024 * 1024;
            StreamingWriter writer(filename, config);
            
            for (size_t i = 0; i < total_objects; ++i) {
                Point2D p(i * 0.1f, i * 0.2f);
                writer.write_object(TypeId::Point2D, i, p);
            }
            
            writer.finalize();
        }
        
        auto start = Clock::now();
        
        StreamingConfig config;
        config.chunk_size = 1024 * 1024;
        StreamingReader reader(filename, config);
        
        // Process file with streaming reader
        size_t count = 0;
        reader.set_object_callback([&count](TypeId type, uint32_t, const void*, size_t) {
            if (type == TypeId::Point2D) {
                count++;
            }
        });
        
        // Read through the file
        while (reader.read_next_object()) {
            // Object is processed by callback
        }
        
        auto end = Clock::now();
        
        // Get file size
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        size_t file_size = file.tellg();
        file.close();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = file_size;
        result.object_count = count;
        result.calculate_metrics();
        
        // Clean up
        std::remove(filename.c_str());
        
        return result;
    }
    
    /**
     * @brief Benchmark JSON writing for comparison
     */
    static BenchmarkResult benchmark_json_write() {
        BenchmarkResult result;
        result.name = "JSON Write (Simple Format)";
        
        const size_t count = 10000;
        
        auto start = Clock::now();
        
        std::ostringstream oss;
        oss << "{\n  \"objects\": [\n";
        
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) oss << ",\n";
            
            // Write Point2D
            oss << "    {\"type\": \"Point2D\", \"x\": " << (i * 0.1f) 
                << ", \"y\": " << (i * 0.2f) << "},\n";
            
            // Write Color
            oss << "    {\"type\": \"Color\", \"r\": " << (i % 256)
                << ", \"g\": " << ((i * 2) % 256)
                << ", \"b\": " << ((i * 3) % 256)
                << ", \"a\": 255}";
        }
        
        oss << "\n  ]\n}";
        
        std::string json_data = oss.str();
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = json_data.size();
        result.object_count = count * 2;
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Benchmark JSON reading for comparison
     */
    static BenchmarkResult benchmark_json_read() {
        BenchmarkResult result;
        result.name = "JSON Read (Simple Parser)";
        
        // Create JSON data
        const size_t count = 10000;
        std::ostringstream oss;
        oss << "{\n  \"objects\": [\n";
        
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) oss << ",\n";
            oss << "    {\"type\": \"Point2D\", \"x\": " << (i * 0.1f) 
                << ", \"y\": " << (i * 0.2f) << "},\n";
            oss << "    {\"type\": \"Color\", \"r\": " << (i % 256)
                << ", \"g\": " << ((i * 2) % 256)
                << ", \"b\": " << ((i * 3) % 256)
                << ", \"a\": 255}";
        }
        
        oss << "\n  ]\n}";
        std::string json_data = oss.str();
        
        auto start = Clock::now();
        
        // Simple parsing simulation (not a real JSON parser)
        size_t object_count = 0;
        size_t pos = 0;
        while ((pos = json_data.find("\"type\":", pos)) != std::string::npos) {
            object_count++;
            pos += 7;
        }
        
        auto end = Clock::now();
        
        result.time_ms = Duration(end - start).count();
        result.data_size = json_data.size();
        result.object_count = object_count;
        result.calculate_metrics();
        
        return result;
    }
    
    /**
     * @brief Compare format sizes
     */
    static BenchmarkResult benchmark_format_size_comparison() {
        BenchmarkResult result;
        result.name = "Format Size Comparison";
        
        const size_t count = 1000;
        
        // Binary format size
        BinaryWriter binary_writer;
        for (size_t i = 0; i < count; ++i) {
            binary_writer.register_object(reinterpret_cast<void*>(i));
        }
        
        binary_writer.write_file_header(create_test_header(count));
        
        for (size_t i = 0; i < count; ++i) {
            Point2D p(i * 0.1f, i * 0.2f);
            ObjectHeader header(TypeId::Point2D, i, sizeof(Point2D));
            binary_writer.write_object_header(header);
            serialize(binary_writer, p);
        }
        
        size_t binary_size = binary_writer.get_buffer().size();
        
        // JSON format size
        std::ostringstream json_oss;
        json_oss << "{\n  \"objects\": [\n";
        
        for (size_t i = 0; i < count; ++i) {
            if (i > 0) json_oss << ",\n";
            json_oss << "    {\"type\": \"Point2D\", \"x\": " << (i * 0.1f) 
                     << ", \"y\": " << (i * 0.2f) << "}";
        }
        
        json_oss << "\n  ]\n}";
        size_t json_size = json_oss.str().size();
        
        // Compressed binary size
        RLECompressor compressor;
        std::vector<uint8_t> compressed;
        auto stats = compressor.compress(binary_writer.get_buffer().data(), 
                                       binary_writer.get_buffer().size(), 
                                       compressed, CompressionLevel::Default);
        size_t compressed_size = compressed.size();
        
        // Report as a special result
        result.name = "Format Sizes - Binary: " + std::to_string(binary_size) +
                      " bytes, JSON: " + std::to_string(json_size) +
                      " bytes, Compressed: " + std::to_string(compressed_size) + " bytes";
        result.time_ms = 0;
        result.data_size = binary_size;
        result.object_count = count;
        
        // Calculate size ratios
        double json_ratio = static_cast<double>(json_size) / binary_size;
        double compression_ratio = stats.compression_ratio;
        
        result.name += " (JSON/Binary: " + std::to_string(json_ratio).substr(0, 4) +
                       ", Compression: " + std::to_string(compression_ratio).substr(0, 4) + ")";
        
        return result;
    }
    
    /**
     * @brief Generate benchmark report
     */
    static std::string generate_report(const std::vector<BenchmarkResult>& results) {
        std::ostringstream oss;
        oss << "\nSerialization Benchmark Results\n";
        oss << "==============================\n\n";
        
        for (const auto& result : results) {
            oss << result.to_string() << "\n";
        }
        
        // Summary statistics
        oss << "\nSummary:\n";
        oss << "--------\n";
        
        // Find fastest/slowest
        if (!results.empty()) {
            auto fastest = std::min_element(results.begin(), results.end(),
                [](const BenchmarkResult& a, const BenchmarkResult& b) {
                    return a.time_ms < b.time_ms && a.time_ms > 0;
                });
            
            auto highest_throughput = std::max_element(results.begin(), results.end(),
                [](const BenchmarkResult& a, const BenchmarkResult& b) {
                    return a.throughput_mb_s < b.throughput_mb_s;
                });
            
            if (fastest != results.end()) {
                oss << "Fastest: " << fastest->name << " (" << fastest->time_ms << " ms)\n";
            }
            
            if (highest_throughput != results.end()) {
                oss << "Highest throughput: " << highest_throughput->name 
                    << " (" << highest_throughput->throughput_mb_s << " MB/s)\n";
            }
        }
        
        return oss.str();
    }
    
private:
    static FileHeader create_test_header(size_t object_count) {
        FileHeader header;
        header.magic = MAGIC_NUMBER;
        header.version_major = CURRENT_VERSION_MAJOR;
        header.version_minor = CURRENT_VERSION_MINOR;
        header.flags = 0;
        header.compression = CompressionType::None;
        header.data_offset = sizeof(FileHeader);
        header.total_size = 0; // Will be updated
        header.object_count = object_count;
        header.checksum = 0;
        return header;
    }
};

} // namespace serialization
} // namespace claude_draw