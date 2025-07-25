#pragma once

#include "claude_draw/serialization/binary_format.h"
#include "claude_draw/memory/arena_allocator.h"
#include <vector>
#include <fstream>
#include <unordered_map>
#include <memory>

namespace claude_draw {
namespace serialization {

/**
 * @brief Binary serializer for Claude Draw objects
 * 
 * Provides efficient binary serialization with:
 * - Type safety via format specification
 * - Object deduplication
 * - Streaming support
 * - Optional compression
 * - Zero-copy where possible
 */
class BinaryWriter {
public:
    explicit BinaryWriter(size_t reserve_size = 1024 * 1024)  // 1MB default
        : arena_(reserve_size)
        , buffer_()
        , object_index_()
        , next_object_id_(1) {
        buffer_.reserve(reserve_size);
    }
    
    // Core write methods
    void write_file_header(const FileHeader& header) {
        // Write with endianness conversion
        FileHeader temp = header;
        temp.magic = to_file_endian(temp.magic);
        temp.version_major = to_file_endian(temp.version_major);
        temp.version_minor = to_file_endian(temp.version_minor);
        temp.flags = to_file_endian(temp.flags);
        temp.total_size = to_file_endian(temp.total_size);
        temp.data_offset = to_file_endian(temp.data_offset);
        temp.object_count = to_file_endian(temp.object_count);
        temp.checksum = to_file_endian(temp.checksum);
        write_raw(&temp, sizeof(FileHeader));
    }
    
    void write_object_header(const ObjectHeader& header) {
        // Write with endianness conversion
        ObjectHeader temp = header;
        temp.object_id = to_file_endian(temp.object_id);
        temp.data_size = to_file_endian(temp.data_size);
        write_raw(&temp, sizeof(ObjectHeader));
    }
    
    void write_array_header(const ArrayHeader& header) {
        // Write with endianness conversion
        ArrayHeader temp = header;
        temp.count = to_file_endian(temp.count);
        write_raw(&temp, sizeof(ArrayHeader));
    }
    
    // Primitive type writers
    void write_uint8(uint8_t value) {
        buffer_.push_back(value);
    }
    
    void write_uint16(uint16_t value) {
        value = to_file_endian(value);
        write_raw(&value, sizeof(value));
    }
    
    void write_uint32(uint32_t value) {
        value = to_file_endian(value);
        write_raw(&value, sizeof(value));
    }
    
    void write_uint64(uint64_t value) {
        value = to_file_endian(value);
        write_raw(&value, sizeof(value));
    }
    
    void write_float(float value) {
        write_raw(&value, sizeof(value));
    }
    
    void write_double(double value) {
        write_raw(&value, sizeof(value));
    }
    
    void write_string(const std::string& str) {
        write_uint32(static_cast<uint32_t>(str.size()));
        write_raw(str.data(), str.size());
    }
    
    void write_bytes(const void* data, size_t size) {
        write_raw(data, size);
    }
    
    // Object reference handling
    uint32_t register_object(const void* obj) {
        auto it = object_index_.find(obj);
        if (it != object_index_.end()) {
            return it->second;
        }
        
        uint32_t id = next_object_id_++;
        object_index_[obj] = id;
        return id;
    }
    
    bool is_registered(const void* obj) const {
        return object_index_.find(obj) != object_index_.end();
    }
    
    uint32_t get_object_id(const void* obj) const {
        auto it = object_index_.find(obj);
        return (it != object_index_.end()) ? it->second : 0;
    }
    
    // Buffer management
    const std::vector<uint8_t>& get_buffer() const { return buffer_; }
    std::vector<uint8_t>& get_buffer() { return buffer_; }
    
    size_t size() const { return buffer_.size(); }
    
    void clear() {
        buffer_.clear();
        object_index_.clear();
        next_object_id_ = 1;
    }
    
    // File I/O
    void write_to_file(const std::string& filename) {
        std::ofstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file for writing: " + filename);
        }
        
        file.write(reinterpret_cast<const char*>(buffer_.data()), buffer_.size());
        if (!file) {
            throw std::runtime_error("Failed to write to file: " + filename);
        }
    }
    
    // Buffer position control
    void seek(size_t position) {
        if (position <= buffer_.size()) {
            // If seeking within current buffer, just update a virtual position
            // For simplicity, we'll just ensure buffer is large enough
            if (position > buffer_.size()) {
                buffer_.resize(position, 0);
            }
        }
    }
    
    // Direct memory access
    void* allocate(size_t size) {
        return arena_.allocate(size);
    }
    
    // Alignment helpers
    void align_to(size_t alignment) {
        size_t current = buffer_.size();
        size_t aligned = align_size(static_cast<uint32_t>(current), static_cast<uint32_t>(alignment));
        if (aligned > current) {
            buffer_.resize(aligned, 0);
        }
    }

private:
    void write_raw(const void* data, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        buffer_.insert(buffer_.end(), bytes, bytes + size);
    }
    
