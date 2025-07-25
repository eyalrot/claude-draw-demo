#include "claude_draw/memory/mmap_file.h"
#include <cstring>
#include <algorithm>

#ifdef __linux__
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE  // For mremap
    #endif
    #include <sys/mman.h>  // Ensure mremap is declared
#endif

namespace claude_draw {
namespace memory {

// MemoryMappedFile implementation

MemoryMappedFile::MemoryMappedFile(MemoryMappedFile&& other) noexcept
    : 
#ifdef _WIN32
      file_handle_(other.file_handle_),
      mapping_handle_(other.mapping_handle_),
#else
      file_descriptor_(other.file_descriptor_),
#endif
      data_(other.data_),
      size_(other.size_),
      mode_(other.mode_),
      filename_(std::move(other.filename_)),
      error_message_(std::move(other.error_message_))
{
    other.data_ = nullptr;
    other.size_ = 0;
#ifdef _WIN32
    other.file_handle_ = INVALID_HANDLE_VALUE;
    other.mapping_handle_ = INVALID_HANDLE_VALUE;
#else
    other.file_descriptor_ = -1;
#endif
}

MemoryMappedFile& MemoryMappedFile::operator=(MemoryMappedFile&& other) noexcept {
    if (this != &other) {
        close();
        
        data_ = other.data_;
        size_ = other.size_;
        mode_ = other.mode_;
        filename_ = std::move(other.filename_);
        error_message_ = std::move(other.error_message_);
#ifdef _WIN32
        file_handle_ = other.file_handle_;
        mapping_handle_ = other.mapping_handle_;
#else
        file_descriptor_ = other.file_descriptor_;
#endif
        
        other.data_ = nullptr;
        other.size_ = 0;
#ifdef _WIN32
        other.file_handle_ = INVALID_HANDLE_VALUE;
        other.mapping_handle_ = INVALID_HANDLE_VALUE;
#else
        other.file_descriptor_ = -1;
#endif
    }
    return *this;
}

bool MemoryMappedFile::open(const std::string& filename, Mode mode, size_t size) {
    close();
    
    filename_ = filename;
    mode_ = mode;
    
    return platform_open(filename, mode, size);
}

void MemoryMappedFile::close() {
    if (data_) {
        platform_close();
        data_ = nullptr;
        size_ = 0;
    }
}

bool MemoryMappedFile::resize(size_t new_size) {
    if (!is_open()) {
        set_error("File not open");
        return false;
    }
    
    if (mode_ == Mode::ReadOnly) {
        set_error("Cannot resize read-only file");
        return false;
    }
    
    return platform_resize(new_size);
}

bool MemoryMappedFile::sync(SyncMode mode) {
    if (!is_open()) {
        set_error("File not open");
        return false;
    }
    
    return platform_sync(mode);
}

bool MemoryMappedFile::advise(AccessPattern pattern, size_t offset, size_t length) {
    if (!is_open()) {
        set_error("File not open");
        return false;
    }
    
    if (length == 0) length = size_ - offset;
    return platform_advise(pattern, offset, length);
}

bool MemoryMappedFile::lock(size_t offset, size_t length) {
    if (!is_open()) {
        set_error("File not open");
        return false;
    }
    
    if (length == 0) length = size_ - offset;
    return platform_lock(offset, length, true);
}

bool MemoryMappedFile::unlock(size_t offset, size_t length) {
    if (!is_open()) {
        set_error("File not open");
        return false;
    }
    
    if (length == 0) length = size_ - offset;
    return platform_lock(offset, length, false);
}

void MemoryMappedFile::set_error(const std::string& msg) {
    error_message_ = msg;
}

// Platform-specific implementations

#ifdef _WIN32

bool MemoryMappedFile::platform_open(const std::string& filename, Mode mode, size_t size) {
    // Convert filename to wide string for Windows
    int wlen = MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, nullptr, 0);
    std::wstring wfilename(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, filename.c_str(), -1, &wfilename[0], wlen);
    
    // Open file
    DWORD access = 0;
    DWORD share = 0;
    DWORD creation = 0;
    DWORD flags = FILE_ATTRIBUTE_NORMAL;
    
    switch (mode) {
        case Mode::ReadOnly:
            access = GENERIC_READ;
            share = FILE_SHARE_READ;
            creation = OPEN_EXISTING;
            break;
        case Mode::ReadWrite:
            access = GENERIC_READ | GENERIC_WRITE;
            share = FILE_SHARE_READ;
            creation = OPEN_ALWAYS;
            break;
        case Mode::CreateNew:
            access = GENERIC_READ | GENERIC_WRITE;
            share = FILE_SHARE_READ;
            creation = CREATE_NEW;
            break;
    }
    
