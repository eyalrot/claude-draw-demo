#pragma once

#include "claude_draw/serialization/binary_format.h"
#include "claude_draw/serialization/binary_serializer.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/serialization/compression.h"
#include "claude_draw/serialization/validation.h"
#include <functional>
#include <atomic>
#include <thread>
#include <future>

namespace claude_draw {
namespace serialization {

/**
 * @brief Progress information for streaming operations
 */
struct StreamingProgress {
    size_t bytes_processed;
    size_t total_bytes;
    size_t objects_processed;
    size_t total_objects;
    double percentage;
    std::chrono::milliseconds elapsed_time;
    std::chrono::milliseconds estimated_time_remaining;
    
    StreamingProgress()
        : bytes_processed(0)
        , total_bytes(0)
        , objects_processed(0)
        , total_objects(0)
        , percentage(0.0)
        , elapsed_time(0)
        , estimated_time_remaining(0) {}
    
    bool is_complete() const {
        return percentage >= 100.0 || 
               (total_bytes > 0 && bytes_processed >= total_bytes) ||
               (total_objects > 0 && objects_processed >= total_objects);
    }
};

/**
 * @brief Callback types for streaming operations
 */
using ProgressCallback = std::function<void(const StreamingProgress&)>;
using ErrorCallback = std::function<void(const std::string& error)>;
using ObjectCallback = std::function<void(TypeId type, uint32_t id, const void* data, size_t size)>;

/**
 * @brief Configuration for streaming operations
 */
struct StreamingConfig {
    size_t chunk_size = 1024 * 1024;        // 1MB default chunk size
    size_t buffer_count = 2;                // Double buffering by default
    CompressionType compression = CompressionType::None;
    CompressionLevel compression_level = CompressionLevel::Default;
    bool enable_checksums = true;
    bool enable_progress = true;
    std::chrono::milliseconds progress_interval = std::chrono::milliseconds(100);
    size_t max_memory_usage = 100 * 1024 * 1024;  // 100MB max memory
};

/**
 * @brief Base class for streaming operations with interruption support
 */
class StreamingOperation {
public:
    StreamingOperation() : interrupted_(false), paused_(false) {}
    virtual ~StreamingOperation() = default;
    
    /**
     * @brief Request interruption of the operation
     */
    void interrupt() {
        interrupted_ = true;
        cv_.notify_all();
    }
    
    /**
     * @brief Pause the operation
     */
    void pause() {
        paused_ = true;
    }
    
    /**
     * @brief Resume the operation
     */
    void resume() {
        paused_ = false;
        cv_.notify_all();
    }
    
    /**
     * @brief Check if operation was interrupted
     */
    bool is_interrupted() const {
        return interrupted_;
    }
    
    /**
     * @brief Check if operation is paused
     */
    bool is_paused() const {
        return paused_;
    }
    
protected:
    /**
     * @brief Wait for resume if paused, check for interruption
     * @return true if should continue, false if interrupted
     */
    bool wait_if_paused() {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return !paused_ || interrupted_; });
        return !interrupted_;
    }
    
    std::atomic<bool> interrupted_;
    std::atomic<bool> paused_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
};

/**
 * @brief Streaming writer with progress and interruption support
 */
class StreamingWriter : public StreamingOperation {
public:
    StreamingWriter(const std::string& filename, const StreamingConfig& config = StreamingConfig())
        : filename_(filename)
        , config_(config)
        , bytes_written_(0)
        , objects_written_(0)
        , start_time_(std::chrono::steady_clock::now()) {
        
        file_.open(filename, std::ios::binary);
        if (!file_) {
            throw std::runtime_error("Failed to open file for streaming write: " + filename);
        }
        
        // Reserve space for file header
        FileHeader dummy_header;
        file_.write(reinterpret_cast<const char*>(&dummy_header), sizeof(FileHeader));
        
        // Initialize chunk buffer
        chunk_buffer_.reserve(config_.chunk_size);
        
        // Setup compression if needed
        if (config_.compression != CompressionType::None) {
            compressor_ = CompressionFactory::create(config_.compression);
        }
    }
    
