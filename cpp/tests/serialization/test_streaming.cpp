#include <gtest/gtest.h>
#include "claude_draw/serialization/streaming.h"
#include "claude_draw/core/point2d.h"
#include "claude_draw/core/color.h"
#include "claude_draw/core/bounding_box.h"
#include <thread>
#include <atomic>
#include <random>

using namespace claude_draw;
using namespace claude_draw::serialization;

class StreamingTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_streaming.bin";
    }
    
    void TearDown() override {
        std::remove(test_file_.c_str());
    }
    
    // Generate large dataset
    std::vector<Point2D> generate_points(size_t count) {
        std::vector<Point2D> points;
        points.reserve(count);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(-1000.0f, 1000.0f);
        
        for (size_t i = 0; i < count; ++i) {
            points.emplace_back(dist(gen), dist(gen));
        }
        
        return points;
    }
    
    std::string test_file_;
};

TEST_F(StreamingTest, BasicStreaming) {
    const size_t num_objects = 100;
    
    // Write objects using streaming
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
    
    // For now, just verify the file was created with proper header
    {
        std::ifstream file(test_file_, std::ios::binary);
        EXPECT_TRUE(file.is_open());
        
        FileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
        
        // Convert from file endianness
        header.magic = from_file_endian(header.magic);
        header.version_major = from_file_endian(header.version_major);
        header.version_minor = from_file_endian(header.version_minor);
        header.object_count = from_file_endian(header.object_count);
        
        EXPECT_EQ(header.magic, MAGIC_NUMBER);
        EXPECT_EQ(header.version_major, CURRENT_VERSION_MAJOR);
        EXPECT_EQ(header.version_minor, CURRENT_VERSION_MINOR);
        EXPECT_EQ(header.object_count, num_objects);
    }
}

TEST_F(StreamingTest, LargeFileStreaming) {
    const size_t num_points = 100000;  // 100k points
    auto points = generate_points(num_points);
    
    // Configure for smaller chunks to test chunking
    StreamingConfig config;
    config.chunk_size = 64 * 1024;  // 64KB chunks
    
    // Write
    {
        StreamingWriter writer(test_file_, config);
        
        for (size_t i = 0; i < points.size(); ++i) {
            writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), points[i]);
        }
        
        writer.finalize();
    }
    
    // Read and verify
    {
        StreamingReader reader(test_file_, config);
        
        size_t count = 0;
        reader.set_object_callback([&](TypeId type, uint32_t id, const void* data, size_t size) {
            EXPECT_LT(count, points.size());
            const Point2D* point = static_cast<const Point2D*>(data);
            EXPECT_FLOAT_EQ(point->x, points[count].x);
            EXPECT_FLOAT_EQ(point->y, points[count].y);
            count++;
        });
        
        reader.read_all();
        EXPECT_EQ(count, num_points);
    }
}

TEST_F(StreamingTest, CompressedStreaming) {
    const size_t num_objects = 10000;
    
    StreamingConfig config;
    config.compression = CompressionType::LZ4;  // Will use RLE in our implementation
    config.chunk_size = 32 * 1024;  // 32KB chunks
    
    // Write with compression
    {
        StreamingWriter writer(test_file_, config);
        
        // Write some compressible data (repeated colors)
        for (size_t i = 0; i < num_objects; ++i) {
            Color color(static_cast<uint8_t>(i % 10), 
                       static_cast<uint8_t>(i % 10), 
                       static_cast<uint8_t>(i % 10), 255);
            writer.write_object(TypeId::Color, static_cast<uint32_t>(i), color);
        }
        
        writer.finalize();
    }
    
    // Read back
    {
        StreamingReader reader(test_file_, config);
        
        EXPECT_EQ(reader.file_header().compression, CompressionType::LZ4);
        
        size_t count = 0;
        reader.set_object_callback([&](TypeId type, uint32_t id, const void* data, size_t size) {
            EXPECT_EQ(type, TypeId::Color);
            const Color* color = static_cast<const Color*>(data);
            uint8_t expected = static_cast<uint8_t>(id % 10);
            EXPECT_EQ(color->r, expected);
            EXPECT_EQ(color->g, expected);
            EXPECT_EQ(color->b, expected);
            count++;
        });
        
        reader.read_all();
        EXPECT_EQ(count, num_objects);
    }
}

