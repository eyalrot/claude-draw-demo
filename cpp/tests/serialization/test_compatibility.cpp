#include <gtest/gtest.h>
#include "claude_draw/serialization/version_migration.h"
#include "claude_draw/serialization/format_changes.h"
#include "claude_draw/serialization/simple_serializers.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include <fstream>

using namespace claude_draw;
using namespace claude_draw::serialization;

class CompatibilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_compatibility.bin";
        
        // Register migrations
        ExampleMigrations::register_all();
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
    
    // Helper to create a file with specific version
    void create_versioned_file(uint16_t major, uint16_t minor,
                             const std::vector<uint8_t>& data) {
        FileHeader header;
        header.magic = MAGIC_NUMBER;
        header.version_major = major;
        header.version_minor = minor;
        header.flags = 0;
        header.compression = CompressionType::None;
        header.data_offset = sizeof(FileHeader);
        header.total_size = sizeof(FileHeader) + data.size();
        header.object_count = 0; // Will be set based on actual objects
        header.checksum = 0;
        
        std::ofstream file(test_file_, std::ios::binary);
        
        // Write header with endianness conversion
        FileHeader file_header = header;
        file_header.magic = to_file_endian(file_header.magic);
        file_header.version_major = to_file_endian(file_header.version_major);
        file_header.version_minor = to_file_endian(file_header.version_minor);
        file_header.flags = to_file_endian(file_header.flags);
        file_header.total_size = to_file_endian(file_header.total_size);
        file_header.data_offset = to_file_endian(file_header.data_offset);
        file_header.object_count = to_file_endian(file_header.object_count);
        file_header.checksum = to_file_endian(file_header.checksum);
        
        file.write(reinterpret_cast<const char*>(&file_header), sizeof(FileHeader));
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
    }
    
    std::string test_file_;
};

TEST_F(CompatibilityTest, VersionComparison) {
    Version v1_0(1, 0);
    Version v1_1(1, 1);
    Version v2_0(2, 0);
    
    EXPECT_TRUE(v1_0 < v1_1);
    EXPECT_TRUE(v1_1 < v2_0);
    EXPECT_FALSE(v2_0 < v1_1);
    
    EXPECT_TRUE(v1_0 == v1_0);
    EXPECT_FALSE(v1_0 == v1_1);
    
    EXPECT_TRUE(v1_0 <= v1_1);
    EXPECT_TRUE(v1_0 <= v1_0);
}

TEST_F(CompatibilityTest, MigrationPathFinding) {
    auto& manager = VersionMigrationManager::instance();
    
    // Test direct path
    auto path = manager.find_migration_path(Version(1, 0), Version(1, 1));
    ASSERT_EQ(path.size(), 2);
    EXPECT_EQ(path[0], Version(1, 0));
    EXPECT_EQ(path[1], Version(1, 1));
    
    // Test multi-step path
    path = manager.find_migration_path(Version(1, 0), Version(2, 0));
    ASSERT_EQ(path.size(), 3);
    EXPECT_EQ(path[0], Version(1, 0));
    EXPECT_EQ(path[1], Version(1, 1));
    EXPECT_EQ(path[2], Version(2, 0));
    
    // Test same version
    path = manager.find_migration_path(Version(1, 0), Version(1, 0));
    ASSERT_EQ(path.size(), 1);
    EXPECT_EQ(path[0], Version(1, 0));
}

TEST_F(CompatibilityTest, SimpleMigration_1_0_to_1_1) {
    // Create v1.0 data
    BinaryWriter writer;
    
    // Write a Point2D in v1.0 format
    ObjectHeader header(TypeId::Point2D, 1, sizeof(Point2D));
    writer.write_object_header(header);
    Point2D point(10.0f, 20.0f);
    writer.write_bytes(&point, sizeof(Point2D));
    
    std::vector<uint8_t> v1_0_data = writer.get_buffer();
    
    // Migrate to v1.1
    std::vector<uint8_t> v1_1_data;
    auto result = Migration_1_0_to_1_1::migrate(v1_0_data, v1_1_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.messages.empty());
    
    // Verify the migrated data (raw data without file header)
    BinaryReader reader(v1_1_data);
    
    ObjectHeader obj_header = reader.read_object_header();
    EXPECT_EQ(obj_header.type, TypeId::Point2D);
    EXPECT_EQ(obj_header.data_size, sizeof(Point2D_v1_1));
    
    Point2D_v1_1 migrated_point;
    reader.read_bytes(&migrated_point, sizeof(Point2D_v1_1));
    EXPECT_FLOAT_EQ(migrated_point.x, 10.0f);
    EXPECT_FLOAT_EQ(migrated_point.y, 20.0f);
    EXPECT_FALSE(migrated_point.high_precision);
}

