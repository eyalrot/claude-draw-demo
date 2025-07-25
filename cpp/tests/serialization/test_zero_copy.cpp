#include <gtest/gtest.h>
#include "claude_draw/serialization/zero_copy.h"
#include "claude_draw/serialization/memory_mapped_file.h"
#include <cstdio>
#include <vector>
#include <chrono>

using namespace claude_draw;
using namespace claude_draw::serialization;

class ZeroCopyTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_zero_copy.bin";
    }
    
    void TearDown() override {
        // Clean up test files
        std::remove(test_file_.c_str());
    }
    
    std::string test_file_;
};

TEST_F(ZeroCopyTest, MemoryRegionAllocation) {
    MemoryRegion region(4096);
    
    // Test basic allocation
    int* i1 = region.allocate<int>();
    *i1 = 42;
    
    float* f1 = region.allocate<float>();
    *f1 = 3.14159f;
    
    double* d1 = region.allocate<double>();
    *d1 = 2.71828;
    
    // Verify values
    EXPECT_EQ(*region.get_at<int>(0), 42);
    EXPECT_FLOAT_EQ(*region.get_at<float>(sizeof(int)), 3.14159f);
    EXPECT_DOUBLE_EQ(*region.get_at<double>(sizeof(int) + sizeof(float)), 2.71828);
}

TEST_F(ZeroCopyTest, ArrayAllocation) {
    MemoryRegion region(4096);
    
    // Allocate array
    size_t count = 10;
    float* array = region.allocate_array<float>(count);
    
    // Initialize array
    for (size_t i = 0; i < count; ++i) {
        array[i] = static_cast<float>(i) * 1.5f;
    }
    
    // Verify values
    const float* read_array = region.get_at<float>(0);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_FLOAT_EQ(read_array[i], static_cast<float>(i) * 1.5f);
    }
}

TEST_F(ZeroCopyTest, AlignmentHandling) {
    MemoryRegion region(4096);
    
    // Allocate with different alignments
    char* c1 = region.allocate<char>();  // 1-byte alignment
    *c1 = 'A';
    
    size_t offset_before_double = region.offset();
    double* d1 = region.allocate<double>();  // 8-byte alignment
    *d1 = 123.456;
    
    // Check that double was properly aligned
    size_t double_offset = reinterpret_cast<uintptr_t>(d1) % alignof(double);
    EXPECT_EQ(double_offset, 0);
    
    // Offset should have been adjusted for alignment
    EXPECT_GE(region.offset() - offset_before_double, sizeof(double));
}

