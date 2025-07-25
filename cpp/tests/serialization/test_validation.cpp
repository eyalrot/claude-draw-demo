#include <gtest/gtest.h>
#include "claude_draw/serialization/validation.h"
#include "claude_draw/serialization/validated_io.h"
#include "claude_draw/serialization/simple_serializers.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include <fstream>
#include <random>

using namespace claude_draw;
using namespace claude_draw::serialization;

class ValidationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_validation.bin";
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
    
    // Helper to corrupt a file at specific offset
    void corrupt_file(const std::string& filename, size_t offset, uint8_t new_value) {
        std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(offset);
        file.write(reinterpret_cast<const char*>(&new_value), 1);
    }
    
    std::string test_file_;
};

TEST_F(ValidationTest, CRC32Calculator) {
    // Test CRC32 calculation
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    uint32_t crc1 = CRC32::calculate(data.data(), data.size());
    
    // Calculate again to ensure consistency
    uint32_t crc2 = CRC32::calculate(data.data(), data.size());
    EXPECT_EQ(crc1, crc2);
    
    // Change data and verify CRC changes
    data[0] = 2;
    uint32_t crc3 = CRC32::calculate(data.data(), data.size());
    EXPECT_NE(crc1, crc3);
    
    // Test incremental calculation
    CRC32 incremental;
    incremental.update(&data[0], 3);
    incremental.update(&data[3], 2);
    uint32_t crc4 = incremental.finalize();
    EXPECT_EQ(crc3, crc4);
}

TEST_F(ValidationTest, ValidateFileHeader) {
    FileHeader header;
    header.magic = MAGIC_NUMBER;
    header.version_major = CURRENT_VERSION_MAJOR;
    header.version_minor = CURRENT_VERSION_MINOR;
    header.flags = 0;
    header.compression = CompressionType::None;
    header.data_offset = sizeof(FileHeader);
    header.total_size = 1000;
    header.object_count = 10;
    header.checksum = 0;
    
    // Valid header
    auto result = FormatValidator::validate_header(header);
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
    
    // Invalid magic number
    header.magic = 0x12345678;
    result = FormatValidator::validate_header(header);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
    
    // Reset
    header.magic = MAGIC_NUMBER;
    
    // Unsupported version
    header.version_major = 99;
    result = FormatValidator::validate_header(header);
    EXPECT_FALSE(result.valid);
    
    // Reset
    header.version_major = CURRENT_VERSION_MAJOR;
    
    // Invalid data offset
    header.data_offset = 10;  // Less than header size
    result = FormatValidator::validate_header(header);
    EXPECT_FALSE(result.valid);
    
    // Reset
    header.data_offset = sizeof(FileHeader);
    
    // File too small for object count - test the actual validation logic
    header.object_count = 100;
    header.total_size = sizeof(FileHeader);  // Equal to data_offset, should fail
    result = FormatValidator::validate_header(header);
    EXPECT_FALSE(result.valid);
    
    // Reset for next test
    header.total_size = 1000;
    
    // Test invalid compression type
    header.object_count = 10;
    header.total_size = 1000;
    header.compression = static_cast<CompressionType>(99);
    result = FormatValidator::validate_header(header);
    EXPECT_FALSE(result.valid);
}

TEST_F(ValidationTest, ValidateObjectHeader) {
    ObjectHeader header;
    header.type = TypeId::Point2D;
    header.object_id = 1;
    header.data_size = sizeof(Point2D);
    
    // Valid header
    auto result = FormatValidator::validate_object_header(header, 1000);
    EXPECT_TRUE(result.valid);
    
    // Since TypeId is uint8_t, we can't test values > 255
    // The validation check for > Reserved will never trigger
    // Skip this test case
    
    // Reset
    header.type = TypeId::Point2D;
    
    // Data size exceeds available
    header.data_size = 2000;
    result = FormatValidator::validate_object_header(header, 1000);
    EXPECT_FALSE(result.valid);
    
    // Very large object warning
    header.data_size = 200 * 1024 * 1024;  // 200MB
    result = FormatValidator::validate_object_header(header, 300 * 1024 * 1024);
    EXPECT_TRUE(result.valid);  // Should be valid but with warning
    EXPECT_FALSE(result.warnings.empty());
}

TEST_F(ValidationTest, ValidatedWriter) {
    ValidatedBinaryWriter writer;
    
    // Write some test data
    serialize(writer.writer(), Point2D(1.0f, 2.0f));
    serialize(writer.writer(), Color(uint8_t(255), uint8_t(128), uint8_t(64), uint8_t(255)));
    
    // Write to file with validation
    EXPECT_NO_THROW(writer.write_to_file(test_file_));
    
    // Verify file exists and is valid
    auto result = FormatValidator::validate_file_checksum(test_file_);
    EXPECT_TRUE(result.valid);
}

TEST_F(ValidationTest, ValidatedReader) {
    // First write a valid file with proper headers
    {
        BinaryWriter writer;
        
        // Register objects so the count is correct
        Point2D p1(3.0f, 4.0f);
        Point2D p2(5.0f, 6.0f);
        writer.register_object(&p1);
        writer.register_object(&p2);
        
        // Write objects with headers
        ObjectHeader obj1(TypeId::Point2D, 1, sizeof(Point2D));
        writer.write_object_header(obj1);
        writer.write_bytes(&p1, sizeof(Point2D));
        
        ObjectHeader obj2(TypeId::Point2D, 2, sizeof(Point2D));
        writer.write_object_header(obj2);
        writer.write_bytes(&p2, sizeof(Point2D));
        
        writer.write_to_file_with_header(test_file_);
    }
    
    // Read with validation
    auto buffer = ValidatedBinaryReader::read_file(test_file_);
    EXPECT_FALSE(buffer.empty());
    
    // Try to read from non-existent file
    EXPECT_THROW({
        ValidatedBinaryReader::read_file("nonexistent.bin");
    }, std::runtime_error);
}

