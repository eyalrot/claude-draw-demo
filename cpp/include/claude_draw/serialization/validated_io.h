#pragma once

#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/serialization/validation.h"
#include "claude_draw/serialization/streaming.h"
#include <string>
#include <memory>

namespace claude_draw {
namespace serialization {

/**
 * @brief Validated binary writer that ensures data integrity
 */
class ValidatedBinaryWriter {
public:
    explicit ValidatedBinaryWriter(size_t reserve_size = 1024 * 1024)
        : writer_(reserve_size)
        , enable_validation_(true) {}
    
    /**
     * @brief Write to file with validation
     */
    void write_to_file(const std::string& filename) {
        writer_.write_to_file_with_header(filename, true);
        
        if (enable_validation_) {
            // Validate the written file
            auto result = FormatValidator::validate_file_checksum(filename);
            if (!result.valid) {
                throw ValidationException(result);
            }
        }
    }
    
    /**
     * @brief Get underlying writer
     */
    BinaryWriter& writer() { return writer_; }
    const BinaryWriter& writer() const { return writer_; }
    
    /**
     * @brief Enable/disable validation
     */
    void set_validation_enabled(bool enabled) {
        enable_validation_ = enabled;
    }
    
private:
    BinaryWriter writer_;
    bool enable_validation_;
};

/**
 * @brief Validated binary reader that ensures data integrity
 */
class ValidatedBinaryReader {
public:
    /**
     * @brief Read file with validation
     */
    static std::vector<uint8_t> read_file(const std::string& filename, 
                                         bool validate_checksum = true,
                                         bool validate_structure = true) {
        // Validate checksum if requested
        if (validate_checksum) {
            auto result = FormatValidator::validate_file_checksum(filename);
            if (!result.valid) {
                throw ValidationException(result);
            }
        }
        
        // Validate structure if requested
        if (validate_structure) {
            auto result = FormatValidator::validate_structure(filename);
            if (!result.valid) {
                throw ValidationException(result);
            }
        }
        
        // Read the file
        return BinaryReader::read_file(filename);
    }
    
    /**
     * @brief Create reader from file with validation
     */
    static std::unique_ptr<BinaryReader> from_file(const std::string& filename,
                                                   bool validate_checksum = true,
                                                   bool validate_structure = true) {
        auto buffer = read_file(filename, validate_checksum, validate_structure);
        return std::make_unique<BinaryReader>(std::move(buffer));
    }
};

/**
 * @brief Streaming writer with validation support
 */
class ValidatedStreamingWriter : public StreamingWriter {
public:
    ValidatedStreamingWriter(const std::string& filename, 
                           const StreamingConfig& config = StreamingConfig())
        : StreamingWriter(filename, config)
        , filename_(filename) {}
    
    ~ValidatedStreamingWriter() {
        if (!finalized_ && !interrupted_) {
            try {
                finalize();
            } catch (...) {
                // Suppress exceptions in destructor
            }
        }
    }
    
    /**
     * @brief Finalize with validation
     */
    void finalize_with_validation() {
        finalize();
        
        // Validate the written file
        auto result = FormatValidator::validate_structure(filename_);
        if (!result.valid) {
            throw ValidationException(result);
        }
    }
    
private:
    std::string filename_;
    bool finalized_ = false;
};

/**
 * @brief Streaming reader with validation support
 */
class ValidatedStreamingReader : public StreamingReader {
public:
    ValidatedStreamingReader(const std::string& filename, 
                           const StreamingConfig& config = StreamingConfig())
        : StreamingReader(filename, config) {
        
        // File header is already validated in StreamingReader constructor
        // Additional validation can be done here if needed
    }
    
    /**
     * @brief Validate file before reading
     */
    static ValidationResult validate_file(const std::string& filename) {
        return FormatValidator::validate_structure(filename);
    }
};

/**
 * @brief Safe file reader with corruption detection
 */
class SafeFileReader {
public:
    /**
     * @brief Read file with comprehensive validation
     */
    static std::vector<uint8_t> read_with_recovery(const std::string& filename) {
        // First try normal validation
        auto checksum_result = FormatValidator::validate_file_checksum(filename);
        auto structure_result = FormatValidator::validate_structure(filename);
        
        if (checksum_result.valid && structure_result.valid) {
            // File is valid, read normally
            return BinaryReader::read_file(filename);
        }
        
        // File has issues, try to recover what we can
        std::vector<uint8_t> recovered_data;
        
        try {
            std::ifstream file(filename, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Cannot open file: " + filename);
            }
            
            // Read file header
            FileHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
            
            // Convert from file endianness
            header.magic = from_file_endian(header.magic);
            header.version_major = from_file_endian(header.version_major);
            header.version_minor = from_file_endian(header.version_minor);
            header.flags = from_file_endian(header.flags);
            header.total_size = from_file_endian(header.total_size);
            header.data_offset = from_file_endian(header.data_offset);
            header.object_count = from_file_endian(header.object_count);
            
            // Try to read data section
            file.seekg(header.data_offset);
            size_t data_size = header.total_size - header.data_offset;
            
            recovered_data.resize(data_size);
            file.read(reinterpret_cast<char*>(recovered_data.data()), data_size);
            
            // Log warnings about what was wrong
            if (!checksum_result.valid) {
                // Checksum failed but structure is OK
                // Data might be corrupted but is readable
            }
            
            if (!structure_result.valid) {
                // Structure validation failed
                // Some objects might be corrupted
            }
            
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to recover file: " + std::string(e.what()));
        }
        
        return recovered_data;
    }
};

} // namespace serialization
} // namespace claude_draw