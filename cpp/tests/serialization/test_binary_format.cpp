#include <gtest/gtest.h>
#include "claude_draw/serialization/binary_format.h"

using namespace claude_draw::serialization;

class BinaryFormatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup if needed
    }
};

TEST_F(BinaryFormatTest, FileHeaderSize) {
    EXPECT_EQ(sizeof(FileHeader), 96);
}

TEST_F(BinaryFormatTest, ObjectHeaderSize) {
    EXPECT_EQ(sizeof(ObjectHeader), 12);
}

TEST_F(BinaryFormatTest, ArrayHeaderSize) {
    EXPECT_EQ(sizeof(ArrayHeader), 8);
}

TEST_F(BinaryFormatTest, FileHeaderConstruction) {
    FileHeader header;
    
    EXPECT_EQ(header.magic, MAGIC_NUMBER);
    EXPECT_EQ(header.version_major, CURRENT_VERSION_MAJOR);
    EXPECT_EQ(header.version_minor, CURRENT_VERSION_MINOR);
    EXPECT_EQ(header.data_offset, sizeof(FileHeader));
    EXPECT_TRUE(header.validate());
}

TEST_F(BinaryFormatTest, FileHeaderValidation) {
    FileHeader header;
    EXPECT_TRUE(header.validate());
    
    // Invalid magic number
    header.magic = 0x12345678;
    EXPECT_FALSE(header.validate());
    
    // Reset magic, invalid version
    header.magic = MAGIC_NUMBER;
    header.version_major = 999;
    EXPECT_FALSE(header.validate());
}

TEST_F(BinaryFormatTest, ObjectHeaderConstruction) {
    ObjectHeader header1;
    EXPECT_EQ(header1.type, TypeId::None);
    EXPECT_EQ(header1.flags, 0);
    EXPECT_EQ(header1.object_id, 0);
    EXPECT_EQ(header1.data_size, 0);
    
    ObjectHeader header2(TypeId::Circle, 42, 128);
    EXPECT_EQ(header2.type, TypeId::Circle);
    EXPECT_EQ(header2.object_id, 42);
    EXPECT_EQ(header2.data_size, 128);
}

TEST_F(BinaryFormatTest, ArrayHeaderConstruction) {
    ArrayHeader header1;
    EXPECT_EQ(header1.element_type, TypeId::None);
    EXPECT_EQ(header1.count, 0);
    
    ArrayHeader header2(TypeId::Point2D, 100);
    EXPECT_EQ(header2.element_type, TypeId::Point2D);
    EXPECT_EQ(header2.count, 100);
}

TEST_F(BinaryFormatTest, TypeIdValues) {
    // Verify type IDs are distinct
    EXPECT_NE(static_cast<int>(TypeId::None), static_cast<int>(TypeId::Point2D));
    EXPECT_NE(static_cast<int>(TypeId::Circle), static_cast<int>(TypeId::Rectangle));
    EXPECT_NE(static_cast<int>(TypeId::Group), static_cast<int>(TypeId::Layer));
    
    // Verify core types
    EXPECT_EQ(static_cast<int>(TypeId::Point2D), 1);
    EXPECT_EQ(static_cast<int>(TypeId::Color), 2);
    EXPECT_EQ(static_cast<int>(TypeId::Transform2D), 3);
    EXPECT_EQ(static_cast<int>(TypeId::BoundingBox), 4);
    
    // Verify shape types
    EXPECT_EQ(static_cast<int>(TypeId::Circle), 10);
    EXPECT_EQ(static_cast<int>(TypeId::Rectangle), 11);
    EXPECT_EQ(static_cast<int>(TypeId::Line), 12);
    EXPECT_EQ(static_cast<int>(TypeId::Ellipse), 13);
}