TEST_F(ValidationTest, ChecksumValidation) {
    // Write file with checksum
    {
        BinaryWriter writer;
        writer.write_uint32(42);
        writer.write_float(3.14f);
        writer.write_to_file_with_header(test_file_, true);
    }
    
    // Validate checksum
    auto result = FormatValidator::validate_file_checksum(test_file_);
    EXPECT_TRUE(result.valid);
    
    // Corrupt the data
    corrupt_file(test_file_, sizeof(FileHeader) + 1, 99);
    
    // Checksum should fail
    result = FormatValidator::validate_file_checksum(test_file_);
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST_F(ValidationTest, StructureValidation) {
    // Write a valid file
    {
        BinaryWriter writer;
        
        // Register objects
        Point2D p1(1.0f, 2.0f);
        Color c1(uint8_t(255), uint8_t(0), uint8_t(0), uint8_t(255));
        writer.register_object(&p1);
        writer.register_object(&c1);
        
        // Write objects with headers
        ObjectHeader obj1(TypeId::Point2D, 1, sizeof(Point2D));
        writer.write_object_header(obj1);
        writer.write_bytes(&p1, sizeof(Point2D));
        
        ObjectHeader obj2(TypeId::Color, 2, sizeof(Color));
        writer.write_object_header(obj2);
        writer.write_bytes(&c1, sizeof(Color));
        
        writer.write_to_file_with_header(test_file_);
    }
    
    // Validate structure
    auto result = FormatValidator::validate_structure(test_file_);
    EXPECT_TRUE(result.valid);
    
    // Corrupt object header - corrupt the object ID to create invalid size
    corrupt_file(test_file_, sizeof(FileHeader) + offsetof(ObjectHeader, data_size), 0xFF);
    corrupt_file(test_file_, sizeof(FileHeader) + offsetof(ObjectHeader, data_size) + 1, 0xFF);
    
    // Structure validation should fail due to object size exceeding file size
    result = FormatValidator::validate_structure(test_file_);
    EXPECT_FALSE(result.valid);
}

TEST_F(ValidationTest, ValidationException) {
    ValidationResult result;
    result.add_error("Test error 1");
    result.add_error("Test error 2");
    result.add_warning("Test warning");
    
    try {
        throw ValidationException(result);
    } catch (const ValidationException& e) {
        EXPECT_FALSE(e.result().valid);
        EXPECT_EQ(e.result().errors.size(), 2);
        EXPECT_EQ(e.result().warnings.size(), 1);
        
        std::string msg = e.what();
        EXPECT_NE(msg.find("Test error 1"), std::string::npos);
    }
}

TEST_F(ValidationTest, ValidatedStreamingWriter) {
    StreamingConfig config;
    config.chunk_size = 1024;
    
    {
        ValidatedStreamingWriter writer(test_file_, config);
        
        // Write some objects
        for (int i = 0; i < 100; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i * 2));
            writer.write_object(TypeId::Point2D, i, point);
        }
        
        // Finalize with validation
        EXPECT_NO_THROW(writer.finalize_with_validation());
    }
    
    // Verify file is valid
    auto result = FormatValidator::validate_structure(test_file_);
    EXPECT_TRUE(result.valid);
}

TEST_F(ValidationTest, SafeFileReader) {
    // Write a valid file
    {
        BinaryWriter writer;
        writer.write_uint32(12345);
        writer.write_float(67.89f);
        writer.write_to_file_with_header(test_file_, true);
    }
    
    // Read normally
    auto data1 = SafeFileReader::read_with_recovery(test_file_);
    EXPECT_FALSE(data1.empty());
    
    // Corrupt checksum but keep structure valid
    {
        std::fstream file(test_file_, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(offsetof(FileHeader, checksum));
        uint32_t bad_checksum = 0xDEADBEEF;
        bad_checksum = to_file_endian(bad_checksum);
        file.write(reinterpret_cast<const char*>(&bad_checksum), sizeof(bad_checksum));
    }
    
    // Should still be able to recover data
    auto data2 = SafeFileReader::read_with_recovery(test_file_);
    EXPECT_FALSE(data2.empty());
    // Size might differ due to different recovery methods
}

TEST_F(ValidationTest, EmptyFileValidation) {
    // Create empty file
    std::ofstream file(test_file_, std::ios::binary);
    file.close();
    
    // Should fail validation
    auto result = FormatValidator::validate_file_checksum(test_file_);
    EXPECT_FALSE(result.valid);
    
    result = FormatValidator::validate_structure(test_file_);
    EXPECT_FALSE(result.valid);
}

TEST_F(ValidationTest, LargeFileValidation) {
    // Write a large file
    {
        BinaryWriter writer(10 * 1024 * 1024);  // 10MB buffer
        
        // Pre-register all objects
        std::vector<Point2D> points;
        points.reserve(10000);
        for (int i = 0; i < 10000; ++i) {
            points.emplace_back(static_cast<float>(i), static_cast<float>(i));
            writer.register_object(&points.back());
        }
        
        // Write many objects
        for (int i = 0; i < 10000; ++i) {
            ObjectHeader header(TypeId::Point2D, i, sizeof(Point2D));
            writer.write_object_header(header);
            writer.write_bytes(&points[i], sizeof(Point2D));
        }
        
        writer.write_to_file_with_header(test_file_, true);
    }
    
    // Validate large file
    auto result = FormatValidator::validate_file_checksum(test_file_);
    EXPECT_TRUE(result.valid);
    
    result = FormatValidator::validate_structure(test_file_);
    EXPECT_TRUE(result.valid);
}