    file_handle_ = CreateFileW(wfilename.c_str(), access, share, nullptr,
                               creation, flags, nullptr);
    
    if (file_handle_ == INVALID_HANDLE_VALUE) {
        set_error("Failed to open file");
        return false;
    }
    
    // Get file size
    LARGE_INTEGER file_size;
    if (!GetFileSizeEx(file_handle_, &file_size)) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        set_error("Failed to get file size");
        return false;
    }
    
    // Set file size if specified
    if (size > 0 && mode != Mode::ReadOnly) {
        LARGE_INTEGER new_size;
        new_size.QuadPart = size;
        if (!SetFilePointerEx(file_handle_, new_size, nullptr, FILE_BEGIN) ||
            !SetEndOfFile(file_handle_)) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
            set_error("Failed to set file size");
            return false;
        }
        size_ = size;
    } else {
        size_ = static_cast<size_t>(file_size.QuadPart);
    }
    
    if (size_ == 0) {
        // Empty file, nothing to map
        return true;
    }
    
    // Create file mapping
    DWORD protect = (mode == Mode::ReadOnly) ? PAGE_READONLY : PAGE_READWRITE;
    mapping_handle_ = CreateFileMappingW(file_handle_, nullptr, protect,
                                        0, 0, nullptr);
    
    if (mapping_handle_ == nullptr) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
        set_error("Failed to create file mapping");
        return false;
    }
    
    // Map view of file
    DWORD map_access = (mode == Mode::ReadOnly) ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS;
    data_ = MapViewOfFile(mapping_handle_, map_access, 0, 0, size_);
    
    if (data_ == nullptr) {
        CloseHandle(mapping_handle_);
        CloseHandle(file_handle_);
        mapping_handle_ = INVALID_HANDLE_VALUE;
        file_handle_ = INVALID_HANDLE_VALUE;
        set_error("Failed to map view of file");
        return false;
    }
    
    return true;
}

void MemoryMappedFile::platform_close() {
    if (data_) {
        UnmapViewOfFile(data_);
    }
    if (mapping_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(mapping_handle_);
        mapping_handle_ = INVALID_HANDLE_VALUE;
    }
    if (file_handle_ != INVALID_HANDLE_VALUE) {
        CloseHandle(file_handle_);
        file_handle_ = INVALID_HANDLE_VALUE;
    }
}

bool MemoryMappedFile::platform_resize(size_t new_size) {
    // Set new file size
    LARGE_INTEGER li_size;
    li_size.QuadPart = new_size;
    if (!SetFilePointerEx(file_handle_, li_size, nullptr, FILE_BEGIN) ||
        !SetEndOfFile(file_handle_)) {
        set_error("Failed to resize file");
        return false;
    }
    
    // If current size is 0 or no mapping exists, create initial mapping
    if (size_ == 0 || data_ == nullptr) {
        // Create mapping
        mapping_handle_ = CreateFileMappingW(file_handle_, nullptr, PAGE_READWRITE,
                                            0, 0, nullptr);
        if (mapping_handle_ == nullptr) {
            set_error("Failed to create file mapping");
            return false;
        }
        
        // Map view
        data_ = MapViewOfFile(mapping_handle_, FILE_MAP_ALL_ACCESS, 0, 0, new_size);
        if (data_ == nullptr) {
            CloseHandle(mapping_handle_);
            mapping_handle_ = INVALID_HANDLE_VALUE;
            set_error("Failed to map view of file");
            return false;
        }
    } else {
        // Windows requires unmapping and remapping for resize
        UnmapViewOfFile(data_);
        CloseHandle(mapping_handle_);
        
        // Recreate mapping
        mapping_handle_ = CreateFileMappingW(file_handle_, nullptr, PAGE_READWRITE,
                                            0, 0, nullptr);
        if (mapping_handle_ == nullptr) {
            set_error("Failed to recreate file mapping");
            return false;
        }
        
        // Remap view
        data_ = MapViewOfFile(mapping_handle_, FILE_MAP_ALL_ACCESS, 0, 0, new_size);
        if (data_ == nullptr) {
            CloseHandle(mapping_handle_);
            mapping_handle_ = INVALID_HANDLE_VALUE;
            set_error("Failed to remap view of file");
            return false;
        }
    }
    
    size_ = new_size;
    return true;
}

bool MemoryMappedFile::platform_sync(SyncMode mode) {
    return FlushViewOfFile(data_, size_) != 0;
}