TEST_F(CompatibilityTest, SimpleMigration_1_1_to_2_0) {
    // Create v1.1 data with Color
    BinaryWriter writer;
    
    // Write a Color in v1.1 format (uint8)
    ObjectHeader header(TypeId::Color, 1, sizeof(Color));
    writer.write_object_header(header);
    Color color(uint8_t(255), uint8_t(128), uint8_t(64), uint8_t(255));
    writer.write_bytes(&color, sizeof(Color));
    
    std::vector<uint8_t> v1_1_data = writer.get_buffer();
    
    // Create file header for v1.1
    FileHeader file_header;
    file_header.version_major = 1;
    file_header.version_minor = 1;
    
    // Migrate to v2.0
    std::vector<uint8_t> v2_0_data;
    auto result = Migration_1_1_to_2_0::migrate(v1_1_data, v2_0_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.messages.empty());
    EXPECT_FALSE(result.warnings.empty()); // Should warn about format change
    
    // Verify the migrated data (raw data without file header)
    BinaryReader reader(v2_0_data);
    
    ObjectHeader obj_header = reader.read_object_header();
    EXPECT_EQ(obj_header.type, TypeId::Color);
    EXPECT_EQ(obj_header.data_size, sizeof(Color_v2_0));
    
    Color_v2_0 migrated_color;
    reader.read_bytes(&migrated_color, sizeof(Color_v2_0));
    EXPECT_FLOAT_EQ(migrated_color.r, 1.0f);
    EXPECT_FLOAT_EQ(migrated_color.g, 128.0f / 255.0f);
    EXPECT_FLOAT_EQ(migrated_color.b, 64.0f / 255.0f);
    EXPECT_FLOAT_EQ(migrated_color.a, 1.0f);
}

TEST_F(CompatibilityTest, ChainedMigration) {
    auto& manager = VersionMigrationManager::instance();
    
    // Create v1.0 data
    BinaryWriter writer;
    
    // Mix of objects
    ObjectHeader p_header(TypeId::Point2D, 1, sizeof(Point2D));
    writer.write_object_header(p_header);
    Point2D point(5.0f, 10.0f);
    writer.write_bytes(&point, sizeof(Point2D));
    
    ObjectHeader c_header(TypeId::Color, 2, sizeof(Color));
    writer.write_object_header(c_header);
    Color color(uint8_t(200), uint8_t(100), uint8_t(50), uint8_t(255));
    writer.write_bytes(&color, sizeof(Color));
    
    std::vector<uint8_t> v1_0_data = writer.get_buffer();
    
    // Migrate from 1.0 to 2.0 (should go through 1.1)
    std::vector<uint8_t> v2_0_data;
    auto result = manager.migrate(Version(1, 0), Version(2, 0), v1_0_data, v2_0_data);
    
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.messages.size(), 2); // At least two migration steps
}

TEST_F(CompatibilityTest, BackwardCompatibleReader) {
    // Create a v1.0 file
    BinaryWriter writer;
    writer.register_object(&test_file_); // Dummy registration
    
    ObjectHeader header(TypeId::Point2D, 1, sizeof(Point2D));
    writer.write_object_header(header);
    Point2D point(15.0f, 25.0f);
    writer.write_bytes(&point, sizeof(Point2D));
    
    // Manually create file with v1.0 header
    create_versioned_file(1, 0, writer.get_buffer());
    
    // Read with backward compatible reader
    MigrationResult migration_result;
    auto data = BackwardCompatibleReader::read_file(test_file_, &migration_result);
    
    // Current version is 1.0, so no migration needed
    EXPECT_TRUE(migration_result.success);
    EXPECT_FALSE(data.empty());
}

