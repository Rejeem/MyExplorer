#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#include "FileNode.h" // Assuming FileNode.h exists and defines FileNode

/**
 * @class MemoryPool
 * @brief Implements a Memory Pool (Arena/Linear Allocator) for fixed-size objects.
 * 
 * This pool is highly optimized for allocating fixed-size structures like FileNode. 
 * By utilizing a contiguous std::vector<FileNode>, it guarantees that all objects 
 * are stored back-to-back in memory. This architecture successfully eliminates 
 * heap fragmentation, maximizes cache locality, and allows for O(1) allocation 
 * time, making it ideal for high-performance virtual file system scanning 
 * and deep data structure indexing.
 */
class MemoryPool {
public:
    /**
     * @brief Provides the singleton instance of the MemoryPool.
     * @return MemoryPool& Reference to the single instance.
     */
    static MemoryPool& GetInstance();
    
    // Delete copy constructor and assignment operator
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    /**
     * @brief Allocates the next available slot (O(1) time complexity).
     * @return FileNode* A pointer to the newly allocated, uninitialized slot.
     * @throws std::runtime_error if the pool capacity is exceeded.
     */
    FileNode* allocate();

    /**
     * @brief Conceptually deallocates the slot at the given index (No-op).
     * 
     * In a high-performance Arena Allocator, individual deallocation is forbidden 
     * or highly inefficient. This function is a no-op to maintain O(1) allocation 
     * time complexity and ensure memory stability, only resetting the whole pool 
     * upon completion of a scanning cycle.
     * @param index The index of the slot to conceptually 'deallocate'.
     */
    void deallocate(size_t index);

    /**
     * @brief Resets the entire pool, making all slots available again.
     * 
     * This efficiently resets the pool's logical state by resetting the index counter.
     * It preserves the total allocated memory capacity, thus guaranteeing O(1) 
     * allocation for subsequent scan cycles.
     */
    void reset();

    /**
     * @brief Gets the total number of currently active slots.
     * @return size_t The current count of allocated nodes.
     */
    size_t count() const { return next_available_index_.load(); }

    /**
     * @brief Gets the maximum capacity of the pool.
     * @return size_t The hard limit of the allocated memory.
     */
    size_t getCapacity() const { return nodes_.capacity(); }

private:
    /**
     * @brief Private Constructor. Initializes the memory arena.
     * 
     * Reserves a fixed, large block of memory upfront to meet anticipated dataset 
     * size requirements. This is crucial for preventing runtime overheads associated 
     * with dynamic memory growth during intensive I/O operations.
     */
    MemoryPool();
    
    /**
     * @brief Virtual destructor.
     */
    ~MemoryPool() = default;
    
    // The contiguous storage for all FileNode objects. Using std::vector<T> 
    // ensures perfect fixed-size alignment and excellent cache performance.
    std::vector<FileNode> nodes_; 
    // Tracks the index of the next available slot for thread-safe allocation.
    std::atomic<size_t> next_available_index_{0};
};