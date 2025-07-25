#include <gtest/gtest.h>
#include "claude_draw/serialization/streaming.h"
#include "claude_draw/core/point2d.h"
#include <fstream>

using namespace claude_draw;
using namespace claude_draw::serialization;

class SimpleStreamingTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_streaming_simple.bin";
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
    
    std::string test_file_;
};

TEST_F(SimpleStreamingTest, WriteAndVerifyHeader) {
    const size_t num_objects = 1000;
    
    // Write objects
    {
        StreamingWriter writer(test_file_);
        
        for (size_t i = 0; i < num_objects; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i * 2));
            EXPECT_TRUE(writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point));
        }
        
        writer.finalize();
        
        EXPECT_EQ(writer.objects_written(), num_objects);
        EXPECT_GT(writer.bytes_written(), 0);
    }
    
    // Verify file header
    {
        std::ifstream file(test_file_, std::ios::binary);
        ASSERT_TRUE(file.is_open());
        
        FileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
        
        // Convert from file endianness
        header.magic = from_file_endian(header.magic);
        header.version_major = from_file_endian(header.version_major);
        header.version_minor = from_file_endian(header.version_minor);
        header.flags = from_file_endian(header.flags);
        header.object_count = from_file_endian(header.object_count);
        header.total_size = from_file_endian(header.total_size);
        
        EXPECT_EQ(header.magic, MAGIC_NUMBER);
        EXPECT_EQ(header.version_major, CURRENT_VERSION_MAJOR);
        EXPECT_EQ(header.version_minor, CURRENT_VERSION_MINOR);
        EXPECT_EQ(header.object_count, num_objects);
        EXPECT_GT(header.total_size, sizeof(FileHeader));
        EXPECT_EQ(header.compression, CompressionType::None);
        
        // Check that streaming flag is set
        EXPECT_NE(header.flags & static_cast<uint32_t>(FormatFlags::Streaming), 0);
    }
}

TEST_F(SimpleStreamingTest, WriteWithCompression) {
    StreamingConfig config;
    config.compression = CompressionType::LZ4;  // Will use RLE
    
    StreamingWriter writer(test_file_, config);
    
    // Write some compressible data
    for (size_t i = 0; i < 100; ++i) {
        Point2D point(1.0f, 2.0f);  // Same point repeated
        writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
    }
    
    writer.finalize();
    
    // Verify header has compression
    std::ifstream file(test_file_, std::ios::binary);
    FileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
    
    header.compression = static_cast<CompressionType>(header.compression);
    header.flags = from_file_endian(header.flags);
    
    EXPECT_EQ(header.compression, CompressionType::LZ4);
    EXPECT_NE(header.flags & static_cast<uint32_t>(FormatFlags::Compressed), 0);
}

TEST_F(SimpleStreamingTest, ProgressCallback) {
    StreamingConfig config;
    config.progress_interval = std::chrono::milliseconds(1);
    
    StreamingWriter writer(test_file_, config);
    
    bool progress_called = false;
    writer.set_progress_callback([&](const StreamingProgress& progress) {
        progress_called = true;
        EXPECT_GE(progress.bytes_processed, 0);
        EXPECT_GE(progress.objects_processed, 0);
    });
    
    // Write enough objects to trigger progress
    for (size_t i = 0; i < 1000; ++i) {
        Point2D point(static_cast<float>(i), static_cast<float>(i));
        writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
    }
    
    writer.finalize();
    
    EXPECT_TRUE(progress_called);
}

TEST_F(SimpleStreamingTest, Interruption) {
    StreamingWriter writer(test_file_);
    
    // Write some objects
    for (size_t i = 0; i < 10; ++i) {
        Point2D point(static_cast<float>(i), static_cast<float>(i));
        writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
    }
    
    // Interrupt before finalizing
    writer.interrupt();
    
    // Further writes should fail
    Point2D point(100.0f, 200.0f);
    EXPECT_FALSE(writer.write_object(TypeId::Point2D, 100, point));
    
    EXPECT_TRUE(writer.is_interrupted());
}

TEST_F(SimpleStreamingTest, ChunkFlushing) {
    StreamingConfig config;
    config.chunk_size = 256;  // Small chunks to force flushing
    
    StreamingWriter writer(test_file_, config);
    
    size_t initial_bytes = 0;
    
    // Write objects that will exceed chunk size
    for (size_t i = 0; i < 50; ++i) {
        Point2D point(static_cast<float>(i), static_cast<float>(i));
        writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
        
        // Check if bytes written increased (indicates flush)
        if (i == 10) {
            initial_bytes = writer.bytes_written();
        }
    }
    
    // Should have flushed some chunks
    EXPECT_GT(writer.bytes_written(), initial_bytes);
    
    writer.finalize();
}