TEST_F(ZeroCopyTest, ZeroCopyWriterReader) {
    MemoryRegion region(4096);
    
    // Write data
    {
        ZeroCopyWriter writer(region);
        
        // Write primitive types
        writer.write(uint32_t(0xDEADBEEF));
        writer.write(float(3.14159f));
        writer.write(double(2.71828));
        
        // Write object with header
        Point2D point(10.0f, 20.0f);
        writer.write_object(TypeId::Point2D, 123, point);
        
        // Write array
        float data[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
        writer.write_array(data, 5);
        
        writer.finalize();
    }
    
    // Read data back
    {
        ZeroCopyReader reader(region.data(), region.offset());
        
        // Skip file header
        reader.seek(sizeof(FileHeader));
        
        // Read primitive types
        EXPECT_EQ(*reader.read<uint32_t>(), 0xDEADBEEF);
        EXPECT_FLOAT_EQ(*reader.read<float>(), 3.14159f);
        EXPECT_DOUBLE_EQ(*reader.read<double>(), 2.71828);
        
        // Read object with header
        ObjectHeader header;
        const Point2D* point = reader.read_object<Point2D>(header);
        EXPECT_EQ(header.type, TypeId::Point2D);
        EXPECT_EQ(header.object_id, 123);
        EXPECT_FLOAT_EQ(point->x, 10.0f);
        EXPECT_FLOAT_EQ(point->y, 20.0f);
        
        // Read array
        const float* array = reader.read_array<float>(5);
        for (int i = 0; i < 5; ++i) {
            EXPECT_FLOAT_EQ(array[i], static_cast<float>(i + 1));
        }
    }
}

TEST_F(ZeroCopyTest, CoreTypesSerialization) {
    MemoryRegion region(4096);
    
    // Write core types
    {
        ZeroCopyWriter writer(region);
        
        // Point2D
        Point2D point(100.0f, 200.0f);
        zero_copy::write_point(writer, point);
        
        // Color
        Color color(uint8_t(255), uint8_t(128), uint8_t(64), uint8_t(32));
        zero_copy::write_color(writer, color);
        
        // Transform2D
        Transform2D transform;
        transform.translate(10.0f, 20.0f);
        transform.rotate(45.0f);
        zero_copy::write_transform(writer, transform);
        
        // BoundingBox
        BoundingBox bbox(0.0f, 0.0f, 100.0f, 100.0f);
        zero_copy::write_bbox(writer, bbox);
        
        writer.finalize();
    }
    
    // Read core types back
    {
        ZeroCopyReader reader(region.data(), region.offset());
        reader.seek(sizeof(FileHeader));
        
        // Point2D
        const Point2D* point = zero_copy::read_point(reader);
        EXPECT_FLOAT_EQ(point->x, 100.0f);
        EXPECT_FLOAT_EQ(point->y, 200.0f);
        
        // Color
        const Color* color = zero_copy::read_color(reader);
        EXPECT_EQ(color->r, 255);
        EXPECT_EQ(color->g, 128);
        EXPECT_EQ(color->b, 64);
        EXPECT_EQ(color->a, 32);
        
        // Transform2D
        Transform2D transform = zero_copy::read_transform(reader);
        // Just verify it's a valid transform
        EXPECT_NEAR(transform(0, 0) * transform(0, 0) + transform(1, 0) * transform(1, 0), 1.0f, 0.1f);
        
        // BoundingBox
        const BoundingBox* bbox = zero_copy::read_bbox(reader);
        EXPECT_FLOAT_EQ(bbox->min_x, 0.0f);
        EXPECT_FLOAT_EQ(bbox->min_y, 0.0f);
        EXPECT_FLOAT_EQ(bbox->max_x, 100.0f);
        EXPECT_FLOAT_EQ(bbox->max_y, 100.0f);
    }
}

TEST_F(ZeroCopyTest, MemoryMappedFileWrite) {
    size_t file_size = 4096;
    
    // Create and write to memory-mapped file
    {
        auto mapped_file = MappedFile::create_for_write(test_file_, file_size);
        
        // Write directly to the mapped memory
        uint8_t* data = static_cast<uint8_t*>(mapped_file->data());
        
        // Initialize file header
        FileHeader* header = reinterpret_cast<FileHeader*>(data);
        new (header) FileHeader();
        
        size_t offset = sizeof(FileHeader);
        
        // Write some test data
        uint32_t* value1 = reinterpret_cast<uint32_t*>(data + offset);
        *value1 = 0x12345678;
        offset += sizeof(uint32_t);
        
        float* value2 = reinterpret_cast<float*>(data + offset);
        *value2 = 3.14159f;
        offset += sizeof(float);
        
        // Update header
        header->object_count = 2;
        header->total_size = offset;
        
        mapped_file->sync();
    }
    
    // Read from memory-mapped file
    {
        auto mapped_file = MappedFile::open_for_read(test_file_);
        const uint8_t* data = static_cast<const uint8_t*>(mapped_file->data());
        
        // Verify header
        const FileHeader* header = reinterpret_cast<const FileHeader*>(data);
        EXPECT_EQ(header->magic, MAGIC_NUMBER);
        EXPECT_EQ(header->object_count, 2);
        
        size_t offset = sizeof(FileHeader);
        
        // Read test data
        const uint32_t* value1 = reinterpret_cast<const uint32_t*>(data + offset);
        EXPECT_EQ(*value1, 0x12345678);
        offset += sizeof(uint32_t);
        
        const float* value2 = reinterpret_cast<const float*>(data + offset);
        EXPECT_FLOAT_EQ(*value2, 3.14159f);
    }
}

TEST_F(ZeroCopyTest, LargeDataPerformance) {
    const size_t num_points = 10000;
    MemoryRegion region(sizeof(FileHeader) + num_points * sizeof(Point2D) + 1024);
    
    // Measure write performance
    auto write_start = std::chrono::high_resolution_clock::now();
    {
        ZeroCopyWriter writer(region);
        
        for (size_t i = 0; i < num_points; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i * 2));
            writer.write(point);
        }
        
        writer.finalize();
    }
    auto write_end = std::chrono::high_resolution_clock::now();
    auto write_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        write_end - write_start).count();
    
    // Measure read performance
    auto read_start = std::chrono::high_resolution_clock::now();
    {
        ZeroCopyReader reader(region.data(), region.offset());
        reader.seek(sizeof(FileHeader));
        
        for (size_t i = 0; i < num_points; ++i) {
            const Point2D* point = reader.read<Point2D>();
            // Just access the data to prevent optimization
            volatile float x = point->x;
            volatile float y = point->y;
            (void)x;
            (void)y;
        }
    }
    auto read_end = std::chrono::high_resolution_clock::now();
    auto read_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        read_end - read_start).count();
    
    // Zero-copy should be very fast
    double write_throughput = (num_points * sizeof(Point2D)) / (write_duration / 1000000.0) / (1024 * 1024);
    double read_throughput = (num_points * sizeof(Point2D)) / (read_duration / 1000000.0) / (1024 * 1024);
    
    // Log performance results
    std::cout << "Zero-copy write throughput: " << write_throughput << " MB/s" << std::endl;
    std::cout << "Zero-copy read throughput: " << read_throughput << " MB/s" << std::endl;
    
    // Should be very fast (> 1000 MB/s for in-memory operations)
    EXPECT_GT(write_throughput, 500.0);
    EXPECT_GT(read_throughput, 1000.0);
}

