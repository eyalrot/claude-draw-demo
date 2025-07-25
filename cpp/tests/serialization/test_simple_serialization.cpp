#include <gtest/gtest.h>
#include "claude_draw/serialization/simple_serializers.h"

using namespace claude_draw;
using namespace claude_draw::serialization;

class SimpleSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
};

TEST_F(SimpleSerializationTest, SerializeBasicTypes) {
    BinaryWriter writer;
    
    // Write some basic types
    writer.write_uint8(42);
    writer.write_uint16(1234);
    writer.write_uint32(0x12345678);
    writer.write_float(3.14159f);
    writer.write_string("Hello, World!");
    
    // Read them back
    BinaryReader reader(writer.get_buffer());
    
    EXPECT_EQ(reader.read_uint8(), 42);
    EXPECT_EQ(reader.read_uint16(), 1234);
    EXPECT_EQ(reader.read_uint32(), 0x12345678);
    EXPECT_FLOAT_EQ(reader.read_float(), 3.14159f);
    EXPECT_EQ(reader.read_string(), "Hello, World!");
}

TEST_F(SimpleSerializationTest, SerializePoint2D) {
    BinaryWriter writer;
    Point2D point(3.14f, 2.71f);
    
    serialize(writer, point);
    
    BinaryReader reader(writer.get_buffer());
    Point2D deserialized = deserialize_point(reader);
    
    EXPECT_FLOAT_EQ(deserialized.x, 3.14f);
    EXPECT_FLOAT_EQ(deserialized.y, 2.71f);
}

TEST_F(SimpleSerializationTest, SerializeColor) {
    BinaryWriter writer;
    Color color(uint8_t(255), uint8_t(128), uint8_t(64), uint8_t(32));
    
    serialize(writer, color);
    
    BinaryReader reader(writer.get_buffer());
    Color deserialized = deserialize_color(reader);
    
    EXPECT_EQ(deserialized.r, 255);
    EXPECT_EQ(deserialized.g, 128);
    EXPECT_EQ(deserialized.b, 64);
    EXPECT_EQ(deserialized.a, 32);
}

TEST_F(SimpleSerializationTest, SerializeTransform2D) {
    BinaryWriter writer;
    Transform2D transform;
    transform.translate(10.0f, 20.0f);
    transform.rotate(45.0f);
    transform.scale(2.0f, 3.0f);
    
    serialize(writer, transform);
    
    BinaryReader reader(writer.get_buffer());
    Transform2D deserialized = deserialize_transform(reader);
    
    // Compare matrix elements with some tolerance
    EXPECT_NEAR(deserialized(0, 0), transform(0, 0), 1e-6f);
    EXPECT_NEAR(deserialized(0, 1), transform(0, 1), 1e-6f);
    EXPECT_NEAR(deserialized(0, 2), transform(0, 2), 1e-6f);
    EXPECT_NEAR(deserialized(1, 0), transform(1, 0), 1e-6f);
    EXPECT_NEAR(deserialized(1, 1), transform(1, 1), 1e-6f);
    EXPECT_NEAR(deserialized(1, 2), transform(1, 2), 1e-6f);
}

TEST_F(SimpleSerializationTest, SerializeBoundingBox) {
    BinaryWriter writer;
    BoundingBox bbox(10.0f, 20.0f, 100.0f, 200.0f);
    
    serialize(writer, bbox);
    
    BinaryReader reader(writer.get_buffer());
    BoundingBox deserialized = deserialize_bbox(reader);
    
    EXPECT_FLOAT_EQ(deserialized.min_x, 10.0f);
    EXPECT_FLOAT_EQ(deserialized.min_y, 20.0f);
    EXPECT_FLOAT_EQ(deserialized.max_x, 100.0f);
    EXPECT_FLOAT_EQ(deserialized.max_y, 200.0f);
}

TEST_F(SimpleSerializationTest, SerializeCircleData) {
    BinaryWriter writer;
    
    // Serialize circle data manually
    writer.write_uint32(123); // id
    writer.write_float(50.0f); // center x
    writer.write_float(75.0f); // center y
    writer.write_float(25.0f); // radius
    writer.write_uint32(0xFF0000FF); // fill color
    writer.write_uint32(0x0000FFFF); // stroke color
    writer.write_float(2.5f); // stroke width
    
    // Read back
    BinaryReader reader(writer.get_buffer());
    
    uint32_t id = reader.read_uint32();
    float cx = reader.read_float();
    float cy = reader.read_float();
    float radius = reader.read_float();
    uint32_t fill = reader.read_uint32();
    uint32_t stroke = reader.read_uint32();
    float stroke_width = reader.read_float();
    
    EXPECT_EQ(id, 123);
    EXPECT_FLOAT_EQ(cx, 50.0f);
    EXPECT_FLOAT_EQ(cy, 75.0f);
    EXPECT_FLOAT_EQ(radius, 25.0f);
    EXPECT_EQ(fill, 0xFF0000FF);
    EXPECT_EQ(stroke, 0x0000FFFF);
    EXPECT_FLOAT_EQ(stroke_width, 2.5f);
}

TEST_F(SimpleSerializationTest, FileHeaderRoundTrip) {
    BinaryWriter writer;
    
    FileHeader header;
    header.object_count = 42;
    header.total_size = 1024;
    header.compression = CompressionType::LZ4;
    
    writer.write_file_header(header);
    
    BinaryReader reader(writer.get_buffer());
    FileHeader read_header = reader.read_file_header();
    
    EXPECT_EQ(read_header.magic, MAGIC_NUMBER);
    EXPECT_EQ(read_header.version_major, CURRENT_VERSION_MAJOR);
    EXPECT_EQ(read_header.version_minor, CURRENT_VERSION_MINOR);
    EXPECT_EQ(read_header.object_count, 42);
    EXPECT_EQ(read_header.total_size, 1024);
    EXPECT_EQ(read_header.compression, CompressionType::LZ4);
}

TEST_F(SimpleSerializationTest, WriteToFile) {
    std::string test_file = "test_simple_binary.bin";
    
    {
        BinaryWriter writer;
        
        // Write some test data
        writer.write_uint32(0xDEADBEEF);
        writer.write_string("Test data");
        writer.write_float(3.14159f);
        
        writer.write_to_file(test_file);
    }
    
    {
        // Read back from file
        auto buffer = BinaryReader::read_file(test_file);
        BinaryReader reader(buffer);
        
        EXPECT_EQ(reader.read_uint32(), 0xDEADBEEF);
        EXPECT_EQ(reader.read_string(), "Test data");
        EXPECT_FLOAT_EQ(reader.read_float(), 3.14159f);
    }
    
    // Clean up
    std::remove(test_file.c_str());
}