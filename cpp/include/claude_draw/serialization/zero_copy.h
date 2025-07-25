#pragma once

#include "claude_draw/serialization/binary_format.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/transform2d.h"
#include "claude_draw/core/bounding_box.h"
#include <cstring>
#include <type_traits>
#include <memory>
#include <new>

namespace claude_draw {
namespace serialization {

/**
 * @brief Zero-copy serialization utilities
 * 
 * Provides direct memory access for efficient serialization/deserialization
 * with proper alignment and type safety.
 */

// Alignment requirements
constexpr size_t CACHE_LINE_SIZE = 64;
constexpr size_t DEFAULT_ALIGNMENT = 8;

// Type trait to check if a type is trivially copyable
template<typename T>
constexpr bool is_zero_copy_safe_v = std::is_trivially_copyable_v<T> && 
                                     std::is_standard_layout_v<T>;

/**
 * @brief Aligned memory allocator for zero-copy operations
 */
class AlignedAllocator {
public:
    static void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) {
        void* ptr = nullptr;
#ifdef _WIN32
        ptr = _aligned_malloc(size, alignment);
#else
        if (posix_memalign(&ptr, alignment, size) != 0) {
            ptr = nullptr;
        }
#endif
        if (!ptr) {
            throw std::bad_alloc();
        }
        return ptr;
    }
    
    static void deallocate(void* ptr) {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        free(ptr);
#endif
    }
};

/**
 * @brief Memory region for zero-copy operations
 */
class MemoryRegion {
public:
    MemoryRegion(size_t size, size_t alignment = DEFAULT_ALIGNMENT)
        : size_(size)
        , alignment_(alignment)
        , data_(static_cast<uint8_t*>(AlignedAllocator::allocate(size, alignment)))
        , offset_(0) {
        std::memset(data_, 0, size);
    }
    
    ~MemoryRegion() {
        if (data_) {
            AlignedAllocator::deallocate(data_);
        }
    }
    
    // Non-copyable
    MemoryRegion(const MemoryRegion&) = delete;
    MemoryRegion& operator=(const MemoryRegion&) = delete;
    
    // Movable
    MemoryRegion(MemoryRegion&& other) noexcept
        : size_(other.size_)
        , alignment_(other.alignment_)
        , data_(other.data_)
        , offset_(other.offset_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.offset_ = 0;
    }
    
    MemoryRegion& operator=(MemoryRegion&& other) noexcept {
        if (this != &other) {
            if (data_) {
                AlignedAllocator::deallocate(data_);
            }
            size_ = other.size_;
            alignment_ = other.alignment_;
            data_ = other.data_;
            offset_ = other.offset_;
            other.data_ = nullptr;
            other.size_ = 0;
            other.offset_ = 0;
        }
        return *this;
    }
    
    // Allocate space for an object
    template<typename T>
    T* allocate() {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable for zero-copy");
        
        // Align offset
        offset_ = align_size(static_cast<uint32_t>(offset_), alignof(T));
        
        // Check space
        if (offset_ + sizeof(T) > size_) {
            throw std::runtime_error("MemoryRegion out of space");
        }
        
        T* ptr = reinterpret_cast<T*>(data_ + offset_);
        offset_ += sizeof(T);
        return ptr;
    }
    
    // Allocate space for an array
    template<typename T>
    T* allocate_array(size_t count) {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable for zero-copy");
        
        // Align offset
        offset_ = align_size(static_cast<uint32_t>(offset_), alignof(T));
        
        // Check space
        size_t required = sizeof(T) * count;
        if (offset_ + required > size_) {
            throw std::runtime_error("MemoryRegion out of space");
        }
        
        T* ptr = reinterpret_cast<T*>(data_ + offset_);
        offset_ += required;
        return ptr;
    }
    
    // Get pointer at specific offset
    template<typename T>
    T* get_at(size_t offset) {
        if (offset + sizeof(T) > size_) {
            throw std::runtime_error("Offset out of bounds");
        }
        return reinterpret_cast<T*>(data_ + offset);
    }
    