bool MemoryMappedFile::platform_advise(AccessPattern pattern, size_t offset, size_t length) {
    // Windows doesn't have direct equivalent to madvise
    // Could use PrefetchVirtualMemory on Windows 8+
    return true;
}

bool MemoryMappedFile::platform_lock(size_t offset, size_t length, bool lock) {
    void* addr = static_cast<char*>(data_) + offset;
    if (lock) {
        return VirtualLock(addr, length) != 0;
    } else {
        return VirtualUnlock(addr, length) != 0;
    }
}

#else  // POSIX implementation

bool MemoryMappedFile::platform_open(const std::string& filename, Mode mode, size_t size) {
    // Open file
    int flags = 0;
    mode_t file_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    
    switch (mode) {
        case Mode::ReadOnly:
            flags = O_RDONLY;
            break;
        case Mode::ReadWrite:
            flags = O_RDWR | O_CREAT;
            break;
        case Mode::CreateNew:
            flags = O_RDWR | O_CREAT | O_EXCL;
            break;
    }
    
    file_descriptor_ = ::open(filename.c_str(), flags, file_mode);
    if (file_descriptor_ < 0) {
        set_error("Failed to open file");
        return false;
    }
    
    // Get file size
    struct stat st;
    if (fstat(file_descriptor_, &st) < 0) {
        ::close(file_descriptor_);
        file_descriptor_ = -1;
        set_error("Failed to get file size");
        return false;
    }
    
    // Set file size if specified
    if (size > 0 && mode != Mode::ReadOnly) {
        if (ftruncate(file_descriptor_, size) < 0) {
            ::close(file_descriptor_);
            file_descriptor_ = -1;
            set_error("Failed to set file size");
            return false;
        }
        size_ = size;
    } else {
        size_ = st.st_size;
    }
    
    if (size_ == 0) {
        // Empty file, nothing to map
        return true;
    }
    
    // Map file
    int prot = (mode == Mode::ReadOnly) ? PROT_READ : (PROT_READ | PROT_WRITE);
    int flags_map = MAP_SHARED;
    
    data_ = mmap(nullptr, size_, prot, flags_map, file_descriptor_, 0);
    if (data_ == MAP_FAILED) {
        data_ = nullptr;
        ::close(file_descriptor_);
        file_descriptor_ = -1;
        set_error("Failed to map file");
        return false;
    }
    
    return true;
}

void MemoryMappedFile::platform_close() {
    if (data_) {
        munmap(data_, size_);
    }
    if (file_descriptor_ >= 0) {
        ::close(file_descriptor_);
        file_descriptor_ = -1;
    }
}

bool MemoryMappedFile::platform_resize(size_t new_size) {
    // Check file descriptor
    if (file_descriptor_ < 0) {
        set_error("Invalid file descriptor");
        return false;
    }
    
    // Resize file
    if (ftruncate(file_descriptor_, new_size) < 0) {
        set_error("Failed to resize file");
        return false;
    }
    
    // Remap if size changed
    if (new_size != size_) {
        // Special case: if current size is 0 (empty file), just map the new size
        if (size_ == 0 || data_ == nullptr) {
            data_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, file_descriptor_, 0);
            if (data_ == MAP_FAILED) {
                data_ = nullptr;
                set_error("Failed to map file");
                return false;
            }
        } else {
#ifdef __linux__
            // Use mremap on Linux
            void* new_data = mremap(data_, size_, new_size, MREMAP_MAYMOVE);
            if (new_data == MAP_FAILED) {
                // Fallback to munmap/mmap
                munmap(data_, size_);
                data_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, file_descriptor_, 0);
                if (data_ == MAP_FAILED) {
                    data_ = nullptr;
                    set_error("Failed to remap file");
                    return false;
                }
            } else {
                data_ = new_data;
            }
#else
            // Non-Linux systems: munmap and mmap again
            munmap(data_, size_);
            data_ = mmap(nullptr, new_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, file_descriptor_, 0);
            if (data_ == MAP_FAILED) {
                data_ = nullptr;
                set_error("Failed to remap file");
                return false;
            }
#endif
        }
        size_ = new_size;
    }
    
    return true;
}

bool MemoryMappedFile::platform_sync(SyncMode mode) {
    int flags = (mode == SyncMode::Sync) ? MS_SYNC : MS_ASYNC;
    return msync(data_, size_, flags) == 0;
}