    ~StreamingWriter() {
        if (file_.is_open() && !interrupted_) {
            try {
                finalize();
            } catch (...) {
                // Suppress exceptions in destructor
            }
        }
    }
    
    /**
     * @brief Set progress callback
     */
    void set_progress_callback(ProgressCallback callback) {
        progress_callback_ = callback;
    }
    
    /**
     * @brief Set error callback
     */
    void set_error_callback(ErrorCallback callback) {
        error_callback_ = callback;
    }
    
    /**
     * @brief Write an object to the stream
     */
    template<typename T>
    bool write_object(TypeId type, uint32_t id, const T& obj) {
        if (!wait_if_paused()) return false;
        
        try {
            // Create object header
            ObjectHeader header(type, id, sizeof(T));
            
            // Check if this would exceed chunk size
            size_t required = sizeof(ObjectHeader) + sizeof(T);
            if (chunk_buffer_.size() + required > config_.chunk_size) {
                flush_chunk();
            }
            
            // Convert header to file endianness
            ObjectHeader file_header = header;
            file_header.object_id = to_file_endian(file_header.object_id);
            file_header.data_size = to_file_endian(file_header.data_size);
            
            // Write to chunk buffer
            const uint8_t* header_bytes = reinterpret_cast<const uint8_t*>(&file_header);
            chunk_buffer_.insert(chunk_buffer_.end(), header_bytes, header_bytes + sizeof(ObjectHeader));
            
            const uint8_t* obj_bytes = reinterpret_cast<const uint8_t*>(&obj);
            chunk_buffer_.insert(chunk_buffer_.end(), obj_bytes, obj_bytes + sizeof(T));
            
            objects_written_++;
            
            // Update progress if needed
            if (config_.enable_progress && progress_callback_) {
                update_progress();
            }
            
            return true;
            
        } catch (const std::exception& e) {
            if (error_callback_) {
                error_callback_(e.what());
            }
            return false;
        }
    }
    
    /**
     * @brief Write raw data to the stream
     */
    bool write_raw(const void* data, size_t size) {
        if (!wait_if_paused()) return false;
        
        try {
            const uint8_t* bytes = static_cast<const uint8_t*>(data);
            
            // Handle large data that exceeds chunk size
            size_t offset = 0;
            while (offset < size) {
                size_t remaining = size - offset;
                size_t available = config_.chunk_size - chunk_buffer_.size();
                size_t to_write = std::min(remaining, available);
                
                chunk_buffer_.insert(chunk_buffer_.end(), 
                                   bytes + offset, 
                                   bytes + offset + to_write);
                offset += to_write;
                
                if (chunk_buffer_.size() >= config_.chunk_size) {
                    flush_chunk();
                }
            }
            
            return true;
            
        } catch (const std::exception& e) {
            if (error_callback_) {
                error_callback_(e.what());
            }
            return false;
        }
    }
    
    /**
     * @brief Flush current chunk to file
     */
    void flush_chunk() {
        if (chunk_buffer_.empty()) return;
        
        if (config_.compression != CompressionType::None) {
            // Compress chunk
            std::vector<uint8_t> compressed;
            [[maybe_unused]] auto stats = compressor_->compress(
                chunk_buffer_.data(),
                chunk_buffer_.size(),
                compressed,
                config_.compression_level
            );
            
            // Write compressed chunk header
            uint32_t chunk_header[2];
            chunk_header[0] = to_file_endian(static_cast<uint32_t>(chunk_buffer_.size()));
            chunk_header[1] = to_file_endian(static_cast<uint32_t>(compressed.size()));
            file_.write(reinterpret_cast<const char*>(chunk_header), sizeof(chunk_header));
            
            // Write compressed data
            file_.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
            bytes_written_ += sizeof(chunk_header) + compressed.size();
            
        } else {
            // Write uncompressed
            file_.write(reinterpret_cast<const char*>(chunk_buffer_.data()), chunk_buffer_.size());
            bytes_written_ += chunk_buffer_.size();
        }
        
        chunk_buffer_.clear();
    }
    
