#include "StringPool.h"
#include <windows.h>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <mutex>

/**
 * @brief Get the singleton instance of the StringPool.
 * @return StringPool& A reference to the single, globally managed StringPool instance.
 */
StringPool& StringPool::GetInstance() {
    static StringPool instance;
    return instance;
}

/**
 * @brief Constructor: Initializes the StringPool by reserving virtual memory.
 * 
 * Reserves a large block of virtual memory (1 GB) using VirtualAlloc(..., MEM_RESERVE, ...).
 * This step reserves the address space without allocating physical RAM, which is crucial
 * for guaranteeing memory address availability for the entire dataset.
 * 
 * @throws std::runtime_error If the virtual memory reservation fails.
 */
StringPool::StringPool() {
    buffer_ptr_ = (char*)VirtualAlloc(NULL, reserved_size_, MEM_RESERVE, PAGE_NOACCESS);
    if (!buffer_ptr_) {
        throw std::runtime_error("Failed to reserve virtual memory for StringPool");
    }
}

/**
 * @brief Destructor: Releases the reserved virtual memory.
 * 
 * Uses VirtualFree to release the entire reserved address space allocated in the
 * constructor, preventing memory leaks at the process level.
 */
StringPool::~StringPool() {
    if (buffer_ptr_) {
        VirtualFree(buffer_ptr_, 0, MEM_RELEASE);
    }
}

/**
 * @brief Reserves space and copies the string into the pool buffer in a thread-safe manner.
 * 
 * This method atomically reserves the required space using current_offset_.fetch_add(),
 * simulating a fast allocation process characteristic of a high-performance
 * memory pool. It also manages the physical memory commit in fixed blocks (1MB alignment)
 * to optimize performance and cache usage, mimicking virtual memory management.
 * 
 * @param str The string to be stored in the pool.
 * @return uint32_t The starting offset of the stored string within the pool buffer.
 * @throws std::runtime_error If physical memory commit fails.
 */
uint32_t StringPool::getOffset(const std::string& str) {
    uint32_t sizeNeeded = static_cast<uint32_t>(str.size() + 1);
    
    // Atomic reservation of space for this thread/call
    uint32_t oldOffset = current_offset_.fetch_add(sizeNeeded);
    
    // Physical memory commit management block (1MB chunks)
    uint32_t endOffset = oldOffset + sizeNeeded;
    if (endOffset > committed_size_) {
        static std::mutex commit_mutex;
        std::lock_guard<std::mutex> lock(commit_mutex);
        
        // Double check after lock acquisition
        if (endOffset > committed_size_) {
            size_t newCommitSize = (size_t)std::max((uint32_t)(committed_size_ + 1024 * 1024), endOffset);
            // Align to 64KB (Windows allocation granularity)
            newCommitSize = (newCommitSize + 0xFFFF) & ~0xFFFF;
            
            if (VirtualAlloc(buffer_ptr_, newCommitSize, MEM_COMMIT, PAGE_READWRITE)) {
                committed_size_ = newCommitSize;
            } else {
                throw std::runtime_error("Failed to commit physical memory for StringPool");
            }
        }
    }
    
    // Thread-safe copy, as each thread writes only to its reserved zone
    std::memcpy(buffer_ptr_ + oldOffset, str.c_str(), sizeNeeded);
    return oldOffset;
}

/**
 * @brief Resets the pool's offset pointer.
 * 
 * This function resets the atomic offset counter to zero, making all slots logically 
 * available for immediate reuse. The previously committed physical memory remains 
 * allocated, optimizing performance and avoiding costly reallocations during subsequent scan cycles.
 */
void StringPool::reset() {
    // Reset the offset, but keep the committed memory for performance
    current_offset_.store(0);
}
