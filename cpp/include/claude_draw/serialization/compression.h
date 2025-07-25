#pragma once

#include <vector>
#include <memory>
#include <string>
#include <stdexcept>
#include <cstring>
#include <chrono>
#include "claude_draw/serialization/binary_format.h"

namespace claude_draw {
namespace serialization {

/**
 * @brief Compression level enumeration
 */
enum class CompressionLevel : int {
    None = 0,
    Fastest = 1,
    Fast = 3,
    Default = 6,
    Better = 9,
    Best = 12
};

/**
 * @brief Compression statistics
 */
struct CompressionStats {
    size_t original_size;
    size_t compressed_size;
    double compression_ratio;
    double compression_speed_mbps;
    double decompression_speed_mbps;
    
    CompressionStats()
        : original_size(0)
        , compressed_size(0)
        , compression_ratio(0.0)
        , compression_speed_mbps(0.0)
        , decompression_speed_mbps(0.0) {}
};

/**
 * @brief Abstract base class for compression algorithms
 */
class Compressor {
public:
    virtual ~Compressor() = default;
    
    /**
     * @brief Compress data
     * @param input Input data buffer
     * @param input_size Size of input data
     * @param output Output buffer (will be resized)
     * @param level Compression level
     * @return Compression statistics
     */
    virtual CompressionStats compress(
        const void* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        CompressionLevel level = CompressionLevel::Default) = 0;
    
    /**
     * @brief Decompress data
     * @param input Compressed data buffer
     * @param input_size Size of compressed data
     * @param output Output buffer (will be resized)
     * @param expected_size Expected decompressed size (if known)
     * @return Decompressed data size
     */
    virtual size_t decompress(
        const void* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        size_t expected_size = 0) = 0;
    
    /**
     * @brief Get maximum compressed size for given input size
     */
    virtual size_t get_max_compressed_size(size_t input_size) const = 0;
    
    /**
     * @brief Get compressor name
     */
    virtual const char* name() const = 0;
    
    /**
     * @brief Check if compression is available
     */
    virtual bool is_available() const = 0;
};

/**
 * @brief No-op compressor (stores data uncompressed)
 */
class NoCompressor : public Compressor {
public:
    CompressionStats compress(
        const void* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        CompressionLevel level) override {
        
        (void)level;  // Unused
        
        output.resize(input_size);
        std::memcpy(output.data(), input, input_size);
        
        CompressionStats stats;
        stats.original_size = input_size;
        stats.compressed_size = input_size;
        stats.compression_ratio = 1.0;
        stats.compression_speed_mbps = 10000.0;  // Essentially memcpy speed
        stats.decompression_speed_mbps = 10000.0;
        
        return stats;
    }
    
    size_t decompress(
        const void* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        size_t expected_size) override {
        
        size_t output_size = expected_size > 0 ? expected_size : input_size;
        output.resize(output_size);
        std::memcpy(output.data(), input, input_size);
        return input_size;
    }
    
    size_t get_max_compressed_size(size_t input_size) const override {
        return input_size;
    }
    
    const char* name() const override { return "none"; }
    bool is_available() const override { return true; }
};

/**
 * @brief Simple RLE (Run-Length Encoding) compressor for demonstration
 * 
 * This is a simple implementation for testing. Real applications should use
 * LZ4 or Zstd for better compression ratios and performance.
 */
class RLECompressor : public Compressor {
public:
    CompressionStats compress(
        const void* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        CompressionLevel level) override {
        
        (void)level;  // Simple RLE doesn't have levels
        
        const uint8_t* src = static_cast<const uint8_t*>(input);
        output.clear();
        output.reserve(input_size);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t i = 0;
        while (i < input_size) {
            uint8_t value = src[i];
            size_t run_length = 1;
            
            // Count consecutive bytes (max 255)
            while (i + run_length < input_size && 
                   run_length < 255 && 
                   src[i + run_length] == value) {
                run_length++;
            }
            
            // Encode as: [count][value]
            output.push_back(static_cast<uint8_t>(run_length));
            output.push_back(value);
            
            i += run_length;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
        
        CompressionStats stats;
        stats.original_size = input_size;
        stats.compressed_size = output.size();
        stats.compression_ratio = static_cast<double>(input_size) / output.size();
        stats.compression_speed_mbps = (input_size / 1048576.0) / (duration / 1000000.0);
        stats.decompression_speed_mbps = stats.compression_speed_mbps * 2.0;  // RLE decode is faster
        
        return stats;
    }
    
    size_t decompress(
        const void* input,
        size_t input_size,
        std::vector<uint8_t>& output,
        size_t expected_size) override {
        
        const uint8_t* src = static_cast<const uint8_t*>(input);
        output.clear();
        
        if (expected_size > 0) {
            output.reserve(expected_size);
        }
        
        size_t i = 0;
        while (i + 1 < input_size) {
            uint8_t count = src[i];
            uint8_t value = src[i + 1];
            
            for (uint8_t j = 0; j < count; ++j) {
                output.push_back(value);
            }
            
            i += 2;
        }
        
        return output.size();
    }
    
    size_t get_max_compressed_size(size_t input_size) const override {
        // Worst case: no compression, plus overhead
        return input_size * 2;
    }
    
    const char* name() const override { return "rle"; }
    bool is_available() const override { return true; }
};

/**
 * @brief Streaming compression interface
 */
class StreamingCompressor {
public:
    virtual ~StreamingCompressor() = default;
    
