#pragma once
#include <string>
#include <atomic>
#include <cstdint>

/**
 * @class StringPool
 * @brief Manages a contiguous pool of memory for storing unique strings.
 * 
 * The StringPool implements a global, thread-safe string buffer. All stored strings 
 * reside in a single, managed memory region (the "Virtual Memory"). This architecture 
 * maximizes cache locality and facilitates highly performant string handling, which 
 * is crucial for low-level file system indexing and resource management operations.
 * 
 * @note This pool ensures thread-safety for all write operations and provides 
 * predictable memory access patterns.
 */
class StringPool {
public:
    /**
     * @brief Accessor method to get the singleton instance of the pool.
     * @return StringPool& The singleton instance.
     */
    static StringPool& GetInstance();
    
    // Disable copy constructor and assignment operator to enforce singleton pattern
    StringPool(const StringPool&) = delete;
    StringPool& operator=(const StringPool&) = delete;

    /**
     * @brief Atomically reserves space and copies the string into the pool buffer.
     * 
     * This operation is thread-safe. It reserves the required memory at the current 
     * offset, ensuring the string is correctly stored at a unique location within 
     * the pool's managed memory.
     * 
     * @param str The string content to be stored.
     * @return uint32_t The starting offset (index) of the stored string within the pool buffer.
     * @throws std::runtime_error if the pool memory is full.
     */
    uint32_t getOffset(const std::string& str);
    
    /**
     * @brief Resets the pool's current offset, making all allocated slots logically available.
     * 
     * This action resets the pool's write cursor. It does not deallocate the underlying 
     * reserved memory, which maintains high performance across multiple scan cycles 
     * by preventing expensive system memory allocations/deallocations.
     */
    void reset();
    
    /**
     * @brief Retrieves the base address of the underlying memory buffer.
     * @return const char* A raw pointer to the start of the allocated virtual memory block.
     */
    const char* getBuffer() const { return buffer_ptr_; }

    /**
     * @brief Returns the current write offset, indicating the next available memory position.
     * @return uint32_t The current offset in bytes.
     */
    uint32_t getCurrentOffset() const { return current_offset_.load(); }
    
private:
    /**
     * @brief Private constructor enforcing singleton usage. Initializes the memory buffer.
     */
    StringPool(); 
    
    /**
     * @brief Private destructor. Cleans up the allocated memory buffer.
     */
    ~StringPool();
    
    // The raw pointer to the start of the allocated memory chunk.
    char* buffer_ptr_ = nullptr;
    
    // Atomic counter tracking the current write offset (the next free byte).
    std::atomic<uint32_t> current_offset_{0};
    
    // The total size of the reserved memory pool in bytes.
    size_t committed_size_ = 0;
    
    // The fixed total size of the pool (1 GB).
    const size_t reserved_size_ = 1024 * 1024 * 1024; 
};