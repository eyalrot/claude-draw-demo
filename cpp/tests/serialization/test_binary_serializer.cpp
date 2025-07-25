#include <gtest/gtest.h>
#include <cstring>
#include <fstream>
#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"

using namespace claude_draw::serialization;

class BinarySerializerTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_binary_output.bin";
    }
    
    void TearDown() override {
        // Clean up test files
        std::remove(test_file_.c_str());
    }
    
    std::string test_file_;
};

TEST_F(BinarySerializerTest, WriterConstruction) {
    BinaryWriter writer;
    EXPECT_EQ(writer.size(), 0);
    
    BinaryWriter writer2(4096);
    EXPECT_EQ(writer2.size(), 0);  // Reserved but not used
}

TEST_F(BinarySerializerTest, WritePrimitives) {
    BinaryWriter writer;
    
    writer.write_uint8(0x12);
    writer.write_uint16(0x3456);
    writer.write_uint32(0x789ABCDE);
    writer.write_uint64(0x123456789ABCDEF0ULL);
    writer.write_float(3.14159f);
    writer.write_double(2.71828);
    
    const auto& buffer = writer.get_buffer();
    EXPECT_EQ(buffer.size(), 1 + 2 + 4 + 8 + 4 + 8);
    
    // Verify data with reader
    BinaryReader reader(buffer);
    EXPECT_EQ(reader.read_uint8(), 0x12);
    EXPECT_EQ(reader.read_uint16(), 0x3456);
    EXPECT_EQ(reader.read_uint32(), 0x789ABCDE);
    EXPECT_EQ(reader.read_uint64(), 0x123456789ABCDEF0ULL);
    EXPECT_FLOAT_EQ(reader.read_float(), 3.14159f);
    EXPECT_DOUBLE_EQ(reader.read_double(), 2.71828);
}

TEST_F(BinarySerializerTest, WriteString) {
    BinaryWriter writer;
    
    writer.write_string("Hello, World!");
    writer.write_string("");  // Empty string
    writer.write_string("Unicode: αβγ");
    
    BinaryReader reader(writer.get_buffer());
    EXPECT_EQ(reader.read_string(), "Hello, World!");
    EXPECT_EQ(reader.read_string(), "");
    EXPECT_EQ(reader.read_string(), "Unicode: αβγ");
}

TEST_F(BinarySerializerTest, WriteBytes) {
    BinaryWriter writer;
    
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    writer.write_bytes(data, sizeof(data));
    
    const auto& buffer = writer.get_buffer();
    EXPECT_EQ(buffer.size(), 8);
    EXPECT_EQ(std::memcmp(buffer.data(), data, 8), 0);
}

TEST_F(BinarySerializerTest, FileHeader) {
    BinaryWriter writer;
    
    FileHeader header;
    header.object_count = 42;
    header.total_size = 1024;
    
    writer.write_file_header(header);
    
    BinaryReader reader(writer.get_buffer());
    FileHeader read_header = reader.read_file_header();
    
    EXPECT_EQ(read_header.magic, MAGIC_NUMBER);
    EXPECT_EQ(read_header.version_major, CURRENT_VERSION_MAJOR);
    EXPECT_EQ(read_header.version_minor, CURRENT_VERSION_MINOR);
    EXPECT_EQ(read_header.object_count, 42);
    EXPECT_EQ(read_header.total_size, 1024);
}

TEST_F(BinarySerializerTest, ObjectHeader) {
    BinaryWriter writer;
    
    ObjectHeader header(TypeId::Circle, 123, 456);
    writer.write_object_header(header);
    
    BinaryReader reader(writer.get_buffer());
    ObjectHeader read_header = reader.read_object_header();
    
    EXPECT_EQ(read_header.type, TypeId::Circle);
    EXPECT_EQ(read_header.object_id, 123);
    EXPECT_EQ(read_header.data_size, 456);
}

TEST_F(BinarySerializerTest, ArrayHeader) {
    BinaryWriter writer;
    
    ArrayHeader header(TypeId::Point2D, 100);
    writer.write_array_header(header);
    
    BinaryReader reader(writer.get_buffer());
    ArrayHeader read_header = reader.read_array_header();
    
    EXPECT_EQ(read_header.element_type, TypeId::Point2D);
    EXPECT_EQ(read_header.count, 100);
}

TEST_F(BinarySerializerTest, ObjectRegistration) {
    BinaryWriter writer;
    
    int obj1 = 42;
    int obj2 = 99;
    
    EXPECT_FALSE(writer.is_registered(&obj1));
    
    uint32_t id1 = writer.register_object(&obj1);
    EXPECT_TRUE(writer.is_registered(&obj1));
    EXPECT_EQ(writer.get_object_id(&obj1), id1);
    
    uint32_t id2 = writer.register_object(&obj2);
    EXPECT_NE(id1, id2);
    
    // Re-registering returns same ID
    uint32_t id1_again = writer.register_object(&obj1);
    EXPECT_EQ(id1, id1_again);
}

