#pragma once

#include <cstdint>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <atomic>
#include "FileNode.h"

/**
 * @class MemoryPool
 * @brief Singleton arena allocator for fixed-size FileNode objects.
 * 
 * Implements a high-performance memory pool using a contiguous std::vector to guarantee
 * optimal cache locality, zero heap fragmentation, and O(1) allocation time.
 * Ideal for large-scale file system traversal with millions of nodes.
 * 
 * @note This class is a singleton; use GetInstance() to access the global pool.
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
     * Resets the internal allocation counter without deallocating the underlying memory.
     * This preserves capacity for subsequent scan cycles while allowing reuse of slots.
     */
    void reset();

    /**
     * @brief Gets the total number of currently allocated nodes.
     * @return size_t The current count of allocated nodes.
     */
    size_t count() const { return next_available_index_.load(); }

    /**
     * @brief Retrieves a pointer to a node by its index (inline for performance).
     * 
     * @param index The node index within the pool.
     * @return FileNode* Pointer to the node, or nullptr if index is out of range.
     */
    inline FileNode* get(size_t index) {
    if (index >= next_available_index_.load(std::memory_order_relaxed)) {
        return nullptr;
    }
    return &nodes_[index];
    }
    
    /**
     * @brief Gets the maximum capacity of the pool.
     * @return size_t The hard limit of allocated slots.
     */
    size_t getCapacity() const { return nodes_.capacity(); }

    /**
     * @brief Gets the current count of allocated nodes (alias for count()).
     * @return uint32_t The number of nodes currently allocated.
     */
    uint32_t getAllocatedCount() const {
    return next_available_index_.load(); 
    }

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