TEST_F(BinaryFormatTest, AlignmentHelper) {
    EXPECT_EQ(align_size(0, 8), 0);
    EXPECT_EQ(align_size(1, 8), 8);
    EXPECT_EQ(align_size(7, 8), 8);
    EXPECT_EQ(align_size(8, 8), 8);
    EXPECT_EQ(align_size(9, 8), 16);
    EXPECT_EQ(align_size(64, 8), 64);
    
    // Different alignments
    EXPECT_EQ(align_size(5, 4), 8);
    EXPECT_EQ(align_size(5, 16), 16);
}

TEST_F(BinaryFormatTest, EndiannessDetection) {
    // Just verify the function runs
    bool big = is_big_endian();
    EXPECT_TRUE(big || !big);  // Always true, just checking it compiles
}

TEST_F(BinaryFormatTest, EndiannessConversion16) {
    uint16_t value = 0x1234;
    uint16_t swapped = to_big_endian(value);
    
    if (is_big_endian()) {
        EXPECT_EQ(swapped, value);
    } else {
        EXPECT_EQ(swapped, 0x3412);
    }
    
    // Round trip
    EXPECT_EQ(to_big_endian(swapped), value);
}

TEST_F(BinaryFormatTest, EndiannessConversion32) {
    uint32_t value = 0x12345678;
    uint32_t swapped = to_big_endian(value);
    
    if (is_big_endian()) {
        EXPECT_EQ(swapped, value);
    } else {
        EXPECT_EQ(swapped, 0x78563412);
    }
    
    // Round trip
    EXPECT_EQ(to_big_endian(swapped), value);
}

TEST_F(BinaryFormatTest, EndiannessConversion64) {
    uint64_t value = 0x123456789ABCDEF0ULL;
    uint64_t swapped = to_big_endian(value);
    
    if (is_big_endian()) {
        EXPECT_EQ(swapped, value);
    } else {
        EXPECT_EQ(swapped, 0xF0DEBC9A78563412ULL);
    }
    
    // Round trip
    EXPECT_EQ(to_big_endian(swapped), value);
}

TEST_F(BinaryFormatTest, FileEndianConversion) {
    uint32_t value = 0x12345678;
    uint32_t file_value = to_file_endian(value);
    uint32_t restored = from_file_endian(file_value);
    
    EXPECT_EQ(restored, value);
}

TEST_F(BinaryFormatTest, CompressionTypes) {
    EXPECT_EQ(static_cast<int>(CompressionType::None), 0);
    EXPECT_EQ(static_cast<int>(CompressionType::LZ4), 1);
    EXPECT_EQ(static_cast<int>(CompressionType::Zstd), 2);
}

TEST_F(BinaryFormatTest, FormatFlags) {
    uint32_t flags = 0;
    
    flags |= static_cast<uint32_t>(FormatFlags::Compressed);
    EXPECT_NE(flags & static_cast<uint32_t>(FormatFlags::Compressed), 0);
    
    flags |= static_cast<uint32_t>(FormatFlags::Indexed);
    EXPECT_NE(flags & static_cast<uint32_t>(FormatFlags::Indexed), 0);
    
    flags |= static_cast<uint32_t>(FormatFlags::Streaming);
    EXPECT_NE(flags & static_cast<uint32_t>(FormatFlags::Streaming), 0);
    
    flags |= static_cast<uint32_t>(FormatFlags::ChecksumCRC32);
    EXPECT_NE(flags & static_cast<uint32_t>(FormatFlags::ChecksumCRC32), 0);
}

TEST_F(BinaryFormatTest, ChunkHeader) {
    ChunkHeader chunk;
    EXPECT_EQ(chunk.chunk_size, 0);
    EXPECT_EQ(chunk.object_count, 0);
    EXPECT_EQ(chunk.flags, 0);
    EXPECT_EQ(chunk.checksum, 0);
}

TEST_F(BinaryFormatTest, ObjectReference) {
    ObjectReference ref1;
    EXPECT_EQ(ref1.object_id, 0);
    
    ObjectReference ref2(42);
    EXPECT_EQ(ref2.object_id, 42);
}