TEST_F(ZeroCopyTest, DataIntegrity) {
    MemoryRegion region(8192);
    
    // Create complex test data
    struct TestData {
        uint32_t id;
        float values[4];
        double timestamp;
        char name[32];
    };
    
    static_assert(is_zero_copy_safe_v<TestData>, "TestData must be trivially copyable");
    
    const size_t num_objects = 50;
    
    // Write test data
    {
        ZeroCopyWriter writer(region);
        
        for (size_t i = 0; i < num_objects; ++i) {
            TestData* data = writer.construct<TestData>();
            data->id = static_cast<uint32_t>(i);
            for (int j = 0; j < 4; ++j) {
                data->values[j] = static_cast<float>(i * 10 + j);
            }
            data->timestamp = static_cast<double>(i) * 1000.0;
            snprintf(data->name, sizeof(data->name), "Object_%zu", i);
        }
        
        writer.finalize();
    }
    
    // Verify data integrity
    {
        ZeroCopyReader reader(region.data(), region.offset());
        reader.seek(sizeof(FileHeader));
        
        for (size_t i = 0; i < num_objects; ++i) {
            const TestData* data = reader.read<TestData>();
            
            EXPECT_EQ(data->id, static_cast<uint32_t>(i));
            for (int j = 0; j < 4; ++j) {
                EXPECT_FLOAT_EQ(data->values[j], static_cast<float>(i * 10 + j));
            }
            EXPECT_DOUBLE_EQ(data->timestamp, static_cast<double>(i) * 1000.0);
            
            char expected_name[32];
            snprintf(expected_name, sizeof(expected_name), "Object_%zu", i);
            EXPECT_STREQ(data->name, expected_name);
        }
    }
}

TEST_F(ZeroCopyTest, BoundaryConditions) {
    // Test small region
    {
        MemoryRegion small_region(sizeof(FileHeader) + 10);  // Just enough for header + a few bytes
        ZeroCopyWriter writer(small_region);
        
        // Should be able to write a few bytes after header
        writer.write(uint8_t(42));
        
        // Should throw when out of space
        EXPECT_THROW({
            for (int i = 0; i < 100; ++i) {
                writer.write(uint32_t(i));
            }
        }, std::runtime_error);
    }
    
    // Test alignment at boundary
    {
        MemoryRegion region(sizeof(FileHeader) + 9);  // 9 bytes after header
        ZeroCopyWriter writer(region);
        
        writer.write(uint8_t(1));  // 1 byte
        
        // This should require alignment padding and might not fit
        bool threw = false;
        try {
            writer.write(double(3.14));  // 8 bytes, needs 8-byte alignment
        } catch (const std::runtime_error&) {
            threw = true;
        }
        
        // Either it fit with alignment, or it threw
        EXPECT_TRUE(writer.object_count() == 2 || threw);
    }
}

TEST_F(ZeroCopyTest, ZeroCopyReaderNavigation) {
    MemoryRegion region(4096);
    
    // Write known pattern
    {
        ZeroCopyWriter writer(region);
        
        for (int i = 0; i < 10; ++i) {
            writer.write(static_cast<uint32_t>(i * 100));
        }
        
        writer.finalize();
    }
    
    // Test navigation
    {
        ZeroCopyReader reader(region.data(), region.offset());
        size_t header_size = sizeof(FileHeader);
        
        // Reader should start after header
        reader.seek(header_size);
        
        // Test tell()
        EXPECT_EQ(reader.tell(), header_size);
        
        // Test skip()
        reader.skip(sizeof(uint32_t) * 2);
        EXPECT_EQ(*reader.read<uint32_t>(), 200);  // Should be at third element
        
        // Test seek()
        reader.seek(header_size + sizeof(uint32_t) * 5);
        EXPECT_EQ(*reader.read<uint32_t>(), 500);
        
        // Test remaining()
        size_t expected_remaining = region.offset() - reader.tell();
        EXPECT_EQ(reader.remaining(), expected_remaining);
        
        // Test eof()
        reader.seek(region.offset());
        EXPECT_TRUE(reader.eof());
    }
}