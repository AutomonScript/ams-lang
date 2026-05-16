#pragma once

#include "error.hpp"
#include <string>
#include <vector>
#include <fstream>
#include <memory>
#include <chrono>
#include <sstream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
// Undefine Windows macros that conflict with our enums
#ifdef ERROR
#undef ERROR
#endif
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace ams {
namespace stdlib {
namespace source {

// Access mode for log files
enum class AccessMode {
    READ,
    WRITE
};

// LOG_FILE manages physical file access and buffering
class LOG_FILE {
public:
    // Factory method to open a log file
    static Result<std::unique_ptr<LOG_FILE>> open(
        const std::string& filepath,
        AccessMode mode
    ) {
        auto file = std::unique_ptr<LOG_FILE>(new LOG_FILE(filepath, mode));
        
        if (mode == AccessMode::READ) {
            auto result = file->initializeRead();
            if (result.isErr()) {
                return Result<std::unique_ptr<LOG_FILE>>::Err(
                    result.getError(),
                    result.getErrorMessage()
                );
            }
        } else {
            auto result = file->initializeWrite();
            if (result.isErr()) {
                return Result<std::unique_ptr<LOG_FILE>>::Err(
                    result.getError(),
                    result.getErrorMessage()
                );
            }
        }
        
        return Result<std::unique_ptr<LOG_FILE>>::Ok(std::move(file));
    }
    
    ~LOG_FILE() {
        cleanup();
    }
    
    // Read operations
    Result<std::string> readAll() const {
        if (mode_ != AccessMode::READ) {
            return Result<std::string>::Err(
                LogSourceError::INVALID_MODE,
                "Cannot read from file opened in WRITE mode"
            );
        }
        
        if (use_mmap_) {
            // Use memory-mapped I/O
            #ifdef _WIN32
            if (mmap_view_ == nullptr) {
                return Result<std::string>::Err(
                    LogSourceError::IO_ERROR,
                    "Memory-mapped view is null"
                );
            }
            return Result<std::string>::Ok(
                std::string(static_cast<const char*>(mmap_view_), file_size_)
            );
            #else
            if (mmap_addr_ == nullptr || mmap_addr_ == MAP_FAILED) {
                return Result<std::string>::Err(
                    LogSourceError::IO_ERROR,
                    "Memory-mapped address is invalid"
                );
            }
            return Result<std::string>::Ok(
                std::string(static_cast<const char*>(mmap_addr_), file_size_)
            );
            #endif
        } else {
            // Use buffered I/O
            std::ifstream file(filepath_, std::ios::binary);
            if (!file) {
                return Result<std::string>::Err(
                    LogSourceError::IO_ERROR,
                    "Failed to open file for reading: " + filepath_
                );
            }
            
            std::string content((std::istreambuf_iterator<char>(file)),
                               std::istreambuf_iterator<char>());
            return Result<std::string>::Ok(std::move(content));
        }
    }
    
    Result<std::string> readFrom(size_t position) const {
        auto all_result = readAll();
        if (all_result.isErr()) {
            return all_result;
        }
        
        std::string content = all_result.unwrap();
        if (position >= content.size()) {
            return Result<std::string>::Ok(std::string());
        }
        
        return Result<std::string>::Ok(content.substr(position));
    }
    
    Result<std::vector<std::string>> readLines() const {
        auto content_result = readAll();
        if (content_result.isErr()) {
            return Result<std::vector<std::string>>::Err(
                content_result.getError(),
                content_result.getErrorMessage()
            );
        }
        
        std::string content = content_result.unwrap();
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        return Result<std::vector<std::string>>::Ok(std::move(lines));
    }
    
    Result<std::vector<std::string>> readLinesFrom(size_t position) const {
        auto content_result = readFrom(position);
        if (content_result.isErr()) {
            return Result<std::vector<std::string>>::Err(
                content_result.getError(),
                content_result.getErrorMessage()
            );
        }
        
        std::string content = content_result.unwrap();
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        
        return Result<std::vector<std::string>>::Ok(std::move(lines));
    }
    
    // Write operations
    Result<void> append(const std::string& content) {
        if (mode_ != AccessMode::WRITE) {
            return Result<void>::Err(
                LogSourceError::WRITE_NOT_PERMITTED,
                "Cannot write to file opened in READ mode"
            );
        }
        
        if (!write_stream_ || !write_stream_->is_open()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "Write stream is not open"
            );
        }
        
        (*write_stream_) << content;
        
        if (write_stream_->fail()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "Failed to write to file: " + filepath_
            );
        }
        
        return Result<void>::Ok();
    }
    
    Result<void> flush() {
        if (mode_ != AccessMode::WRITE) {
            return Result<void>::Err(
                LogSourceError::INVALID_MODE,
                "Cannot flush file opened in READ mode"
            );
        }
        
        if (!write_stream_ || !write_stream_->is_open()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "Write stream is not open"
            );
        }
        
        write_stream_->flush();
        
        if (write_stream_->fail()) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "Failed to flush file: " + filepath_
            );
        }
        
        return Result<void>::Ok();
    }
    
    // File metadata
    size_t getSize() const {
        return file_size_;
    }
    
    std::chrono::system_clock::time_point getLastModified() const {
        return last_modified_;
    }
    
    bool hasChanged() const {
        try {
            namespace fs = std::filesystem;
            
            if (!fs::exists(filepath_)) {
                return false;
            }
            
            auto current_size = fs::file_size(filepath_);
            auto current_time = fs::last_write_time(filepath_);
            
            // Check size first (most reliable indicator)
            if (current_size != file_size_) {
                return true;
            }
            
            // For time comparison, use the raw file_time_type to avoid conversion issues
            // Store the last file_time and compare directly
            return current_time != cached_file_time_;
        } catch (...) {
            return false;
        }
    }
    
    // Update cached file metadata (call after detecting changes)
    void updateMetadata() {
        try {
            namespace fs = std::filesystem;
            
            if (fs::exists(filepath_)) {
                file_size_ = fs::file_size(filepath_);
                cached_file_time_ = fs::last_write_time(filepath_);
                
                // Also update last_modified_ for compatibility
                auto ftime = cached_file_time_;
                last_modified_ = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
            }
        } catch (...) {
            // Ignore errors
        }
    }
    
    // Position tracking
    size_t getCurrentPosition() const {
        return current_position_;
    }
    
    void setPosition(size_t pos) {
        current_position_ = pos;
    }
    
    // Get filepath
    std::string getFilePath() const {
        return filepath_;
    }
    