    memory::ArenaAllocator arena_;
    std::vector<uint8_t> buffer_;
    std::unordered_map<const void*, uint32_t> object_index_;
    uint32_t next_object_id_;
};

/**
 * @brief Streaming binary writer for large files
 */
class StreamingBinaryWriter {
public:
    explicit StreamingBinaryWriter(const std::string& filename, size_t chunk_size = 1024 * 1024)
        : file_(filename, std::ios::binary)
        , chunk_writer_(chunk_size)
        , current_chunk_objects_(0) {
        
        if (!file_) {
            throw std::runtime_error("Failed to open file for streaming write: " + filename);
        }
        
        // Reserve space for file header
        FileHeader dummy_header;
        file_.write(reinterpret_cast<const char*>(&dummy_header), sizeof(FileHeader));
    }
    
    ~StreamingBinaryWriter() {
        if (file_.is_open()) {
            finalize();
        }
    }
    
    // Write object to current chunk
    template<typename T>
    void write_object(TypeId type, uint32_t id, const T& obj) {
        size_t start_pos = chunk_writer_.size();
        
        // Write object header
        ObjectHeader header(type, id, 0);  // Size will be updated
        chunk_writer_.write_object_header(header);
        
        // Write object data (template specializations needed)
        write_object_data(obj);
        
        // Update object size
        size_t end_pos = chunk_writer_.size();
        uint32_t object_size = static_cast<uint32_t>(end_pos - start_pos - sizeof(ObjectHeader));
        
        // Write size back into header
        auto& buffer = chunk_writer_.get_buffer();
        uint32_t* size_ptr = reinterpret_cast<uint32_t*>(&buffer[start_pos + offsetof(ObjectHeader, data_size)]);
        *size_ptr = to_file_endian(object_size);
        
        current_chunk_objects_++;
        
        // Flush chunk if it's getting large
        if (chunk_writer_.size() > chunk_writer_.get_buffer().capacity() * 0.8) {
            flush_chunk();
        }
    }
    
    void flush_chunk() {
        if (current_chunk_objects_ == 0) return;
        
        ChunkHeader chunk_header;
        chunk_header.chunk_size = static_cast<uint32_t>(chunk_writer_.size());
        chunk_header.object_count = current_chunk_objects_;
        chunk_header.flags = 0;
        chunk_header.checksum = 0;  // TODO: Calculate CRC32
        
        // Write chunk header
        file_.write(reinterpret_cast<const char*>(&chunk_header), sizeof(ChunkHeader));
        
        // Write chunk data
        file_.write(reinterpret_cast<const char*>(chunk_writer_.get_buffer().data()), 
                   chunk_writer_.size());
        
        // Reset for next chunk
        chunk_writer_.clear();
        current_chunk_objects_ = 0;
        total_objects_ += current_chunk_objects_;
    }
    
    void finalize() {
        // Flush any remaining data
        flush_chunk();
        
        // Update file header
        file_.seekp(0);
        
        FileHeader header;
        header.object_count = total_objects_;
        header.total_size = static_cast<uint64_t>(file_.tellp());
        header.flags = static_cast<uint32_t>(FormatFlags::Streaming);
        
        file_.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));
        file_.close();
    }

private:
    // Template specializations for object types would go here
    template<typename T>
    void write_object_data(const T& obj);
    
    std::ofstream file_;
    BinaryWriter chunk_writer_;
    uint32_t current_chunk_objects_;
    uint32_t total_objects_ = 0;
};

/**
 * @brief Zero-copy writer for memory-mapped files
 */
class MappedBinaryWriter {
public:
    MappedBinaryWriter(void* mapped_memory, size_t size)
        : memory_(static_cast<uint8_t*>(mapped_memory))
        , size_(size)
        , offset_(0) {
        
        // Reserve space for header
        offset_ = sizeof(FileHeader);
    }
    
    template<typename T>
    T* allocate_object() {
        size_t required = sizeof(ObjectHeader) + sizeof(T);
        if (offset_ + required > size_) {
            throw std::runtime_error("Memory mapped file is full");
        }
        
        // Write object header
        ObjectHeader* header = reinterpret_cast<ObjectHeader*>(memory_ + offset_);
        offset_ += sizeof(ObjectHeader);
        
        // Return pointer to object location
        T* obj = reinterpret_cast<T*>(memory_ + offset_);
        offset_ += sizeof(T);
        
        // Align to 8 bytes
        offset_ = align_size(static_cast<uint32_t>(offset_), 8);
        
        return obj;
    }
    
    void finalize(uint32_t object_count) {
        FileHeader* header = reinterpret_cast<FileHeader*>(memory_);
        new (header) FileHeader();
        header->object_count = object_count;
        header->total_size = offset_;
        header->flags = static_cast<uint32_t>(FormatFlags::None);
    }
    
    size_t get_offset() const { return offset_; }

private:
    uint8_t* memory_;
    size_t size_;
    size_t offset_;
};

} // namespace serialization
} // namespace claude_draw