    // Get const pointer at specific offset
    template<typename T>
    const T* get_at(size_t offset) const {
        if (offset + sizeof(T) > size_) {
            throw std::runtime_error("Offset out of bounds");
        }
        return reinterpret_cast<const T*>(data_ + offset);
    }
    
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    size_t size() const { return size_; }
    size_t offset() const { return offset_; }
    size_t remaining() const { return size_ - offset_; }
    
    void reset() { offset_ = 0; }
    
private:
    size_t size_;
    size_t alignment_;
    uint8_t* data_;
    size_t offset_;
};

/**
 * @brief Zero-copy serialization context
 */
class ZeroCopyWriter {
public:
    explicit ZeroCopyWriter(MemoryRegion& region)
        : region_(region)
        , object_count_(0) {
        
        // Reserve space for file header
        file_header_ = region_.allocate<FileHeader>();
        new (file_header_) FileHeader();
    }
    
    // Write primitive types
    template<typename T>
    void write(const T& value) {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable");
        T* ptr = region_.allocate<T>();
        *ptr = value;
    }
    
    // Write arrays
    template<typename T>
    void write_array(const T* data, size_t count) {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable");
        T* ptr = region_.allocate_array<T>(count);
        std::memcpy(ptr, data, sizeof(T) * count);
    }
    
    // Write with custom alignment
    template<typename T>
    T* write_aligned(const T& value, size_t alignment) {
        // Manually align
        size_t current = region_.offset();
        size_t aligned = ((current + alignment - 1) / alignment) * alignment;
        if (aligned > current) {
            region_.allocate_array<uint8_t>(aligned - current);
        }
        
        T* ptr = region_.allocate<T>();
        *ptr = value;
        return ptr;
    }
    
    // Direct object construction
    template<typename T, typename... Args>
    T* construct(Args&&... args) {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable");
        T* ptr = region_.allocate<T>();
        new (ptr) T(std::forward<Args>(args)...);
        object_count_++;
        return ptr;
    }
    
    // Write object with header
    template<typename T>
    T* write_object(TypeId type, uint32_t id, const T& obj) {
        // Write object header
        ObjectHeader* header = region_.allocate<ObjectHeader>();
        header->type = type;
        header->flags = 0;
        header->reserved = 0;
        header->object_id = to_file_endian(id);
        header->data_size = to_file_endian(static_cast<uint32_t>(sizeof(T)));
        
        // Write object data
        T* ptr = region_.allocate<T>();
        *ptr = obj;
        
        object_count_++;
        return ptr;
    }
    
    void finalize() {
        file_header_->object_count = to_file_endian(object_count_);
        file_header_->total_size = to_file_endian(static_cast<uint64_t>(region_.offset()));
        file_header_->flags = to_file_endian(static_cast<uint32_t>(FormatFlags::None));
    }
    
    uint32_t object_count() const { return object_count_; }
    
private:
    MemoryRegion& region_;
    FileHeader* file_header_;
    uint32_t object_count_;
};

/**
 * @brief Zero-copy deserialization context
 */
class ZeroCopyReader {
public:
    ZeroCopyReader(const void* data, size_t size)
        : data_(static_cast<const uint8_t*>(data))
        , size_(size)
        , offset_(0) {
        
        // Read and validate file header
        if (size < sizeof(FileHeader)) {
            throw std::runtime_error("Buffer too small for file header");
        }
        
        file_header_ = get<FileHeader>();
        if (!file_header_->validate()) {
            throw std::runtime_error("Invalid file header");
        }
        
        // Convert endianness
        object_count_ = from_file_endian(file_header_->object_count);
        total_size_ = from_file_endian(file_header_->total_size);
    }
    
    // Read primitive type
    template<typename T>
    const T* read() {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable");
        
        // Align offset
        offset_ = align_size(static_cast<uint32_t>(offset_), alignof(T));
        
        if (offset_ + sizeof(T) > size_) {
            throw std::runtime_error("Buffer underflow");
        }
        
        const T* ptr = reinterpret_cast<const T*>(data_ + offset_);
        offset_ += sizeof(T);
        return ptr;
    }
    