private:
    LOG_FILE(const std::string& filepath, AccessMode mode)
        : filepath_(filepath)
        , mode_(mode)
        , file_size_(0)
        , current_position_(0)
        , use_mmap_(false)
        #ifdef _WIN32
        , file_handle_(INVALID_HANDLE_VALUE)
        , mmap_handle_(NULL)
        , mmap_view_(nullptr)
        #else
        , fd_(-1)
        , mmap_addr_(nullptr)
        #endif
    {
    }
    
    Result<void> initializeRead() {
        namespace fs = std::filesystem;
        
        // Check if file exists
        if (!fs::exists(filepath_)) {
            return Result<void>::Err(
                LogSourceError::FILE_NOT_FOUND,
                "File not found: " + filepath_
            );
        }
        
        try {
            file_size_ = fs::file_size(filepath_);
            auto ftime = fs::last_write_time(filepath_);
            cached_file_time_ = ftime;
            
            // Convert file_time to system_clock time
            last_modified_ = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
            );
        } catch (const std::exception& e) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "Failed to get file metadata: " + std::string(e.what())
            );
        }
        
        // Use memory-mapped I/O for files > 1MB
        if (file_size_ > 1024 * 1024) {
            use_mmap_ = true;
            
            #ifdef _WIN32
            // Windows memory-mapped I/O
            file_handle_ = CreateFileA(
                filepath_.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                NULL
            );
            
            if (file_handle_ == INVALID_HANDLE_VALUE) {
                return Result<void>::Err(
                    LogSourceError::PERMISSION_DENIED,
                    "Failed to open file: " + filepath_
                );
            }
            
            mmap_handle_ = CreateFileMappingA(
                file_handle_,
                NULL,
                PAGE_READONLY,
                0,
                0,
                NULL
            );
            
            if (mmap_handle_ == NULL) {
                CloseHandle(file_handle_);
                file_handle_ = INVALID_HANDLE_VALUE;
                return Result<void>::Err(
                    LogSourceError::IO_ERROR,
                    "Failed to create file mapping: " + filepath_
                );
            }
            
            mmap_view_ = MapViewOfFile(
                mmap_handle_,
                FILE_MAP_READ,
                0,
                0,
                0
            );
            
            if (mmap_view_ == nullptr) {
                CloseHandle(mmap_handle_);
                CloseHandle(file_handle_);
                mmap_handle_ = NULL;
                file_handle_ = INVALID_HANDLE_VALUE;
                return Result<void>::Err(
                    LogSourceError::IO_ERROR,
                    "Failed to map view of file: " + filepath_
                );
            }
            #else
            // POSIX memory-mapped I/O
            fd_ = ::open(filepath_.c_str(), O_RDONLY);
            if (fd_ == -1) {
                return Result<void>::Err(
                    LogSourceError::PERMISSION_DENIED,
                    "Failed to open file: " + filepath_
                );
            }
            
            mmap_addr_ = ::mmap(nullptr, file_size_, PROT_READ, MAP_PRIVATE, fd_, 0);
            if (mmap_addr_ == MAP_FAILED) {
                ::close(fd_);
                fd_ = -1;
                return Result<void>::Err(
                    LogSourceError::IO_ERROR,
                    "Failed to memory-map file: " + filepath_
                );
            }
            #endif
        }
        
        return Result<void>::Ok();
    }
    
    Result<void> initializeWrite() {
        namespace fs = std::filesystem;
        
        // Create parent directories if they don't exist
        try {
            auto parent = fs::path(filepath_).parent_path();
            if (!parent.empty() && !fs::exists(parent)) {
                fs::create_directories(parent);
            }
        } catch (const std::exception& e) {
            return Result<void>::Err(
                LogSourceError::IO_ERROR,
                "Failed to create parent directories: " + std::string(e.what())
            );
        }
        
        // Open file in append mode
        write_stream_ = std::make_unique<std::ofstream>(
            filepath_,
            std::ios::app | std::ios::binary
        );
        
        if (!write_stream_->is_open()) {
            return Result<void>::Err(
                LogSourceError::PERMISSION_DENIED,
                "Failed to open file for writing: " + filepath_
            );
        }
        
        // Update file metadata
        try {
            if (fs::exists(filepath_)) {
                file_size_ = fs::file_size(filepath_);
                auto ftime = fs::last_write_time(filepath_);
                cached_file_time_ = ftime;
                last_modified_ = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
            }
        } catch (...) {
            // Ignore metadata errors for new files
        }
        
        return Result<void>::Ok();
    }
    
    void cleanup() {
        #ifdef _WIN32
        if (mmap_view_ != nullptr) {
            UnmapViewOfFile(mmap_view_);
            mmap_view_ = nullptr;
        }
        if (mmap_handle_ != NULL) {
            CloseHandle(mmap_handle_);
            mmap_handle_ = NULL;
        }
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
        }
        #else
        if (mmap_addr_ != nullptr && mmap_addr_ != MAP_FAILED) {
            ::munmap(mmap_addr_, file_size_);
            mmap_addr_ = nullptr;
        }
        if (fd_ != -1) {
            ::close(fd_);
            fd_ = -1;
        }
        #endif
        
        if (write_stream_ && write_stream_->is_open()) {
            write_stream_->close();
        }
    }
    
    std::string filepath_;
    AccessMode mode_;
    size_t file_size_;
    size_t current_position_;
    bool use_mmap_;
    std::unique_ptr<std::ofstream> write_stream_;
    mutable std::chrono::system_clock::time_point last_modified_;
    mutable std::filesystem::file_time_type cached_file_time_;
    
    #ifdef _WIN32
    HANDLE file_handle_;
    HANDLE mmap_handle_;
    LPVOID mmap_view_;
    #else
    int fd_;
    void* mmap_addr_;
    #endif
};

} // namespace source
} // namespace stdlib
} // namespace ams
