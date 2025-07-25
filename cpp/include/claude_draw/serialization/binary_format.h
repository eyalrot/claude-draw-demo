#pragma once

#include <cstdint>
#include <cstring>
#include <array>

namespace claude_draw {
namespace serialization {

/**
 * @brief Binary format specification for Claude Draw
 * 
 * This format is designed for:
 * - Fast serialization/deserialization
 * - Compact storage
 * - Version compatibility
 * - Zero-copy where possible
 * - Optional compression
 */

// Format constants
constexpr uint32_t MAGIC_NUMBER = 0x43445246;  // "CDRF" (Claude Draw Format)
constexpr uint16_t CURRENT_VERSION_MAJOR = 1;
constexpr uint16_t CURRENT_VERSION_MINOR = 0;

// Type identifiers
enum class TypeId : uint8_t {
    None = 0,
    
    // Core types
    Point2D = 1,
    Color = 2,
    Transform2D = 3,
    BoundingBox = 4,
    
    // Shapes
    Circle = 10,
    Rectangle = 11,
    Line = 12,
    Ellipse = 13,
    
    // Containers
    Group = 20,
    Layer = 21,
    Drawing = 22,
    
    // Special
    Reference = 30,  // For object references
    Array = 31,      // For arrays
    
    // Reserved for future use
    Reserved = 255
};

// Compression types
enum class CompressionType : uint8_t {
    None = 0,
    LZ4 = 1,
    Zstd = 2
};

// Flags
enum class FormatFlags : uint32_t {
    None = 0,
    Compressed = 1 << 0,
    Indexed = 1 << 1,      // Uses object index for references
    Streaming = 1 << 2,    // Supports streaming read/write
    ChecksumCRC32 = 1 << 3 // Has CRC32 checksum
};

// Enable bitwise operations on FormatFlags
inline FormatFlags operator|(FormatFlags a, FormatFlags b) {
    return static_cast<FormatFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline FormatFlags operator&(FormatFlags a, FormatFlags b) {
    return static_cast<FormatFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline FormatFlags& operator|=(FormatFlags& a, FormatFlags b) {
    a = a | b;
    return a;
}

inline FormatFlags& operator&=(FormatFlags& a, FormatFlags b) {
    a = a & b;
    return a;
}

/**
 * @brief Main file header
 */
struct FileHeader {
    uint32_t magic;              // Magic number (4 bytes)
    uint16_t version_major;      // Major version (2 bytes)
    uint16_t version_minor;      // Minor version (2 bytes)
    uint32_t flags;              // Format flags (4 bytes)
    uint64_t total_size;         // Total file size (8 bytes)
    uint64_t data_offset;        // Offset to data section (8 bytes)
    uint32_t object_count;       // Number of objects (4 bytes)
    uint32_t checksum;           // CRC32 checksum (4 bytes)
    CompressionType compression; // Compression type (1 byte)
    uint8_t reserved[55];        // Reserved for future use (55 bytes to make total 96)
    
    FileHeader() {
        std::memset(this, 0, sizeof(FileHeader));
        magic = MAGIC_NUMBER;
        version_major = CURRENT_VERSION_MAJOR;
        version_minor = CURRENT_VERSION_MINOR;
        data_offset = sizeof(FileHeader);
    }
    
    bool validate() const {
        return magic == MAGIC_NUMBER &&
               version_major <= CURRENT_VERSION_MAJOR;
    }
};

static_assert(sizeof(FileHeader) == 96, "FileHeader must be 96 bytes");

/**
 * @brief Object header for each serialized object
 */
struct ObjectHeader {
    TypeId type;          // Object type
    uint8_t flags;        // Object-specific flags
    uint16_t reserved;    // Reserved/padding
    uint32_t object_id;   // Unique object ID (for references)
    uint32_t data_size;   // Size of object data
    
    ObjectHeader() : type(TypeId::None), flags(0), reserved(0), 
                    object_id(0), data_size(0) {}
    
    ObjectHeader(TypeId t, uint32_t id, uint32_t size)
        : type(t), flags(0), reserved(0), object_id(id), data_size(size) {}
};

static_assert(sizeof(ObjectHeader) == 12, "ObjectHeader must be 12 bytes");

/**
 * @brief Array header for serialized arrays
 */
struct ArrayHeader {
    TypeId element_type;  // Type of array elements
    uint8_t flags;        // Array flags
    uint16_t reserved;    // Reserved/padding
    uint32_t count;       // Number of elements
    
    ArrayHeader() : element_type(TypeId::None), flags(0), 
                   reserved(0), count(0) {}
    
    ArrayHeader(TypeId type, uint32_t n)
        : element_type(type), flags(0), reserved(0), count(n) {}
};

static_assert(sizeof(ArrayHeader) == 8, "ArrayHeader must be 8 bytes");

/**
 * @brief Reference to another object
 */
struct ObjectReference {
    uint32_t object_id;   // ID of referenced object
    
    explicit ObjectReference(uint32_t id = 0) : object_id(id) {}
};

/**
 * @brief Chunk header for streaming format
 */
struct ChunkHeader {
    uint32_t chunk_size;  // Size of this chunk
    uint32_t object_count; // Objects in this chunk
    uint32_t flags;       // Chunk flags
    uint32_t checksum;    // Chunk checksum
    
    ChunkHeader() : chunk_size(0), object_count(0), 
                   flags(0), checksum(0) {}
};

// Helper functions
inline uint32_t align_size(uint32_t size, uint32_t alignment = 8) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Endianness helpers (for network byte order)
inline uint16_t to_big_endian(uint16_t value) {
#ifdef _MSC_VER
    return _byteswap_ushort(value);
#else
    return __builtin_bswap16(value);
#endif
}

inline uint32_t to_big_endian(uint32_t value) {
#ifdef _MSC_VER
    return _byteswap_ulong(value);
#else
    return __builtin_bswap32(value);
#endif
}

inline uint64_t to_big_endian(uint64_t value) {
#ifdef _MSC_VER
    return _byteswap_uint64(value);
#else
    return __builtin_bswap64(value);
#endif
}

// Check system endianness
inline bool is_big_endian() {
    union {
        uint32_t i;
        char c[4];
    } test = {0x01020304};
    return test.c[0] == 1;
}

// Conditional endian conversion
template<typename T>
inline T to_file_endian(T value) {
    if (is_big_endian()) {
        return value;
    } else {
        return to_big_endian(value);
    }
}

template<typename T>
inline T from_file_endian(T value) {
    return to_file_endian(value);  // Same operation
}

} // namespace serialization
} // namespace claude_draw