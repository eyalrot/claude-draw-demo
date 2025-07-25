#pragma once

#include "claude_draw/serialization/binary_format.h"
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>
#include <fstream>

namespace claude_draw {
namespace serialization {

/**
 * @brief CRC32 checksum calculator
 */
class CRC32 {
public:
    CRC32() : crc_(0xFFFFFFFF) {
        // Initialize CRC table
        if (!table_initialized_) {
            initialize_table();
            table_initialized_ = true;
        }
    }
    
    void update(const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        for (size_t i = 0; i < size; ++i) {
            crc_ = (crc_ >> 8) ^ crc_table_[(crc_ ^ bytes[i]) & 0xFF];
        }
    }
    
    uint32_t finalize() const {
        return crc_ ^ 0xFFFFFFFF;
    }
    
    static uint32_t calculate(const void* data, size_t size) {
        CRC32 crc;
        crc.update(data, size);
        return crc.finalize();
    }
    
private:
    static void initialize_table() {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t crc = i;
            for (int j = 0; j < 8; ++j) {
                crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
            }
            crc_table_[i] = crc;
        }
    }
    
    uint32_t crc_;
    static uint32_t crc_table_[256];
    static bool table_initialized_;
};

// Static member definitions
inline uint32_t CRC32::crc_table_[256];
inline bool CRC32::table_initialized_ = false;

/**
 * @brief Validation result structure
 */
struct ValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    
    void add_error(const std::string& error) {
        valid = false;
        errors.push_back(error);
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    std::string to_string() const {
        std::string result = valid ? "Valid" : "Invalid";
        
        if (!errors.empty()) {
            result += "\nErrors:";
            for (const auto& error : errors) {
                result += "\n  - " + error;
            }
        }
        
        if (!warnings.empty()) {
            result += "\nWarnings:";
            for (const auto& warning : warnings) {
                result += "\n  - " + warning;
            }
        }
        
        return result;
    }
};

/**
 * @brief Format validator for binary serialization
 */
class FormatValidator {
public:
    /**
     * @brief Validate a file header
     */
    static ValidationResult validate_header(const FileHeader& header) {
        ValidationResult result;
        
        // Check magic number
        if (header.magic != MAGIC_NUMBER) {
            result.add_error("Invalid magic number: " + std::to_string(header.magic));
        }
        
        // Check version
        if (header.version_major > CURRENT_VERSION_MAJOR) {
            result.add_error("Unsupported major version: " + 
                           std::to_string(header.version_major));
        } else if (header.version_major == CURRENT_VERSION_MAJOR &&
                   header.version_minor > CURRENT_VERSION_MINOR) {
            result.add_warning("Newer minor version: " + 
                             std::to_string(header.version_minor));
        }
        
        // Check data offset
        if (header.data_offset < sizeof(FileHeader)) {
            result.add_error("Invalid data offset: " + 
                           std::to_string(header.data_offset));
        }
        
        // Check object count vs file size
        if (header.object_count > 0 && header.total_size <= header.data_offset) {
            result.add_error("File size too small for object count");
        }
        
        // Check compression type (Zstd is value 2)
        if (static_cast<int>(header.compression) > 2) {
            result.add_error("Unknown compression type: " + 
                           std::to_string(static_cast<int>(header.compression)));
        }
        
        // Check flags consistency
        if ((header.flags & static_cast<uint32_t>(FormatFlags::Compressed)) &&
            header.compression == CompressionType::None) {
            result.add_warning("Compressed flag set but no compression specified");
        }
        
        return result;
    }
    
    /**
     * @brief Validate an object header
     */
    static ValidationResult validate_object_header(const ObjectHeader& header,
                                                  size_t available_bytes) {
        ValidationResult result;
        
        // Check type ID - Reserved is the maximum value
        if (header.type > TypeId::Reserved) {
            result.add_error("Invalid type ID: " + 
                           std::to_string(static_cast<int>(header.type)));
        }
        
        // Check data size
        if (header.data_size == 0 && header.type != TypeId::None) {
            result.add_warning("Zero data size for non-null type");
        }
        
        if (header.data_size > available_bytes) {
            result.add_error("Object data size exceeds available bytes: " +
                           std::to_string(header.data_size) + " > " +
                           std::to_string(available_bytes));
        }
        
        // Sanity check for extremely large objects
        const size_t MAX_OBJECT_SIZE = 100 * 1024 * 1024; // 100MB
        if (header.data_size > MAX_OBJECT_SIZE) {
            result.add_warning("Unusually large object size: " +
                             std::to_string(header.data_size));
        }
        
        return result;
    }
    
