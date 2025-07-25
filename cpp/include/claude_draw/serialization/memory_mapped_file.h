#pragma once

#include "claude_draw/serialization/zero_copy.h"
#include <string>
#include <memory>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace claude_draw {
namespace serialization {

#ifdef _WIN32

/**
 * @brief Windows implementation of memory-mapped file
 */
class WindowsMappedFile : public MappedFile {
public:
    WindowsMappedFile(const std::string& filename, size_t size, bool read_only)
        : file_handle_(INVALID_HANDLE_VALUE)
        , mapping_handle_(nullptr)
        , data_(nullptr)
        , size_(size) {
        
        // Open or create file
        if (read_only) {
            file_handle_ = CreateFileA(
                filename.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            
            if (file_handle_ == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            
            // Get file size
            LARGE_INTEGER file_size;
            if (!GetFileSizeEx(file_handle_, &file_size)) {
                CloseHandle(file_handle_);
                throw std::runtime_error("Failed to get file size");
            }
            size_ = static_cast<size_t>(file_size.QuadPart);
        } else {
            file_handle_ = CreateFileA(
                filename.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                nullptr,
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            
            if (file_handle_ == INVALID_HANDLE_VALUE) {
                throw std::runtime_error("Failed to create file: " + filename);
            }
            
            // Set file size
            LARGE_INTEGER file_size;
            file_size.QuadPart = size;
            if (!SetFilePointerEx(file_handle_, file_size, nullptr, FILE_BEGIN) ||
                !SetEndOfFile(file_handle_)) {
                CloseHandle(file_handle_);
                throw std::runtime_error("Failed to set file size");
            }
        }
        
        // Create file mapping
        mapping_handle_ = CreateFileMappingA(
            file_handle_,
            nullptr,
            read_only ? PAGE_READONLY : PAGE_READWRITE,
            0,
            0,
            nullptr
        );
        
        if (!mapping_handle_) {
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to create file mapping");
        }
        
        // Map view of file
        data_ = MapViewOfFile(
            mapping_handle_,
            read_only ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS,
            0,
            0,
            size_
        );
        
        if (!data_) {
            CloseHandle(mapping_handle_);
            CloseHandle(file_handle_);
            throw std::runtime_error("Failed to map view of file");
        }
    }
    
    ~WindowsMappedFile() override {
        if (data_) {
            UnmapViewOfFile(data_);
        }
        if (mapping_handle_) {
            CloseHandle(mapping_handle_);
        }
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
        }
    }
    
    void* data() override { return data_; }
    const void* data() const override { return data_; }
    size_t size() const override { return size_; }
    
    void sync() override {
        if (data_) {
            FlushViewOfFile(data_, size_);
        }
    }
    
private:
    HANDLE file_handle_;
    HANDLE mapping_handle_;
    void* data_;
    size_t size_;
};

#else

/**
 * @brief POSIX implementation of memory-mapped file
 */
class PosixMappedFile : public MappedFile {
public:
    PosixMappedFile(const std::string& filename, size_t size, bool read_only)
        : fd_(-1)
        , data_(nullptr)
        , size_(size)
        , read_only_(read_only) {
        
        // Open or create file
        if (read_only) {
            fd_ = open(filename.c_str(), O_RDONLY);
            if (fd_ < 0) {
                throw std::runtime_error("Failed to open file: " + filename);
            }
            
            // Get file size
            struct stat st;
            if (fstat(fd_, &st) < 0) {
                close(fd_);
                throw std::runtime_error("Failed to get file size");
            }
            size_ = static_cast<size_t>(st.st_size);
        } else {
            fd_ = open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (fd_ < 0) {
                throw std::runtime_error("Failed to create file: " + filename);
            }
            
            // Set file size
            if (ftruncate(fd_, static_cast<off_t>(size)) < 0) {
                close(fd_);
                throw std::runtime_error("Failed to set file size");
            }
        }
        
        // Map file to memory
        data_ = mmap(
            nullptr,
            size_,
            read_only ? PROT_READ : (PROT_READ | PROT_WRITE),
            MAP_SHARED,
            fd_,
            0
        );
        
        if (data_ == MAP_FAILED) {
            close(fd_);
            throw std::runtime_error("Failed to map file to memory");
        }
        
        // Advise kernel about access pattern
        madvise(data_, size_, MADV_SEQUENTIAL);
    }
    
    ~PosixMappedFile() override {
        if (data_ && data_ != MAP_FAILED) {
            munmap(data_, size_);
        }
        if (fd_ >= 0) {
            close(fd_);
        }
    }
    
    void* data() override { return data_; }
    const void* data() const override { return data_; }
    size_t size() const override { return size_; }
    
    void sync() override {
        if (data_ && data_ != MAP_FAILED) {
            msync(data_, size_, MS_SYNC);
        }
    }
    
private:
    int fd_;
    void* data_;
    size_t size_;
    bool read_only_;
};

#endif

// Factory methods implementation
inline std::unique_ptr<MappedFile> MappedFile::create_for_write(
    const std::string& filename, size_t size) {
#ifdef _WIN32
    return std::make_unique<WindowsMappedFile>(filename, size, false);
#else
    return std::make_unique<PosixMappedFile>(filename, size, false);
#endif
}

inline std::unique_ptr<MappedFile> MappedFile::open_for_read(
    const std::string& filename) {
#ifdef _WIN32
    return std::make_unique<WindowsMappedFile>(filename, 0, true);
#else
    return std::make_unique<PosixMappedFile>(filename, 0, true);
#endif
}

} // namespace serialization
} // namespace claude_draw