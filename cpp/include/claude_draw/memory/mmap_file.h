#pragma once

#include <string>
#include <memory>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <span>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace claude_draw {
namespace memory {

/**
 * @brief Memory-mapped file for efficient file I/O
 * 
 * Provides cross-platform memory-mapped file access for large data sets.
 * Supports both read-only and read-write access with automatic file resizing.
 * 
 * Features:
 * - Zero-copy access to file data
 * - Automatic file creation and resizing
 * - Cross-platform (Windows/POSIX)
 * - RAII-style resource management
 * - Support for sparse files
 */
class MemoryMappedFile {
public:
    enum class Mode {
        ReadOnly,
        ReadWrite,
        CreateNew     // Create new file, fail if exists
    };
    
    enum class SyncMode {
        Async,        // Changes written asynchronously
        Sync          // Changes written synchronously
    };
    
    /**
     * @brief Constructor
     */
    MemoryMappedFile() = default;
    
    /**
     * @brief Open a memory-mapped file
     * @param filename Path to the file
     * @param mode Access mode
     * @param size Initial size (0 = use existing file size)
     * @return True on success
     */
    bool open(const std::string& filename, Mode mode = Mode::ReadOnly, size_t size = 0);
    
    /**
     * @brief Close the memory-mapped file
     */
    void close();
    
    /**
     * @brief Destructor - automatically closes file
     */
    ~MemoryMappedFile() {
        close();
    }
    
    // Disable copy
    MemoryMappedFile(const MemoryMappedFile&) = delete;
    MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
    
    // Enable move
    MemoryMappedFile(MemoryMappedFile&& other) noexcept;
    MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept;
    
    /**
     * @brief Get pointer to mapped memory
     * @return Pointer to mapped memory or nullptr if not mapped
     */
    void* data() { return data_; }
    const void* data() const { return data_; }
    
    /**
     * @brief Get typed pointer to mapped memory
     */
    template<typename T>
    T* data_as() { return static_cast<T*>(data_); }
    
    template<typename T>
    const T* data_as() const { return static_cast<const T*>(data_); }
    
    /**
     * @brief Get span view of mapped memory
     */
    template<typename T>
    std::span<T> as_span() {
        return std::span<T>(data_as<T>(), size_ / sizeof(T));
    }
    
    template<typename T>
    std::span<const T> as_span() const {
        return std::span<const T>(data_as<T>(), size_ / sizeof(T));
    }
    
    /**
     * @brief Get size of mapped region
     */
    size_t size() const { return size_; }
    
    /**
     * @brief Check if file is open
     */
    bool is_open() const {
#ifdef _WIN32
        return file_handle_ != INVALID_HANDLE_VALUE;
#else
        return file_descriptor_ >= 0;
#endif
    }
    
    /**
     * @brief Resize the mapped file
     * @param new_size New size in bytes
     * @return True on success
     */
    bool resize(size_t new_size);
    
    /**
     * @brief Sync changes to disk
     * @param mode Synchronous or asynchronous sync
     * @return True on success
     */
    bool sync(SyncMode mode = SyncMode::Async);
    
    /**
     * @brief Advise kernel about access pattern
     */
    enum class AccessPattern {
        Normal,       // No special treatment
        Sequential,   // Expect sequential access
        Random,       // Expect random access
        WillNeed,     // Will need this range soon
        DontNeed      // Won't need this range soon
    };
    
    bool advise(AccessPattern pattern, size_t offset = 0, size_t length = 0);
    
    /**
     * @brief Lock pages in memory (prevent swapping)
     */
    bool lock(size_t offset = 0, size_t length = 0);
    bool unlock(size_t offset = 0, size_t length = 0);
    
    /**
     * @brief Get last error message
     */
    std::string get_error() const { return error_message_; }
    
    /**
     * @brief Get file path
     */
    const std::string& filename() const { return filename_; }
    
protected:
#ifdef _WIN32
    HANDLE file_handle_ = INVALID_HANDLE_VALUE;
    HANDLE mapping_handle_ = INVALID_HANDLE_VALUE;
#else
    int file_descriptor_ = -1;
#endif

private:
    void* data_ = nullptr;
    size_t size_ = 0;
    Mode mode_ = Mode::ReadOnly;
    std::string filename_;
    std::string error_message_;
    
    void set_error(const std::string& msg);
    bool platform_open(const std::string& filename, Mode mode, size_t size);
    void platform_close();
    bool platform_resize(size_t new_size);
    bool platform_sync(SyncMode mode);
    bool platform_advise(AccessPattern pattern, size_t offset, size_t length);
    bool platform_lock(size_t offset, size_t length, bool lock);
};

/**
 * @brief Memory-mapped arena allocator
 * 
 * Arena allocator that uses memory-mapped files as backing storage.
 * Provides persistent memory allocation with zero-copy access.
 */
class MemoryMappedArena {
public:
    struct Header {
        uint32_t magic = 0x4D4D4152;  // "MMAR"
        uint32_t version = 1;
        uint64_t total_size;
        uint64_t used_size;
        uint64_t block_size;
        uint32_t checksum;
        uint32_t flags;
        uint64_t reserved[8];
    };
    
    static constexpr size_t DEFAULT_BLOCK_SIZE = 1024 * 1024;  // 1MB blocks
    static constexpr size_t HEADER_SIZE = sizeof(Header);
    
    explicit MemoryMappedArena(size_t block_size = DEFAULT_BLOCK_SIZE)
        : block_size_(block_size) {}
    
    /**
     * @brief Open or create a memory-mapped arena
     * @param filename Path to the arena file
     * @param create Create new arena if file doesn't exist
     * @return True on success
     */
    bool open(const std::string& filename, bool create = true);
    
    /**
     * @brief Close the arena
     */
    void close();
    
