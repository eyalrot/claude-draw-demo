#pragma once

#include "claude_draw/serialization/binary_format.h"
#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/serialization/shape_serializers.h"
#include <string>
#include <memory>

namespace claude_draw {
namespace serialization {

/**
 * @brief High-level serialization API for Claude Draw
 * 
 * Provides simple functions for saving and loading drawings
 * in the binary format with optional compression.
 */
class Serialization {
public:
    /**
     * @brief Save a drawing to a binary file
     * 
     * @param drawing The drawing to save
     * @param filename Output filename
     * @param compression Optional compression type
     * @return true on success, false on failure
     */
    static bool save_drawing(const containers::Drawing& drawing, 
                            const std::string& filename,
                            CompressionType compression = CompressionType::None) {
        try {
            BinaryWriter writer;
            
            // Create file header
            FileHeader header;
            header.compression = compression;
            if (compression != CompressionType::None) {
                header.flags |= static_cast<uint32_t>(FormatFlags::Compressed);
            }
            
            // Reserve space for header
            writer.write_file_header(header);
            
            // Write drawing object
            ObjectHeader obj_header(TypeId::Drawing, 1, 0);
            size_t obj_start = writer.size();
            writer.write_object_header(obj_header);
            
            // Serialize drawing
            serialize(writer, drawing);
            
            // Update object size
            size_t obj_end = writer.size();
            obj_header.data_size = static_cast<uint32_t>(obj_end - obj_start - sizeof(ObjectHeader));
            
            // TODO: Apply compression if requested
            if (compression != CompressionType::None) {
                // Compression implementation would go here
                // For now, we'll skip compression
            }
            
            // Update file header
            header.object_count = 1;
            header.total_size = writer.size();
            
            // Write to file
            writer.write_to_file(filename);
            
            return true;
        } catch (const std::exception& e) {
            // Log error
            return false;
        }
    }
    
    /**
     * @brief Load a drawing from a binary file
     * 
     * @param filename Input filename
     * @return Loaded drawing or nullptr on failure
     */
    static std::unique_ptr<containers::Drawing> load_drawing(const std::string& filename) {
        try {
            // Read file
            auto buffer = BinaryReader::read_file(filename);
            BinaryReader reader(buffer);
            
            // Read and validate header
            FileHeader header = reader.read_file_header();
            
            // Handle compression
            if (header.compression != CompressionType::None) {
                // TODO: Decompress data
                // For now, we'll skip decompression
            }
            
            // Read drawing object
            ObjectHeader obj_header = reader.read_object_header();
            if (obj_header.type != TypeId::Drawing) {
                return nullptr;
            }
            
            // Deserialize drawing
            return deserialize_drawing(reader);
            
        } catch (const std::exception& e) {
            // Log error
            return nullptr;
        }
    }
    
    /**
     * @brief Save a shape to a binary buffer
     * 
     * @param shape The shape to serialize
     * @return Binary buffer containing the serialized shape
     */
    static std::vector<uint8_t> serialize_shape_to_buffer(const shapes::ShapeBase* shape) {
        BinaryWriter writer;
        
        // Determine type
        TypeId type = TypeId::None;
        if (dynamic_cast<const shapes::Circle*>(shape)) {
            type = TypeId::Circle;
        } else if (dynamic_cast<const shapes::Rectangle*>(shape)) {
            type = TypeId::Rectangle;
        } else if (dynamic_cast<const shapes::Line*>(shape)) {
            type = TypeId::Line;
        } else if (dynamic_cast<const shapes::Ellipse*>(shape)) {
            type = TypeId::Ellipse;
        } else if (dynamic_cast<const containers::Group*>(shape)) {
            type = TypeId::Group;
        }
        
        // Write type and data
        writer.write_uint8(static_cast<uint8_t>(type));
        ShapeSerializer::serialize_shape(writer, shape);
        
        return writer.get_buffer();
    }
    
    /**
     * @brief Load a shape from a binary buffer
     * 
     * @param buffer Binary buffer containing serialized shape
     * @return Deserialized shape or nullptr on failure
     */
    static std::unique_ptr<shapes::ShapeBase> deserialize_shape_from_buffer(const std::vector<uint8_t>& buffer) {
        if (buffer.empty()) return nullptr;
        
        BinaryReader reader(buffer);
        TypeId type = static_cast<TypeId>(reader.read_uint8());
        return ShapeSerializer::deserialize_shape(reader, type);
    }
    
    /**
     * @brief Get file format version
     */
    static std::pair<uint16_t, uint16_t> get_format_version() {
        return {CURRENT_VERSION_MAJOR, CURRENT_VERSION_MINOR};
    }
    
    /**
     * @brief Check if a file is a valid Claude Draw binary file
     */
    static bool is_valid_file(const std::string& filename) {
        try {
            std::ifstream file(filename, std::ios::binary);
            if (!file) return false;
            
            FileHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
            if (!file) return false;
            
            return header.validate();
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Get file metadata without loading the entire file
     */
    struct FileMetadata {
        uint16_t version_major;
        uint16_t version_minor;
        uint32_t object_count;
        uint64_t file_size;
        CompressionType compression;
        bool has_checksum;
    };
    
    static std::unique_ptr<FileMetadata> get_file_metadata(const std::string& filename) {
        try {
            std::ifstream file(filename, std::ios::binary);
            if (!file) return nullptr;
            
            FileHeader header;
            file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
            if (!file || !header.validate()) return nullptr;
            
            auto metadata = std::make_unique<FileMetadata>();
            metadata->version_major = header.version_major;
            metadata->version_minor = header.version_minor;
            metadata->object_count = header.object_count;
            metadata->file_size = header.total_size;
            metadata->compression = header.compression;
            metadata->has_checksum = (header.flags & static_cast<uint32_t>(FormatFlags::ChecksumCRC32)) != 0;
            
            return metadata;
        } catch (...) {
            return nullptr;
        }
    }

private:
    // Helper to update buffer with correct size
    static void update_object_size(std::vector<uint8_t>& buffer, size_t header_offset, uint32_t size) {
        uint32_t* size_ptr = reinterpret_cast<uint32_t*>(&buffer[header_offset + offsetof(ObjectHeader, data_size)]);
        *size_ptr = to_file_endian(size);
    }
};

/**
 * @brief Convenience functions
 */

inline bool save_drawing(const containers::Drawing& drawing, const std::string& filename) {
    return Serialization::save_drawing(drawing, filename);
}

inline std::unique_ptr<containers::Drawing> load_drawing(const std::string& filename) {
    return Serialization::load_drawing(filename);
}

} // namespace serialization
} // namespace claude_draw