bool MemoryMappedFile::platform_advise(AccessPattern pattern, size_t offset, size_t length) {
    int advice = MADV_NORMAL;
    
    switch (pattern) {
        case AccessPattern::Normal:
            advice = MADV_NORMAL;
            break;
        case AccessPattern::Sequential:
            advice = MADV_SEQUENTIAL;
            break;
        case AccessPattern::Random:
            advice = MADV_RANDOM;
            break;
        case AccessPattern::WillNeed:
            advice = MADV_WILLNEED;
            break;
        case AccessPattern::DontNeed:
            advice = MADV_DONTNEED;
            break;
    }
    
    void* addr = static_cast<char*>(data_) + offset;
    return madvise(addr, length, advice) == 0;
}

bool MemoryMappedFile::platform_lock(size_t offset, size_t length, bool lock) {
    void* addr = static_cast<char*>(data_) + offset;
    if (lock) {
        return mlock(addr, length) == 0;
    } else {
        return munlock(addr, length) == 0;
    }
}

#endif

// MemoryMappedArena implementation

bool MemoryMappedArena::open(const std::string& filename, bool create) {
    close();
    
    if (!file_.open(filename, create ? MemoryMappedFile::Mode::ReadWrite
                                    : MemoryMappedFile::Mode::ReadOnly)) {
        return false;
    }
    
    if (file_.size() < HEADER_SIZE) {
        if (!create) {
            file_.close();
            return false;
        }
        
        // Initialize new arena
        size_t initial_size = std::max(HEADER_SIZE + block_size_, size_t(4096));
        if (!file_.resize(initial_size)) {
            file_.close();
            return false;
        }
        
        header_ = file_.data_as<Header>();
        header_->magic = 0x4D4D4152;
        header_->version = 1;
        header_->total_size = initial_size;
        header_->used_size = HEADER_SIZE;
        header_->block_size = block_size_;
        header_->flags = 0;
        std::memset(header_->reserved, 0, sizeof(header_->reserved));
        header_->checksum = calculate_checksum(header_);
        
        file_.sync();
    } else {
        header_ = file_.data_as<Header>();
        if (!validate()) {
            file_.close();
            header_ = nullptr;
            return false;
        }
        block_size_ = header_->block_size;
    }
    
    return true;
}

void MemoryMappedArena::close() {
    if (header_) {
        header_->checksum = calculate_checksum(header_);
        file_.sync();
    }
    header_ = nullptr;
    file_.close();
}

void* MemoryMappedArena::allocate(size_t size, size_t alignment) {
    if (!header_ || size == 0) return nullptr;
    
    // Align current position
    size_t aligned_offset = align_up(header_->used_size, alignment);
    
    // Check if we need to grow
    if (aligned_offset + size > header_->total_size) {
        if (!grow(aligned_offset + size)) {
            return nullptr;
        }
        // Refresh header pointer after resize
        header_ = file_.data_as<Header>();
    }
    
    void* ptr = reinterpret_cast<char*>(header_) + aligned_offset;
    header_->used_size = aligned_offset + size;
    
    return ptr;
}

void MemoryMappedArena::reset() {
    if (header_) {
        header_->used_size = HEADER_SIZE;
        header_->checksum = calculate_checksum(header_);
    }
}

size_t MemoryMappedArena::get_used_size() const {
    return header_ ? header_->used_size - HEADER_SIZE : 0;
}

size_t MemoryMappedArena::get_total_size() const {
    return header_ ? header_->total_size - HEADER_SIZE : 0;
}

size_t MemoryMappedArena::get_available_size() const {
    return header_ ? header_->total_size - header_->used_size : 0;
}

bool MemoryMappedArena::validate() const {
    if (!header_) return false;
    
    return header_->magic == 0x4D4D4152 &&
           header_->version == 1 &&
           header_->used_size <= header_->total_size &&
           header_->checksum == calculate_checksum(header_);
}

bool MemoryMappedArena::grow(size_t min_size) {
    size_t new_size = header_->total_size;
    while (new_size < min_size) {
        new_size = std::max(new_size + block_size_, new_size * 2);
    }
    
    if (!file_.resize(new_size)) {
        return false;
    }
    
    header_ = file_.data_as<Header>();
    header_->total_size = new_size;
    header_->checksum = calculate_checksum(header_);
    
    return true;
}

uint32_t MemoryMappedArena::calculate_checksum(const Header* header) {
    // Simple XOR checksum excluding the checksum field itself
    uint32_t sum = 0;
    const uint32_t* data = reinterpret_cast<const uint32_t*>(header);
    size_t count = offsetof(Header, checksum) / sizeof(uint32_t);
    
    for (size_t i = 0; i < count; ++i) {
        sum ^= data[i];
    }
    
    return sum;
}

size_t MemoryMappedArena::align_up(size_t value, size_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

} // namespace memory
} // namespace claude_draw