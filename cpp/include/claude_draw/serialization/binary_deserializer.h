#pragma once

#include "claude_draw/serialization/binary_format.h"
#include <vector>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <cstring>

namespace claude_draw {
namespace serialization {

/**
 * @brief Binary deserializer for Claude Draw objects
 * 
 * Provides efficient binary deserialization with:
 * - Type validation
 * - Object reference resolution
 * - Streaming support
 * - Zero-copy access where possible
 * - Version compatibility
 */
class BinaryReader {
public:
    BinaryReader() : data_(nullptr), size_(0), offset_(0) {}
    
    explicit BinaryReader(const std::vector<uint8_t>& buffer)
        : data_(buffer.data())
        , size_(buffer.size())
        , offset_(0) {}
    
    BinaryReader(const uint8_t* data, size_t size)
        : data_(data)
        , size_(size)
        , offset_(0) {}
    
    // Core read methods
    FileHeader read_file_header() {
        FileHeader header;
        read_raw(&header, sizeof(FileHeader));
        
        // Convert endianness if needed
        header.magic = from_file_endian(header.magic);
        header.version_major = from_file_endian(header.version_major);
        header.version_minor = from_file_endian(header.version_minor);
        header.flags = from_file_endian(header.flags);
        header.total_size = from_file_endian(header.total_size);
        header.data_offset = from_file_endian(header.data_offset);
        header.object_count = from_file_endian(header.object_count);
        header.checksum = from_file_endian(header.checksum);
        
        if (!header.validate()) {
            throw std::runtime_error("Invalid file header");
        }
        
        return header;
    }
    
    ObjectHeader read_object_header() {
        ObjectHeader header;
        read_raw(&header, sizeof(ObjectHeader));
        
        header.object_id = from_file_endian(header.object_id);
        header.data_size = from_file_endian(header.data_size);
        
        return header;
    }
    
    ArrayHeader read_array_header() {
        ArrayHeader header;
        read_raw(&header, sizeof(ArrayHeader));
        
        header.count = from_file_endian(header.count);
        
        return header;
    }
    
    // Primitive type readers
    uint8_t read_uint8() {
        check_available(1);
        return data_[offset_++];
    }
    
    uint16_t read_uint16() {
        uint16_t value;
        read_raw(&value, sizeof(value));
        return from_file_endian(value);
    }
    
    uint32_t read_uint32() {
        uint32_t value;
        read_raw(&value, sizeof(value));
        return from_file_endian(value);
    }
    
    uint64_t read_uint64() {
        uint64_t value;
        read_raw(&value, sizeof(value));
        return from_file_endian(value);
    }
    
    float read_float() {
        float value;
        read_raw(&value, sizeof(value));
        return value;
    }
    
    double read_double() {
        double value;
        read_raw(&value, sizeof(value));
        return value;
    }
    
    std::string read_string() {
        uint32_t length = read_uint32();
        check_available(length);
        
        std::string result(reinterpret_cast<const char*>(data_ + offset_), length);
        offset_ += length;
        return result;
    }
    
    void read_bytes(void* buffer, size_t size) {
        read_raw(buffer, size);
    }
    
    // Zero-copy access
    const void* get_current_ptr() const {
        return data_ + offset_;
    }
    
    template<typename T>
    const T* get_object_ptr() {
        check_available(sizeof(T));
        const T* ptr = reinterpret_cast<const T*>(data_ + offset_);
        offset_ += sizeof(T);
        return ptr;
    }
    
    // Navigation
    void seek(size_t position) {
        if (position > size_) {
            throw std::runtime_error("Seek position out of bounds");
        }
        offset_ = position;
    }
    
    void skip(size_t bytes) {
        check_available(bytes);
        offset_ += bytes;
    }
    
    size_t tell() const { return offset_; }
    size_t remaining() const { return size_ - offset_; }
    bool eof() const { return offset_ >= size_; }
    
    // File I/O
    static std::vector<uint8_t> read_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open file for reading: " + filename);
        }
        
        size_t file_size = file.tellg();
        file.seekg(0);
        
        std::vector<uint8_t> buffer(file_size);
        file.read(reinterpret_cast<char*>(buffer.data()), file_size);
        
        if (!file) {
            throw std::runtime_error("Failed to read file: " + filename);
        }
        
        return buffer;
    }
    
    // Object index for reference resolution
    void register_object(uint32_t id, void* obj) {
        object_index_[id] = obj;
    }
    
    void* get_object(uint32_t id) const {
        auto it = object_index_.find(id);
        return (it != object_index_.end()) ? it->second : nullptr;
    }
    
    template<typename T>
    T* get_object_typed(uint32_t id) const {
        return static_cast<T*>(get_object(id));
    }

private:
    void check_available(size_t bytes) const {
        if (offset_ + bytes > size_) {
            throw std::runtime_error("Buffer underflow");
        }
    }
    
    void read_raw(void* buffer, size_t size) {
        check_available(size);
        std::memcpy(buffer, data_ + offset_, size);
        offset_ += size;
    }
    