    /**
     * @brief Initialize compression stream
     * @param level Compression level
     */
    virtual void begin_compression(CompressionLevel level = CompressionLevel::Default) = 0;
    
    /**
     * @brief Compress a chunk of data
     * @param input Input data
     * @param input_size Size of input
     * @param output Output buffer
     * @param output_capacity Output buffer capacity
     * @param flush Whether to flush the compressor
     * @return Number of bytes written to output
     */
    virtual size_t compress_chunk(
        const void* input,
        size_t input_size,
        void* output,
        size_t output_capacity,
        bool flush = false) = 0;
    
    /**
     * @brief Finish compression and get any remaining data
     * @param output Output buffer
     * @param output_capacity Output buffer capacity
     * @return Number of bytes written to output
     */
    virtual size_t finish_compression(void* output, size_t output_capacity) = 0;
    
    /**
     * @brief Initialize decompression stream
     */
    virtual void begin_decompression() = 0;
    
    /**
     * @brief Decompress a chunk of data
     * @param input Compressed input data
     * @param input_size Size of input
     * @param output Output buffer
     * @param output_capacity Output buffer capacity
     * @return Number of bytes written to output
     */
    virtual size_t decompress_chunk(
        const void* input,
        size_t input_size,
        void* output,
        size_t output_capacity) = 0;
    
    /**
     * @brief Reset the compressor for reuse
     */
    virtual void reset() = 0;
};

/**
 * @brief Factory for creating compressors
 */
class CompressionFactory {
public:
    /**
     * @brief Create a compressor by type
     * @param type Compression type from binary_format.h
     * @return Unique pointer to compressor
     */
    static std::unique_ptr<Compressor> create(CompressionType type) {
        switch (type) {
            case CompressionType::None:
                return std::make_unique<NoCompressor>();
            
            case CompressionType::LZ4:
                // In a real implementation, this would create LZ4Compressor
                // For now, fallback to RLE
                return std::make_unique<RLECompressor>();
            
            case CompressionType::Zstd:
                // In a real implementation, this would create ZstdCompressor
                // For now, fallback to RLE
                return std::make_unique<RLECompressor>();
            
            default:
                throw std::runtime_error("Unknown compression type");
        }
    }
    
    /**
     * @brief Check if a compression type is available
     */
    static bool is_available(CompressionType type) {
        auto compressor = create(type);
        return compressor->is_available();
    }
    
    /**
     * @brief Get recommended compression type
     * @param data_size Size of data to compress
     * @param speed_priority True for speed, false for compression ratio
     */
    static CompressionType get_recommended(size_t data_size, bool speed_priority) {
        // For small data, compression overhead might not be worth it
        if (data_size < 1024) {
            return CompressionType::None;
        }
        
        // LZ4 is faster, Zstd compresses better
        if (speed_priority) {
            return CompressionType::LZ4;
        } else {
            return CompressionType::Zstd;
        }
    }
};

/**
 * @brief Compressed data header
 */
struct CompressedDataHeader {
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    CompressionType compression_type;
    uint8_t reserved[3];
    
    CompressedDataHeader()
        : uncompressed_size(0)
        , compressed_size(0)
        , compression_type(CompressionType::None) {
        std::memset(reserved, 0, sizeof(reserved));
    }
};

static_assert(sizeof(CompressedDataHeader) == 12, "CompressedDataHeader must be 12 bytes");

} // namespace serialization
} // namespace claude_draw