    /**
     * @brief Finalize the stream
     */
    void finalize() {
        if (finalized_) return;
        
        // Flush any remaining data
        flush_chunk();
        
        // Calculate checksum if enabled
        uint32_t checksum = 0;
        if (config_.enable_checksums) {
            // For streaming, we'll use a placeholder checksum for now
            // A proper implementation would maintain a running checksum
            checksum = 0;  // TODO: Implement running checksum
        }
        
        // Update file header
        file_.seekp(0);
        
        FileHeader header;
        header.magic = MAGIC_NUMBER;
        header.version_major = CURRENT_VERSION_MAJOR;
        header.version_minor = CURRENT_VERSION_MINOR;
        header.flags = static_cast<uint32_t>(FormatFlags::Streaming);
        header.compression = config_.compression;
        header.object_count = objects_written_;
        header.total_size = sizeof(FileHeader) + bytes_written_;
        header.checksum = checksum;
        
        if (config_.compression != CompressionType::None) {
            header.flags |= static_cast<uint32_t>(FormatFlags::Compressed);
        }
        
        // Convert to file endianness
        FileHeader file_header = header;
        file_header.magic = to_file_endian(file_header.magic);
        file_header.version_major = to_file_endian(file_header.version_major);
        file_header.version_minor = to_file_endian(file_header.version_minor);
        file_header.flags = to_file_endian(file_header.flags);
        file_header.total_size = to_file_endian(file_header.total_size);
        file_header.data_offset = to_file_endian(file_header.data_offset);
        file_header.object_count = to_file_endian(file_header.object_count);
        file_header.checksum = to_file_endian(file_header.checksum);
        
        file_.write(reinterpret_cast<const char*>(&file_header), sizeof(FileHeader));
        file_.close();
        
        finalized_ = true;
        
        // Final progress update
        if (config_.enable_progress && progress_callback_) {
            StreamingProgress progress;
            progress.bytes_processed = bytes_written_;
            progress.total_bytes = bytes_written_;
            progress.objects_processed = objects_written_;
            progress.total_objects = objects_written_;
            progress.percentage = 100.0;
            progress_callback_(progress);
        }
    }
    
    // Statistics
    size_t bytes_written() const { return bytes_written_; }
    size_t objects_written() const { return objects_written_; }
    
private:
    void update_progress() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_progress_time_);
        
        if (elapsed >= config_.progress_interval) {
            StreamingProgress progress;
            progress.bytes_processed = bytes_written_;
            progress.objects_processed = objects_written_;
            progress.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
            
            // Estimate completion based on current rate
            if (bytes_written_ > 0) {
                [[maybe_unused]] double rate = static_cast<double>(bytes_written_) / progress.elapsed_time.count();
                // We don't know total size for streaming, so percentage is indeterminate
                progress.percentage = -1.0;
            }
            
            progress_callback_(progress);
            last_progress_time_ = now;
        }
    }
    
    std::string filename_;
    StreamingConfig config_;
    std::ofstream file_;
    std::vector<uint8_t> chunk_buffer_;
    std::unique_ptr<Compressor> compressor_;
    size_t bytes_written_;
    size_t objects_written_;
    bool finalized_ = false;
    
    ProgressCallback progress_callback_;
    ErrorCallback error_callback_;
    
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_progress_time_;
};

/**
 * @brief Streaming reader with progress and interruption support
 */
