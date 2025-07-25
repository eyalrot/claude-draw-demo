#pragma once

#include "claude_draw/serialization/version_migration.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include <cstring>

namespace claude_draw {
namespace serialization {

/**
 * @brief Example format changes for demonstration
 * 
 * Version 1.0: Original format
 * - Point2D: x(float), y(float)
 * - Color: r(uint8), g(uint8), b(uint8), a(uint8)
 * 
 * Version 1.1: Added precision flag to Point2D
 * - Point2D: x(float), y(float), high_precision(bool)
 * - Color: unchanged
 * 
 * Version 2.0: Changed Color to float representation
 * - Point2D: x(float), y(float), high_precision(bool)
 * - Color: r(float), g(float), b(float), a(float)
 */

/**
 * @brief Point2D with version 1.1 format (with precision flag)
 */
struct Point2D_v1_1 {
    float x;
    float y;
    bool high_precision;
    
    Point2D_v1_1() : x(0), y(0), high_precision(false) {}
    Point2D_v1_1(float x_, float y_, bool hp = false) 
        : x(x_), y(y_), high_precision(hp) {}
    
    // Convert from current Point2D
    static Point2D_v1_1 from_current(const Point2D& p) {
        return Point2D_v1_1(p.x, p.y, false);
    }
    
    // Convert to current Point2D
    Point2D to_current() const {
        return Point2D(x, y);
    }
};

/**
 * @brief Color with version 2.0 format (float representation)
 */
struct Color_v2_0 {
    float r;
    float g;
    float b;
    float a;
    
    Color_v2_0() : r(0), g(0), b(0), a(1) {}
    Color_v2_0(float r_, float g_, float b_, float a_) 
        : r(r_), g(g_), b(b_), a(a_) {}
    
    // Convert from uint8 Color
    static Color_v2_0 from_uint8(const Color& c) {
        return Color_v2_0(
            c.r / 255.0f,
            c.g / 255.0f,
            c.b / 255.0f,
            c.a / 255.0f
        );
    }
    