    /**
     * @brief Validate file integrity with checksum
     */
    static ValidationResult validate_file_checksum(const std::string& filename) {
        ValidationResult result;
        
        try {
            std::ifstream file(filename, std::ios::binary);
            if (!file) {
                result.add_error("Failed to open file: " + filename);
                return result;
            }
            
            // Read header
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
            header.checksum = from_file_endian(header.checksum);
            
            // Validate header first
            result = validate_header(header);
            if (!result.valid) {
                return result;
            }
            
            // Calculate checksum of data section
            file.seekg(header.data_offset);
            size_t data_size = header.total_size - header.data_offset;
            
            if (data_size > 0) {
                std::vector<uint8_t> buffer(std::min(data_size, size_t(1024 * 1024)));
                CRC32 crc;
                
                size_t remaining = data_size;
                while (remaining > 0 && file) {
                    size_t to_read = std::min(remaining, buffer.size());
                    file.read(reinterpret_cast<char*>(buffer.data()), to_read);
                    size_t read = file.gcount();
                    
                    if (read == 0) break;
                    
                    crc.update(buffer.data(), read);
                    remaining -= read;
                }
                
                uint32_t calculated_checksum = crc.finalize();
                
                if (header.checksum != 0 && header.checksum != calculated_checksum) {
                    result.add_error("Checksum mismatch: expected " +
                                   std::to_string(header.checksum) +
                                   ", calculated " +
                                   std::to_string(calculated_checksum));
                }
            }
            
        } catch (const std::exception& e) {
            result.add_error("Exception during validation: " + std::string(e.what()));
        }
        
        return result;
    }
    
    /**
     * @brief Validate structure integrity by parsing all objects
     */
    static ValidationResult validate_structure(const std::string& filename) {
        ValidationResult result;
        
        try {
            std::ifstream file(filename, std::ios::binary);
            if (!file) {
                result.add_error("Failed to open file: " + filename);
                return result;
            }
            
            // Read and validate header
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
            header.checksum = from_file_endian(header.checksum);
            
            result = validate_header(header);
            if (!result.valid) {
                return result;
            }
            
            // Seek to data section
            file.seekg(header.data_offset);
            
            // Parse all objects
            size_t objects_found = 0;
            size_t bytes_read = header.data_offset;
            
            while (file && bytes_read < header.total_size) {
                // Read object header
                ObjectHeader obj_header;
                file.read(reinterpret_cast<char*>(&obj_header), sizeof(ObjectHeader));
                
                if (!file) {
                    if (objects_found < header.object_count) {
                        result.add_error("Unexpected end of file while reading object " +
                                       std::to_string(objects_found));
                    }
                    break;
                }
                
                // Convert from file endianness
                obj_header.object_id = from_file_endian(obj_header.object_id);
                obj_header.data_size = from_file_endian(obj_header.data_size);
                
                bytes_read += sizeof(ObjectHeader);
                
                // Validate object header
                size_t remaining = header.total_size - bytes_read;
                auto obj_result = validate_object_header(obj_header, remaining);
                
                if (!obj_result.valid) {
                    for (const auto& error : obj_result.errors) {
                        result.add_error("Object " + std::to_string(objects_found) + 
                                       ": " + error);
                    }
                    return result;
                }
                
                // Skip object data
                file.seekg(obj_header.data_size, std::ios::cur);
                bytes_read += obj_header.data_size;
                objects_found++;
                
                if (objects_found > header.object_count) {
                    result.add_error("Found more objects than declared in header");
                    break;
                }
            }
            
            if (objects_found != header.object_count) {
                result.add_error("Object count mismatch: found " +
                               std::to_string(objects_found) +
                               ", expected " +
                               std::to_string(header.object_count));
            }
            
            if (bytes_read != header.total_size) {
                result.add_warning("File size mismatch: read " +
                                 std::to_string(bytes_read) +
                                 ", expected " +
                                 std::to_string(header.total_size));
            }
            
        } catch (const std::exception& e) {
            result.add_error("Exception during structure validation: " + 
                           std::string(e.what()));
        }
        
        return result;
    }
};

/**
 * @brief Exception thrown when validation fails
 */
class ValidationException : public std::runtime_error {
public:
    ValidationException(const ValidationResult& result)
        : std::runtime_error("Validation failed: " + result.to_string())
        , result_(result) {}
    
    const ValidationResult& result() const { return result_; }
    
private:
    ValidationResult result_;
};

} // namespace serialization
} // namespace claude_draw