    // Read array
    template<typename T>
    const T* read_array(size_t count) {
        static_assert(is_zero_copy_safe_v<T>, "Type must be trivially copyable");
        
        // Align offset
        offset_ = align_size(static_cast<uint32_t>(offset_), alignof(T));
        
        size_t required = sizeof(T) * count;
        if (offset_ + required > size_) {
            throw std::runtime_error("Buffer underflow");
        }
        
        const T* ptr = reinterpret_cast<const T*>(data_ + offset_);
        offset_ += required;
        return ptr;
    }
    
    // Read object with header
    template<typename T>
    const T* read_object(ObjectHeader& header) {
        const ObjectHeader* hdr = read<ObjectHeader>();
        header = *hdr;
        
        // Convert endianness
        header.object_id = from_file_endian(header.object_id);
        header.data_size = from_file_endian(header.data_size);
        
        if (header.data_size != sizeof(T)) {
            throw std::runtime_error("Object size mismatch");
        }
        
        return read<T>();
    }
    
    // Get pointer at specific offset
    template<typename T>
    const T* get(size_t offset = 0) const {
        if (offset + sizeof(T) > size_) {
            throw std::runtime_error("Offset out of bounds");
        }
        return reinterpret_cast<const T*>(data_ + offset);
    }
    
    // Skip bytes
    void skip(size_t bytes) {
        if (offset_ + bytes > size_) {
            throw std::runtime_error("Skip out of bounds");
        }
        offset_ += bytes;
    }
    
    // Navigation
    void seek(size_t position) {
        if (position > size_) {
            throw std::runtime_error("Seek out of bounds");
        }
        offset_ = position;
    }
    
    size_t tell() const { return offset_; }
    size_t remaining() const { return size_ - offset_; }
    bool eof() const { return offset_ >= size_; }
    
    const FileHeader* file_header() const { return file_header_; }
    uint32_t object_count() const { return object_count_; }
    
private:
    const uint8_t* data_;
    size_t size_;
    size_t offset_;
    const FileHeader* file_header_;
    uint32_t object_count_;
    uint64_t total_size_;
};

/**
 * @brief Memory-mapped file wrapper for zero-copy I/O
 */
class MappedFile {
public:
    static std::unique_ptr<MappedFile> create_for_write(const std::string& filename, size_t size);
    static std::unique_ptr<MappedFile> open_for_read(const std::string& filename);
    
    virtual ~MappedFile() = default;
    
    virtual void* data() = 0;
    virtual const void* data() const = 0;
    virtual size_t size() const = 0;
    virtual void sync() = 0;
    
protected:
    MappedFile() = default;
};

// Specialized serializers for core types
namespace zero_copy {

inline void write_point(ZeroCopyWriter& writer, const Point2D& point) {
    writer.write(point);
}

inline const Point2D* read_point(ZeroCopyReader& reader) {
    return reader.read<Point2D>();
}

inline void write_color(ZeroCopyWriter& writer, const Color& color) {
    writer.write(color);
}

inline const Color* read_color(ZeroCopyReader& reader) {
    return reader.read<Color>();
}

inline void write_transform(ZeroCopyWriter& writer, const Transform2D& transform) {
    // Transform2D might not be trivially copyable, so write elements
    writer.write(transform(0, 0));
    writer.write(transform(0, 1));
    writer.write(transform(0, 2));
    writer.write(transform(1, 0));
    writer.write(transform(1, 1));
    writer.write(transform(1, 2));
}

inline Transform2D read_transform(ZeroCopyReader& reader) {
    float m00 = *reader.read<float>();
    float m01 = *reader.read<float>();
    float m02 = *reader.read<float>();
    float m10 = *reader.read<float>();
    float m11 = *reader.read<float>();
    float m12 = *reader.read<float>();
    return Transform2D(m00, m01, m02, m10, m11, m12);
}

inline void write_bbox(ZeroCopyWriter& writer, const BoundingBox& bbox) {
    writer.write(bbox);
}

inline const BoundingBox* read_bbox(ZeroCopyReader& reader) {
    return reader.read<BoundingBox>();
}

} // namespace zero_copy

} // namespace serialization
} // namespace claude_draw