    // Convert to uint8 Color
    Color to_uint8() const {
        return Color(
            static_cast<uint8_t>(r * 255.0f),
            static_cast<uint8_t>(g * 255.0f),
            static_cast<uint8_t>(b * 255.0f),
            static_cast<uint8_t>(a * 255.0f)
        );
    }
};

/**
 * @brief Migration from version 1.0 to 1.1
 * Adds high_precision flag to Point2D objects
 */
class Migration_1_0_to_1_1 {
public:
    static MigrationResult migrate(const std::vector<uint8_t>& input,
                                 std::vector<uint8_t>& output) {
        MigrationResult result;
        result.from_version = Version(1, 0);
        result.to_version = Version(1, 1);
        
        try {
            // Check if input has file header or is raw data
            bool has_header = input.size() >= sizeof(FileHeader);
            if (has_header) {
                FileHeader test_header;
                std::memcpy(&test_header, input.data(), sizeof(FileHeader));
                test_header.magic = from_file_endian(test_header.magic);
                has_header = (test_header.magic == MAGIC_NUMBER);
            }
            
            BinaryWriter writer(input.size() + 1024); // Extra space for new fields
            
            if (has_header) {
                BinaryReader reader(input);
                
                // Copy file header with updated version
                FileHeader header = reader.read_file_header();
                header.version_minor = 1;
                writer.write_file_header(header);
                
                // Process each object
                while (!reader.eof()) {
                    ObjectHeader obj_header = reader.read_object_header();
                    
                    if (obj_header.type == TypeId::Point2D) {
                        // Read old format Point2D
                        Point2D old_point;
                        reader.read_bytes(&old_point, sizeof(Point2D));
                        
                        // Convert to new format
                        Point2D_v1_1 new_point = Point2D_v1_1::from_current(old_point);
                        
                        // Write with new size
                        ObjectHeader new_header = obj_header;
                        new_header.data_size = sizeof(Point2D_v1_1);
                        writer.write_object_header(new_header);
                        writer.write_bytes(&new_point, sizeof(Point2D_v1_1));
                        
                        result.add_message("Migrated Point2D object " + 
                                         std::to_string(obj_header.object_id));
                    } else {
                        // Copy other objects unchanged
                        writer.write_object_header(obj_header);
                        std::vector<uint8_t> data(obj_header.data_size);
                        reader.read_bytes(data.data(), obj_header.data_size);
                        writer.write_bytes(data.data(), obj_header.data_size);
                    }
                }
                
                output = writer.get_buffer();
            } else {
                // Raw data without file header - treat as single stream
                BinaryReader reader(input);
                
                // Process raw objects
                while (!reader.eof()) {
                    if (reader.remaining() < sizeof(ObjectHeader)) break;
                    
                    ObjectHeader obj_header = reader.read_object_header();
                    
                    if (obj_header.type == TypeId::Point2D) {
                        // Read old format Point2D
                        Point2D old_point;
                        reader.read_bytes(&old_point, sizeof(Point2D));
                        
                        // Convert to new format
                        Point2D_v1_1 new_point = Point2D_v1_1::from_current(old_point);
                        
                        // Write with new size
                        ObjectHeader new_header = obj_header;
                        new_header.data_size = sizeof(Point2D_v1_1);
                        writer.write_object_header(new_header);
                        writer.write_bytes(&new_point, sizeof(Point2D_v1_1));
                    } else {
                        // Copy other objects unchanged
                        writer.write_object_header(obj_header);
                        if (obj_header.data_size > 0) {
                            std::vector<uint8_t> data(obj_header.data_size);
                            reader.read_bytes(data.data(), obj_header.data_size);
                            writer.write_bytes(data.data(), obj_header.data_size);
                        }
                    }
                }
                
                output = writer.get_buffer();
            }
            
            result.add_message("Migration completed successfully");
            
        } catch (const std::exception& e) {
            result.add_error("Migration failed: " + std::string(e.what()));
        }
        
        return result;
    }
};

/**
 * @brief Migration from version 1.1 to 2.0
 * Changes Color from uint8 to float representation
 */
class Migration_1_1_to_2_0 {
public:
    static MigrationResult migrate(const std::vector<uint8_t>& input,
                                 std::vector<uint8_t>& output) {
        MigrationResult result;
        result.from_version = Version(1, 1);
        result.to_version = Version(2, 0);
        
        try {
            // Check if input has file header or is raw data
            bool has_header = input.size() >= sizeof(FileHeader);
            if (has_header) {
                FileHeader test_header;
                std::memcpy(&test_header, input.data(), sizeof(FileHeader));
                test_header.magic = from_file_endian(test_header.magic);
                has_header = (test_header.magic == MAGIC_NUMBER);
            }
            
            BinaryWriter writer(input.size() * 2); // Colors will be larger
            
            if (has_header) {
                BinaryReader reader(input);
                
                // Copy file header with updated version
                FileHeader header = reader.read_file_header();
                header.version_major = 2;
                header.version_minor = 0;
                writer.write_file_header(header);
                
                // Process each object
                while (!reader.eof()) {
                    ObjectHeader obj_header = reader.read_object_header();
                    
                    if (obj_header.type == TypeId::Color) {
                        // Read old format Color (uint8)
                        Color old_color;
                        reader.read_bytes(&old_color, sizeof(Color));
                        
                        // Convert to new format
                        Color_v2_0 new_color = Color_v2_0::from_uint8(old_color);
                        
                        // Write with new size
                        ObjectHeader new_header = obj_header;
                        new_header.data_size = sizeof(Color_v2_0);
                        writer.write_object_header(new_header);
                        writer.write_bytes(&new_color, sizeof(Color_v2_0));
                        
                        result.add_message("Migrated Color object " + 
                                         std::to_string(obj_header.object_id));
                    } else {
                        // Copy other objects unchanged
                        writer.write_object_header(obj_header);
                        std::vector<uint8_t> data(obj_header.data_size);
                        reader.read_bytes(data.data(), obj_header.data_size);
                        writer.write_bytes(data.data(), obj_header.data_size);
                    }
                }
                
                output = writer.get_buffer();
            } else {
                // Raw data without file header
                BinaryReader reader(input);
                
                // Process raw objects
                while (!reader.eof()) {
                    if (reader.remaining() < sizeof(ObjectHeader)) break;
                    
                    ObjectHeader obj_header = reader.read_object_header();
                    
                    if (obj_header.type == TypeId::Color) {
                        // Read old format Color (uint8)
                        Color old_color;
                        reader.read_bytes(&old_color, sizeof(Color));
                        
                        // Convert to new format
                        Color_v2_0 new_color = Color_v2_0::from_uint8(old_color);
                        
                        // Write with new size
                        ObjectHeader new_header = obj_header;
                        new_header.data_size = sizeof(Color_v2_0);
                        writer.write_object_header(new_header);
                        writer.write_bytes(&new_color, sizeof(Color_v2_0));
                    } else {
                        // Copy other objects unchanged
                        writer.write_object_header(obj_header);
                        if (obj_header.data_size > 0) {
                            std::vector<uint8_t> data(obj_header.data_size);
                            reader.read_bytes(data.data(), obj_header.data_size);
                            writer.write_bytes(data.data(), obj_header.data_size);
                        }
                    }
                }
                
                output = writer.get_buffer();
            }
            
            result.add_message("Migration completed successfully");
            result.add_warning("Color format changed from uint8 to float - "
                             "some precision loss may occur on round-trip");
            
        } catch (const std::exception& e) {
            result.add_error("Migration failed: " + std::string(e.what()));
        }
        
        return result;
    }
};

/**
 * @brief Register example migrations
 */
class ExampleMigrations {
public:
    static void register_all() {
        auto& manager = VersionMigrationManager::instance();
        
        // Register migration from 1.0 to 1.1
        manager.register_migration(
            Version(1, 0), Version(1, 1),
            Migration_1_0_to_1_1::migrate);
        
        // Register migration from 1.1 to 2.0
        manager.register_migration(
            Version(1, 1), Version(2, 0),
            Migration_1_1_to_2_0::migrate);
        
        // Register reverse migrations for testing
        manager.register_migration(
            Version(1, 1), Version(1, 0),
            [](const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
                // Reverse of 1.0 to 1.1 - remove high_precision flag
                MigrationResult result;
                result.from_version = Version(1, 1);
                result.to_version = Version(1, 0);
                
                // Implementation would strip the high_precision flag
                output = input; // Simplified for now
                result.add_warning("Downgrade migration - high_precision flag will be lost");
                
                return result;
            });
    }
};

/**
 * @brief File format documentation
 */
class FormatDocumentation {
public:
    static std::string get_changes(const Version& version) {
        if (version == Version(1, 0)) {
            return "Version 1.0 - Initial release\n"
                   "- Basic binary format\n"
                   "- Support for Point2D, Color, Transform2D, BoundingBox\n"
                   "- CRC32 checksums";
        }
        else if (version == Version(1, 1)) {
            return "Version 1.1 - High precision points\n"
                   "- Added high_precision flag to Point2D\n"
                   "- Backward compatible with 1.0 for reading";
        }
        else if (version == Version(2, 0)) {
            return "Version 2.0 - Float colors\n"
                   "- Changed Color from uint8 to float representation\n"
                   "- Breaking change - requires migration from 1.x";
        }
        
        return "Unknown version";
    }
    
    static std::string get_migration_notes(const Version& from, const Version& to) {
        std::string key = from.to_string() + " -> " + to.to_string();
        
        if (key == "1.0 -> 1.1") {
            return "Point2D objects will have high_precision flag added (default: false)";
        }
        else if (key == "1.1 -> 2.0") {
            return "Color values will be converted from 0-255 range to 0.0-1.0 range";
        }
        else if (key == "1.1 -> 1.0") {
            return "WARNING: Downgrade will lose high_precision flag from Point2D objects";
        }
        
        return "No specific migration notes";
    }
};

} // namespace serialization
} // namespace claude_draw