TEST_F(BinarySerializerTest, Alignment) {
    BinaryWriter writer;
    
    writer.write_uint8(1);
    EXPECT_EQ(writer.size(), 1);
    
    writer.align_to(8);
    EXPECT_EQ(writer.size(), 8);
    
    writer.write_uint16(2);
    EXPECT_EQ(writer.size(), 10);
    
    writer.align_to(16);
    EXPECT_EQ(writer.size(), 16);
}

TEST_F(BinarySerializerTest, WriteToFile) {
    BinaryWriter writer;
    
    writer.write_uint32(0x12345678);
    writer.write_string("Test data");
    
    writer.write_to_file(test_file_);
    
    // Verify file exists and has correct size
    std::ifstream file(test_file_, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(file.is_open());
    size_t file_size = file.tellg();
    EXPECT_EQ(file_size, writer.size());
    
    // Read back and verify
    auto buffer = BinaryReader::read_file(test_file_);
    BinaryReader reader(buffer);
    
    EXPECT_EQ(reader.read_uint32(), 0x12345678);
    EXPECT_EQ(reader.read_string(), "Test data");
}

TEST_F(BinarySerializerTest, ClearWriter) {
    BinaryWriter writer;
    
    writer.write_uint32(42);
    writer.register_object((void*)0x1234);
    
    EXPECT_GT(writer.size(), 0);
    EXPECT_TRUE(writer.is_registered((void*)0x1234));
    
    writer.clear();
    
    EXPECT_EQ(writer.size(), 0);
    EXPECT_FALSE(writer.is_registered((void*)0x1234));
}

TEST_F(BinarySerializerTest, ReaderNavigation) {
    BinaryWriter writer;
    writer.write_uint32(1);
    writer.write_uint32(2);
    writer.write_uint32(3);
    
    BinaryReader reader(writer.get_buffer());
    
    EXPECT_EQ(reader.tell(), 0);
    EXPECT_EQ(reader.remaining(), 12);
    EXPECT_FALSE(reader.eof());
    
    reader.read_uint32();
    EXPECT_EQ(reader.tell(), 4);
    EXPECT_EQ(reader.remaining(), 8);
    
    reader.skip(4);
    EXPECT_EQ(reader.tell(), 8);
    
    reader.seek(0);
    EXPECT_EQ(reader.tell(), 0);
    EXPECT_EQ(reader.read_uint32(), 1);
}

TEST_F(BinarySerializerTest, ReaderUnderflow) {
    BinaryWriter writer;
    writer.write_uint32(42);
    
    BinaryReader reader(writer.get_buffer());
    reader.read_uint32();  // OK
    
    // Try to read past end
    EXPECT_THROW(reader.read_uint32(), std::runtime_error);
}

TEST_F(BinarySerializerTest, ZeroCopyAccess) {
    BinaryWriter writer;
    
    struct TestStruct {
        uint32_t a;
        float b;
        uint64_t c;
    };
    
    TestStruct data = {42, 3.14f, 0x123456789ABCDEF0ULL};
    writer.write_bytes(&data, sizeof(data));
    
    BinaryReader reader(writer.get_buffer());
    const TestStruct* ptr = reader.get_object_ptr<TestStruct>();
    
    EXPECT_EQ(ptr->a, 42);
    EXPECT_FLOAT_EQ(ptr->b, 3.14f);
    EXPECT_EQ(ptr->c, 0x123456789ABCDEF0ULL);
}

TEST_F(BinarySerializerTest, ReaderObjectIndex) {
    BinaryReader reader;
    
    int obj1 = 42;
    int obj2 = 99;
    
    reader.register_object(1, &obj1);
    reader.register_object(2, &obj2);
    
    EXPECT_EQ(reader.get_object(1), &obj1);
    EXPECT_EQ(reader.get_object(2), &obj2);
    EXPECT_EQ(reader.get_object(3), nullptr);
    
    EXPECT_EQ(reader.get_object_typed<int>(1), &obj1);
    EXPECT_EQ(reader.get_object_typed<int>(2), &obj2);
}

TEST_F(BinarySerializerTest, StreamingWriter) {
    // Create streaming writer
    std::string stream_file = "test_streaming.bin";
    
    {
        StreamingBinaryWriter stream_writer(stream_file);
        
        // Write some test objects (would need template specializations in real code)
        // For now, just test that it compiles and creates file
    }
    
    // Verify file was created with header
    std::ifstream file(stream_file, std::ios::binary);
    ASSERT_TRUE(file.is_open());
    
    FileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    EXPECT_TRUE(header.validate());
    
    // Clean up
    std::remove(stream_file.c_str());
}

TEST_F(BinarySerializerTest, ArenaAllocation) {
    BinaryWriter writer(1024);
    
    // Test arena allocation
    void* ptr1 = writer.allocate(64);
    void* ptr2 = writer.allocate(128);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr1, ptr2);
    
    // Verify alignment
    uintptr_t addr1 = reinterpret_cast<uintptr_t>(ptr1);
    uintptr_t addr2 = reinterpret_cast<uintptr_t>(ptr2);
    EXPECT_EQ(addr1 % 8, 0);  // 8-byte aligned
    EXPECT_EQ(addr2 % 8, 0);
}