    const uint8_t* data_;
    size_t size_;
    mutable size_t offset_;
    mutable std::unordered_map<uint32_t, void*> object_index_;
};

/**
 * @brief Streaming binary reader for large files
 */
class StreamingBinaryReader {
public:
    explicit StreamingBinaryReader(const std::string& filename, size_t buffer_size = 1024 * 1024)
        : file_(filename, std::ios::binary)
        , buffer_(buffer_size)
        , buffer_offset_(0)
        , buffer_valid_(0) {
        
        if (!file_) {
            throw std::runtime_error("Failed to open file for streaming read: " + filename);
        }
        
        // Read and validate file header
        file_.read(reinterpret_cast<char*>(&file_header_), sizeof(FileHeader));
        if (!file_ || !file_header_.validate()) {
            throw std::runtime_error("Invalid file header");
        }
        
        // Seek to data section
        file_.seekg(file_header_.data_offset);
    }
    
    const FileHeader& get_file_header() const { return file_header_; }
    
    // Read next object from stream
    bool read_next_object(ObjectHeader& header, std::vector<uint8_t>& data) {
        // Ensure we have a chunk header
        if (buffer_valid_ == 0 || buffer_offset_ >= buffer_valid_) {
            if (!read_next_chunk()) {
                return false;  // No more chunks
            }
        }
        
        // Read object header
        if (buffer_offset_ + sizeof(ObjectHeader) > buffer_valid_) {
            return false;  // Incomplete object
        }
        
        std::memcpy(&header, &buffer_[buffer_offset_], sizeof(ObjectHeader));
        buffer_offset_ += sizeof(ObjectHeader);
        
        // Convert endianness
        header.object_id = from_file_endian(header.object_id);
        header.data_size = from_file_endian(header.data_size);
        
        // Read object data
        if (buffer_offset_ + header.data_size > buffer_valid_) {
            return false;  // Incomplete object
        }
        
        data.resize(header.data_size);
        std::memcpy(data.data(), &buffer_[buffer_offset_], header.data_size);
        buffer_offset_ += header.data_size;
        
        objects_read_++;
        return true;
    }
    
    bool eof() const {
        return file_.eof() && buffer_offset_ >= buffer_valid_;
    }
    
    uint32_t objects_read() const { return objects_read_; }

private:
    bool read_next_chunk() {
        if (file_.eof()) {
            return false;
        }
        
        ChunkHeader chunk_header;
        file_.read(reinterpret_cast<char*>(&chunk_header), sizeof(ChunkHeader));
        if (!file_) {
            return false;
        }
        
        // Convert endianness
        chunk_header.chunk_size = from_file_endian(chunk_header.chunk_size);
        chunk_header.object_count = from_file_endian(chunk_header.object_count);
        
        // Read chunk data
        if (chunk_header.chunk_size > buffer_.size()) {
            buffer_.resize(chunk_header.chunk_size);
        }
        
        file_.read(reinterpret_cast<char*>(buffer_.data()), chunk_header.chunk_size);
        if (!file_) {
            return false;
        }
        
        buffer_offset_ = 0;
        buffer_valid_ = chunk_header.chunk_size;
        current_chunk_objects_ = chunk_header.object_count;
        
        return true;
    }
    
    std::ifstream file_;
    FileHeader file_header_;
    std::vector<uint8_t> buffer_;
    size_t buffer_offset_;
    size_t buffer_valid_;
    uint32_t current_chunk_objects_ = 0;
    uint32_t objects_read_ = 0;
};

/**
 * @brief Zero-copy reader for memory-mapped files
 */
class MappedBinaryReader {
public:
    MappedBinaryReader(const void* mapped_memory, size_t size)
        : memory_(static_cast<const uint8_t*>(mapped_memory))
        , size_(size)
        , offset_(0) {
        
        // Validate header
        if (size < sizeof(FileHeader)) {
            throw std::runtime_error("Memory mapped file too small");
        }
        
        const FileHeader* header = reinterpret_cast<const FileHeader*>(memory_);
        if (!header->validate()) {
            throw std::runtime_error("Invalid file header in memory mapped file");
        }
        
        file_header_ = *header;
        offset_ = file_header_.data_offset;
    }
    
    const FileHeader& get_file_header() const { return file_header_; }
    
    template<typename T>
    const T* read_object() {
        if (offset_ + sizeof(ObjectHeader) + sizeof(T) > size_) {
            return nullptr;
        }
        
        // Skip object header
        offset_ += sizeof(ObjectHeader);
        
        // Get object pointer
        const T* obj = reinterpret_cast<const T*>(memory_ + offset_);
        offset_ += sizeof(T);
        
        // Align to 8 bytes
        offset_ = align_size(static_cast<uint32_t>(offset_), 8);
        
        return obj;
    }
    
    bool has_next() const {
        return offset_ + sizeof(ObjectHeader) <= size_;
    }
    
    void reset() {
        offset_ = file_header_.data_offset;
    }

private:
    const uint8_t* memory_;
    size_t size_;
    size_t offset_;
    FileHeader file_header_;
};

} // namespace serialization
} // namespace claude_draw