    /**
     * @brief Allocate memory from the arena
     * @param size Number of bytes to allocate
     * @param alignment Required alignment
     * @return Pointer to allocated memory or nullptr
     */
    void* allocate(size_t size, size_t alignment = alignof(std::max_align_t));
    
    /**
     * @brief Reset the arena (keep file but reset allocations)
     */
    void reset();
    
    /**
     * @brief Get arena statistics
     */
    size_t get_used_size() const;
    size_t get_total_size() const;
    size_t get_available_size() const;
    
    /**
     * @brief Sync arena to disk
     */
    bool sync() { return file_.sync(); }
    
    /**
     * @brief Validate arena integrity
     */
    bool validate() const;
    
private:
    MemoryMappedFile file_;
    size_t block_size_;
    Header* header_ = nullptr;
    
    bool grow(size_t min_size);
    static uint32_t calculate_checksum(const Header* header);
    static size_t align_up(size_t value, size_t alignment);
};

/**
 * @brief Memory-mapped vector
 * 
 * Vector-like container backed by memory-mapped file.
 * Provides persistent storage with vector semantics.
 */
template<typename T>
class MemoryMappedVector {
public:
    struct Header {
        uint32_t magic = 0x4D4D5645;  // "MMVE"
        uint32_t version = 1;
        uint64_t element_size;
        uint64_t capacity;
        uint64_t size;
        uint32_t checksum;
        uint32_t flags;
        uint64_t reserved[8];
    };
    
    static constexpr size_t HEADER_SIZE = sizeof(Header);
    static constexpr size_t MIN_CAPACITY = 16;
    
    MemoryMappedVector() = default;
    
    /**
     * @brief Open or create a memory-mapped vector
     */
    bool open(const std::string& filename, bool create = true) {
        if (!file_.open(filename, create ? MemoryMappedFile::Mode::ReadWrite 
                                        : MemoryMappedFile::Mode::ReadOnly)) {
            return false;
        }
        
        if (file_.size() < HEADER_SIZE) {
            if (!create) return false;
            
            // Initialize new file
            if (!file_.resize(HEADER_SIZE + MIN_CAPACITY * sizeof(T))) {
                return false;
            }
            
            header_ = file_.data_as<Header>();
            header_->magic = 0x4D4D5645;
            header_->version = 1;
            header_->element_size = sizeof(T);
            header_->capacity = MIN_CAPACITY;
            header_->size = 0;
            header_->checksum = calculate_checksum(header_);
        } else {
            header_ = file_.data_as<Header>();
            if (!validate()) {
                close();
                return false;
            }
        }
        
        data_ = reinterpret_cast<T*>(reinterpret_cast<char*>(header_) + HEADER_SIZE);
        return true;
    }
    
    void close() {
        if (header_) {
            header_->checksum = calculate_checksum(header_);
            file_.sync();
        }
        header_ = nullptr;
        data_ = nullptr;
        file_.close();
    }
    
    // Vector-like interface
    size_t size() const { return header_ ? header_->size : 0; }
    size_t capacity() const { return header_ ? header_->capacity : 0; }
    bool empty() const { return size() == 0; }
    
    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }
    
    T& at(size_t idx) {
        if (idx >= size()) throw std::out_of_range("Index out of range");
        return data_[idx];
    }
    
    const T& at(size_t idx) const {
        if (idx >= size()) throw std::out_of_range("Index out of range");
        return data_[idx];
    }
    
    T* data() { return data_; }
    const T* data() const { return data_; }
    
    T& front() { return data_[0]; }
    const T& front() const { return data_[0]; }
    
    T& back() { return data_[header_->size - 1]; }
    const T& back() const { return data_[header_->size - 1]; }
    
    // Modifiers
    void push_back(const T& value) {
        if (header_->size >= header_->capacity) {
            if (!grow()) return;
        }
        data_[header_->size++] = value;
    }
    
    void pop_back() {
        if (header_->size > 0) {
            --header_->size;
        }
    }
    
    void clear() {
        header_->size = 0;
    }
    
    bool resize(size_t new_size) {
        if (new_size > header_->capacity) {
            if (!reserve(new_size)) return false;
        }
        header_->size = new_size;
        return true;
    }
    
    bool reserve(size_t new_capacity) {
        if (new_capacity <= header_->capacity) return true;
        
        size_t new_file_size = HEADER_SIZE + new_capacity * sizeof(T);
        if (!file_.resize(new_file_size)) return false;
        
        header_ = file_.data_as<Header>();
        data_ = reinterpret_cast<T*>(reinterpret_cast<char*>(header_) + HEADER_SIZE);
        header_->capacity = new_capacity;
        return true;
    }
    
    // Iterators
    T* begin() { return data_; }
    const T* begin() const { return data_; }
    
    T* end() { return data_ + header_->size; }
    const T* end() const { return data_ + header_->size; }
    
private:
    MemoryMappedFile file_;
    Header* header_ = nullptr;
    T* data_ = nullptr;
    
    bool validate() const {
        return header_->magic == 0x4D4D5645 &&
               header_->version == 1 &&
               header_->element_size == sizeof(T) &&
               header_->checksum == calculate_checksum(header_);
    }
    
    bool grow() {
        size_t new_capacity = header_->capacity * 2;
        return reserve(new_capacity);
    }
    
    static uint32_t calculate_checksum(const Header* header) {
        // Simple checksum for demonstration
        uint32_t sum = 0;
        const uint32_t* data = reinterpret_cast<const uint32_t*>(header);
        size_t count = offsetof(Header, checksum) / sizeof(uint32_t);
        for (size_t i = 0; i < count; ++i) {
            sum ^= data[i];
        }
        return sum;
    }
};

} // namespace memory
} // namespace claude_draw