TEST_F(CompatibilityTest, NoMigrationPath) {
    auto& manager = VersionMigrationManager::instance();
    
    // Try to migrate to a version with no path
    std::vector<uint8_t> dummy_data = {1, 2, 3, 4};
    std::vector<uint8_t> output;
    
    auto result = manager.migrate(Version(1, 0), Version(3, 0), dummy_data, output);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.messages.empty());
}

TEST_F(CompatibilityTest, FormatDocumentation) {
    std::string doc = FormatDocumentation::get_changes(Version(1, 0));
    EXPECT_NE(doc.find("Initial release"), std::string::npos);
    
    doc = FormatDocumentation::get_changes(Version(1, 1));
    EXPECT_NE(doc.find("High precision"), std::string::npos);
    
    doc = FormatDocumentation::get_changes(Version(2, 0));
    EXPECT_NE(doc.find("Float colors"), std::string::npos);
    
    std::string notes = FormatDocumentation::get_migration_notes(
        Version(1, 0), Version(1, 1));
    EXPECT_NE(notes.find("high_precision"), std::string::npos);
}

TEST_F(CompatibilityTest, MigrationResult) {
    MigrationResult result;
    EXPECT_TRUE(result.success);
    
    result.add_message("Test message");
    result.add_warning("Test warning");
    EXPECT_EQ(result.messages.size(), 1);
    EXPECT_EQ(result.warnings.size(), 1);
    
    result.add_error("Test error");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.messages.size(), 2); // Error is added as message
}

TEST_F(CompatibilityTest, CurrentFormatHandler) {
    CurrentFormatHandler handler;
    
    EXPECT_EQ(handler.get_version().major, CURRENT_VERSION_MAJOR);
    EXPECT_EQ(handler.get_version().minor, CURRENT_VERSION_MINOR);
    
    // Can read from same major version
    EXPECT_TRUE(handler.can_read_from(Version(CURRENT_VERSION_MAJOR, 0)));
    EXPECT_TRUE(handler.can_read_from(Version(CURRENT_VERSION_MAJOR, CURRENT_VERSION_MINOR)));
    
    // Cannot read from different major version
    EXPECT_FALSE(handler.can_read_from(Version(CURRENT_VERSION_MAJOR + 1, 0)));
}

TEST_F(CompatibilityTest, RealWorldScenario) {
    // Simulate a real scenario where we have mixed objects
    BinaryWriter writer;
    
    // Register objects for proper count
    std::vector<Point2D> points = {
        Point2D(1.0f, 2.0f),
        Point2D(3.0f, 4.0f),
        Point2D(5.0f, 6.0f)
    };
    
    std::vector<Color> colors = {
        Color(uint8_t(255), uint8_t(0), uint8_t(0), uint8_t(255)),
        Color(uint8_t(0), uint8_t(255), uint8_t(0), uint8_t(255))
    };
    
    for (auto& p : points) writer.register_object(&p);
    for (auto& c : colors) writer.register_object(&c);
    
    // Write objects
    for (size_t i = 0; i < points.size(); ++i) {
        ObjectHeader header(TypeId::Point2D, i, sizeof(Point2D));
        writer.write_object_header(header);
        writer.write_bytes(&points[i], sizeof(Point2D));
    }
    
    for (size_t i = 0; i < colors.size(); ++i) {
        ObjectHeader header(TypeId::Color, i + points.size(), sizeof(Color));
        writer.write_object_header(header);
        writer.write_bytes(&colors[i], sizeof(Color));
    }
    
    // Save as v1.0 file
    create_versioned_file(1, 0, writer.get_buffer());
    
    // Future version would read and migrate automatically
    // For now, just verify file was created
    std::ifstream file(test_file_, std::ios::binary);
    EXPECT_TRUE(file.is_open());
}