class StreamingReader : public StreamingOperation {
public:
    StreamingReader(const std::string& filename, const StreamingConfig& config = StreamingConfig())
        : filename_(filename)
        , config_(config)
        , bytes_read_(0)
        , objects_read_(0)
        , start_time_(std::chrono::steady_clock::now()) {
        
        file_.open(filename, std::ios::binary);
        if (!file_) {
            throw std::runtime_error("Failed to open file for streaming read: " + filename);
        }
        
        // Read and validate file header
        file_.read(reinterpret_cast<char*>(&file_header_), sizeof(FileHeader));
        
        // Convert from file endianness
        file_header_.magic = from_file_endian(file_header_.magic);
        file_header_.version_major = from_file_endian(file_header_.version_major);
        file_header_.version_minor = from_file_endian(file_header_.version_minor);
        file_header_.flags = from_file_endian(file_header_.flags);
        file_header_.total_size = from_file_endian(file_header_.total_size);
        file_header_.data_offset = from_file_endian(file_header_.data_offset);
        file_header_.object_count = from_file_endian(file_header_.object_count);
        file_header_.checksum = from_file_endian(file_header_.checksum);
        
        if (!file_header_.validate()) {
            throw std::runtime_error("Invalid file header");
        }
        
        // Setup decompression if needed
        if (file_header_.compression != CompressionType::None) {
            compressor_ = CompressionFactory::create(file_header_.compression);
        }
        
        // Seek to data section
        file_.seekg(file_header_.data_offset);
        
        // Initialize buffer
        buffer_.reserve(config.chunk_size);
    }
    
    /**
     * @brief Set object callback
     */
    void set_object_callback(ObjectCallback callback) {
        object_callback_ = callback;
    }
    
    /**
     * @brief Set progress callback
     */
    void set_progress_callback(ProgressCallback callback) {
        progress_callback_ = callback;
    }
    
    /**
     * @brief Set error callback
     */
    void set_error_callback(ErrorCallback callback) {
        error_callback_ = callback;
    }
    
    /**
     * @brief Read all objects from the stream
     */
    bool read_all() {
        while (!file_.eof() && !interrupted_) {
            if (!wait_if_paused()) return false;
            
            if (!read_next_chunk()) {
                break;
            }
        }
        
        return !interrupted_ && objects_read_ == file_header_.object_count;
    }
    
    /**
     * @brief Read next object from the stream
     */
    bool read_next_object() {
        if (!wait_if_paused()) return false;
        
        // Ensure we have data in buffer
        if (buffer_offset_ >= buffer_valid_) {
            if (!read_next_chunk()) {
                return false;
            }
        }
        
        // Read object header
        if (buffer_offset_ + sizeof(ObjectHeader) > buffer_valid_) {
            return false;  // Incomplete object
        }
        
        ObjectHeader header;
        std::memcpy(&header, &buffer_[buffer_offset_], sizeof(ObjectHeader));
        buffer_offset_ += sizeof(ObjectHeader);
        
        // Convert endianness
        header.object_id = from_file_endian(header.object_id);
        header.data_size = from_file_endian(header.data_size);
        
        // Read object data
        if (buffer_offset_ + header.data_size > buffer_valid_) {
            return false;  // Incomplete object
        }
        
        // Invoke callback
        if (object_callback_) {
            object_callback_(header.type, header.object_id, 
                           &buffer_[buffer_offset_], header.data_size);
        }
        
        buffer_offset_ += header.data_size;
        objects_read_++;
        
        // Update progress
        if (config_.enable_progress && progress_callback_) {
            update_progress();
        }
        
        return true;
    }
    