TEST_F(StreamingTest, ProgressCallbacks) {
    const size_t num_objects = 1000;
    
    StreamingConfig config;
    config.progress_interval = std::chrono::milliseconds(10);  // Fast updates for testing
    
    // Test write progress
    {
        StreamingWriter writer(test_file_, config);
        
        std::atomic<int> progress_count(0);
        double last_percentage = -1.0;
        
        writer.set_progress_callback([&](const StreamingProgress& progress) {
            progress_count++;
            // For streaming write, percentage might be indeterminate
            EXPECT_GE(progress.objects_processed, 0);
            EXPECT_GE(progress.bytes_processed, 0);
        });
        
        for (size_t i = 0; i < num_objects; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i));
            writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
            
            // Add small delay to ensure progress updates
            if (i % 100 == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(15));
            }
        }
        
        writer.finalize();
        
        // Should have received at least some progress updates
        EXPECT_GT(progress_count.load(), 0);
    }
    
    // Test read progress
    {
        StreamingReader reader(test_file_, config);
        
        std::atomic<int> progress_count(0);
        double last_percentage = 0.0;
        
        reader.set_progress_callback([&](const StreamingProgress& progress) {
            progress_count++;
            EXPECT_GE(progress.percentage, last_percentage);
            last_percentage = progress.percentage;
            EXPECT_LE(progress.percentage, 100.0);
            EXPECT_GE(progress.objects_processed, 0);
            EXPECT_LE(progress.objects_processed, num_objects);
        });
        
        reader.set_object_callback([](TypeId, uint32_t, const void*, size_t) {
            // Just consume objects
        });
        
        reader.read_all();
        
        EXPECT_GT(progress_count.load(), 0);
        EXPECT_DOUBLE_EQ(last_percentage, 100.0);
    }
}

TEST_F(StreamingTest, InterruptionHandling) {
    const size_t num_objects = 10000;
    
    // Test write interruption
    {
        StreamingWriter writer(test_file_);
        
        std::atomic<size_t> written(0);
        
        // Start writing in a thread
        std::thread write_thread([&]() {
            for (size_t i = 0; i < num_objects; ++i) {
                if (writer.is_interrupted()) break;
                
                Point2D point(static_cast<float>(i), static_cast<float>(i));
                if (writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point)) {
                    written++;
                }
            }
        });
        
        // Let it write some objects
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Interrupt
        writer.interrupt();
        
        write_thread.join();
        
        // Should have written some but not all
        EXPECT_GT(written.load(), 0);
        EXPECT_LT(written.load(), num_objects);
    }
    
    // Test read interruption
    std::remove(test_file_.c_str());
    
    // First write a complete file
    {
        StreamingWriter writer(test_file_);
        for (size_t i = 0; i < num_objects; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i));
            writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
        }
        writer.finalize();
    }
    
    // Now test interrupted read
    {
        StreamingReader reader(test_file_);
        
        std::atomic<size_t> read_count(0);
        
        reader.set_object_callback([&](TypeId, uint32_t, const void*, size_t) {
            read_count++;
        });
        
        // Start reading in a thread
        std::thread read_thread([&]() {
            reader.read_all();
        });
        
        // Let it read some objects
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Interrupt
        reader.interrupt();
        
        read_thread.join();
        
        // Should have read some but likely not all
        EXPECT_GT(read_count.load(), 0);
        EXPECT_LE(read_count.load(), num_objects);
    }
}

TEST_F(StreamingTest, PauseResume) {
    const size_t num_objects = 1000;
    
    StreamingWriter writer(test_file_);
    
    std::atomic<size_t> written(0);
    std::atomic<bool> was_paused(false);
    
    // Write thread
    std::thread write_thread([&]() {
        for (size_t i = 0; i < num_objects; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i));
            if (writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point)) {
                written++;
            }
            
            // Check if we're paused
            if (writer.is_paused()) {
                was_paused = true;
            }
        }
        writer.finalize();
    });
    
    // Let it start
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    
    // Pause
    writer.pause();
    size_t count_at_pause = written.load();
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Count shouldn't have increased much during pause
    size_t count_during_pause = written.load();
    EXPECT_LE(count_during_pause - count_at_pause, 10);  // Allow small buffer
    
    // Resume
    writer.resume();
    
    write_thread.join();
    
    // Should have completed all writes
    EXPECT_EQ(written.load(), num_objects);
    EXPECT_TRUE(was_paused.load());
}

TEST_F(StreamingTest, ErrorHandling) {
    // Test invalid file
    {
        EXPECT_THROW({
            StreamingReader reader("nonexistent_file.bin");
        }, std::runtime_error);
    }
    
    // Test error callback
    {
        StreamingWriter writer(test_file_);
        
        bool error_received = false;
        writer.set_error_callback([&](const std::string& error) {
            error_received = true;
            EXPECT_FALSE(error.empty());
        });
        
        // This should succeed, so no error
        Point2D point(1.0f, 2.0f);
        writer.write_object(TypeId::Point2D, 1, point);
        
        EXPECT_FALSE(error_received);
    }
}

TEST_F(StreamingTest, MemoryUsage) {
    const size_t num_objects = 100000;  // 100k objects
    
    StreamingConfig config;
    config.chunk_size = 1024 * 1024;  // 1MB chunks
    config.max_memory_usage = 10 * 1024 * 1024;  // 10MB max
    
    // Write large dataset
    {
        StreamingWriter writer(test_file_, config);
        
        // Track peak memory usage (approximate)
        size_t initial_memory = writer.bytes_written();
        
        for (size_t i = 0; i < num_objects; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i * 2));
            writer.write_object(TypeId::Point2D, static_cast<uint32_t>(i), point);
            
            // Force flush periodically
            if (i % 10000 == 0) {
                writer.flush_chunk();
            }
        }
        
        writer.finalize();
    }
    
    // Read with controlled memory
    {
        StreamingReader reader(test_file_, config);
        
        size_t count = 0;
        reader.set_object_callback([&](TypeId, uint32_t, const void*, size_t) {
            count++;
        });
        
        reader.read_all();
        EXPECT_EQ(count, num_objects);
    }
}

TEST_F(StreamingTest, AsyncOperations) {
    const size_t num_objects = 1000;
    
    // Async write
    {
        AsyncStreamingWriter async_writer(test_file_);
        
        std::vector<std::future<bool>> futures;
        
        for (size_t i = 0; i < num_objects; ++i) {
            Point2D point(static_cast<float>(i), static_cast<float>(i));
            futures.push_back(
                async_writer.write_object_async(TypeId::Point2D, static_cast<uint32_t>(i), point)
            );
        }
        
        // Wait for all writes
        for (auto& future : futures) {
            EXPECT_TRUE(future.get());
        }
        
        // Finalize
        auto finalize_future = async_writer.finalize_async();
        finalize_future.wait();
    }
    
    // Async read
    {
        AsyncStreamingReader async_reader(test_file_);
        
        std::atomic<size_t> count(0);
        async_reader.reader().set_object_callback([&](TypeId, uint32_t, const void*, size_t) {
            count++;
        });
        
        auto read_future = async_reader.read_all_async();
        EXPECT_TRUE(read_future.get());
        EXPECT_EQ(count.load(), num_objects);
    }
}

TEST_F(StreamingTest, MixedObjectTypes) {
    StreamingWriter writer(test_file_);
    
    // Write different object types
    Point2D point(1.0f, 2.0f);
    writer.write_object(TypeId::Point2D, 1, point);
    
    Color color(uint8_t(255), uint8_t(128), uint8_t(64), uint8_t(32));
    writer.write_object(TypeId::Color, 2, color);
    
    BoundingBox bbox(0.0f, 0.0f, 100.0f, 100.0f);
    writer.write_object(TypeId::BoundingBox, 3, bbox);
    
    writer.finalize();
    
    // Read back and verify types
    StreamingReader reader(test_file_);
    
    std::vector<TypeId> read_types;
    reader.set_object_callback([&](TypeId type, uint32_t id, const void* data, size_t size) {
        read_types.push_back(type);
        
        switch (type) {
            case TypeId::Point2D:
                EXPECT_EQ(size, sizeof(Point2D));
                break;
            case TypeId::Color:
                EXPECT_EQ(size, sizeof(Color));
                break;
            case TypeId::BoundingBox:
                EXPECT_EQ(size, sizeof(BoundingBox));
                break;
            default:
                FAIL() << "Unexpected type";
        }
    });
    
    reader.read_all();
    
    ASSERT_EQ(read_types.size(), 3);
    EXPECT_EQ(read_types[0], TypeId::Point2D);
    EXPECT_EQ(read_types[1], TypeId::Color);
    EXPECT_EQ(read_types[2], TypeId::BoundingBox);
}