    // Getters
    const FileHeader& file_header() const { return file_header_; }
    size_t bytes_read() const { return bytes_read_; }
    size_t objects_read() const { return objects_read_; }
    
private:
    bool read_next_chunk() {
        if (file_.eof()) return false;
        
        if (file_header_.compression != CompressionType::None) {
            // Read compressed chunk header
            uint32_t chunk_header[2];
            file_.read(reinterpret_cast<char*>(chunk_header), sizeof(chunk_header));
            if (!file_) return false;
            
            uint32_t uncompressed_size = from_file_endian(chunk_header[0]);
            uint32_t compressed_size = from_file_endian(chunk_header[1]);
            
            // Read compressed data
            std::vector<uint8_t> compressed(compressed_size);
            file_.read(reinterpret_cast<char*>(compressed.data()), compressed_size);
            if (!file_) return false;
            
            // Decompress
            buffer_.resize(uncompressed_size);
            size_t decompressed_size = compressor_->decompress(
                compressed.data(), compressed_size, buffer_, uncompressed_size);
            
            if (decompressed_size != uncompressed_size) {
                if (error_callback_) {
                    error_callback_("Decompression size mismatch");
                }
                return false;
            }
            
            bytes_read_ += sizeof(chunk_header) + compressed_size;
            
        } else {
            // Read uncompressed chunk
            buffer_.resize(config_.chunk_size);
            file_.read(reinterpret_cast<char*>(buffer_.data()), buffer_.size());
            size_t read_size = file_.gcount();
            buffer_.resize(read_size);
            bytes_read_ += read_size;
        }
        
        buffer_offset_ = 0;
        buffer_valid_ = buffer_.size();
        
        return buffer_valid_ > 0;
    }
    
    void update_progress() {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_progress_time_);
        
        if (elapsed >= config_.progress_interval) {
            StreamingProgress progress;
            progress.bytes_processed = bytes_read_;
            progress.total_bytes = file_header_.total_size - sizeof(FileHeader);
            progress.objects_processed = objects_read_;
            progress.total_objects = file_header_.object_count;
            progress.elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
            
            if (progress.total_bytes > 0) {
                progress.percentage = (static_cast<double>(progress.bytes_processed) / progress.total_bytes) * 100.0;
                
                // Estimate time remaining
                if (progress.percentage > 0) {
                    double total_time = progress.elapsed_time.count() / (progress.percentage / 100.0);
                    progress.estimated_time_remaining = std::chrono::milliseconds(
                        static_cast<long>(total_time - progress.elapsed_time.count())
                    );
                }
            }
            
            progress_callback_(progress);
            last_progress_time_ = now;
        }
    }
    
    std::string filename_;
    StreamingConfig config_;
    std::ifstream file_;
    FileHeader file_header_;
    std::vector<uint8_t> buffer_;
    size_t buffer_offset_ = 0;
    size_t buffer_valid_ = 0;
    std::unique_ptr<Compressor> compressor_;
    size_t bytes_read_;
    size_t objects_read_;
    
    ObjectCallback object_callback_;
    ProgressCallback progress_callback_;
    ErrorCallback error_callback_;
    
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point last_progress_time_;
};

/**
 * @brief Asynchronous streaming writer
 */
class AsyncStreamingWriter {
public:
    AsyncStreamingWriter(const std::string& filename, const StreamingConfig& config = StreamingConfig())
        : writer_(std::make_unique<StreamingWriter>(filename, config)) {}
    
    /**
     * @brief Write object asynchronously
     */
    template<typename T>
    std::future<bool> write_object_async(TypeId type, uint32_t id, const T& obj) {
        return std::async(std::launch::async, [this, type, id, obj]() {
            return writer_->write_object(type, id, obj);
        });
    }
    
    /**
     * @brief Finalize asynchronously
     */
    std::future<void> finalize_async() {
        return std::async(std::launch::async, [this]() {
            writer_->finalize();
        });
    }
    
    StreamingWriter& writer() { return *writer_; }
    
private:
    std::unique_ptr<StreamingWriter> writer_;
};

/**
 * @brief Asynchronous streaming reader
 */
class AsyncStreamingReader {
public:
    AsyncStreamingReader(const std::string& filename, const StreamingConfig& config = StreamingConfig())
        : reader_(std::make_unique<StreamingReader>(filename, config)) {}
    
    /**
     * @brief Read all objects asynchronously
     */
    std::future<bool> read_all_async() {
        return std::async(std::launch::async, [this]() {
            return reader_->read_all();
        });
    }
    
    StreamingReader& reader() { return *reader_; }
    
private:
    std::unique_ptr<StreamingReader> reader_;
};

} // namespace serialization
} // namespace claude_draw