#include "MemoryPool.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

/**
 * @brief Implements the Singleton pattern for MemoryPool.
 * 
 * Ensures that only one instance of the MemoryPool exists, managing the single, 
 * contiguous memory arena for all FileNode objects. This pattern is crucial 
 * for global resource management in the CoreEngine.
 */
MemoryPool& MemoryPool::GetInstance() {
    static MemoryPool instance;
    return instance;
}

/**
 * @brief Constructor: Pre-allocates the memory arena.
 * 
 * Reserves a massive block of memory (10 million slots) upfront to meet the 
 * target dataset size. This prevents performance-heavy memory reallocations 
 * (re-sizing) during the initial FileScanner phase, ensuring O(1) allocation 
 * time complexity and maintaining cache locality.
 */
MemoryPool::MemoryPool() {
    // Mandatory pre-allocation step for high-performance, large-scale datasets.
    // Target capacity: 10,000,000 nodes.
    nodes_.resize(10000000); 
}

/**
 * @brief Allocates the next available slot (O(1) time complexity).
 * 
 * Uses emplace_back() for safe and efficient slot allocation. This method 
 * increments the vector's size and returns a pointer to the newly constructed 
 * object.
 * 
 * @return FileNode* A pointer to the newly allocated, uninitialized slot.
 * @throws std::runtime_error if the pre-allocated capacity was exceeded (though unlikely).
 */
FileNode* MemoryPool::allocate() {
    // Use atomic fetch_add for thread-safe, lock-free index acquisition.
    size_t index = next_available_index_.fetch_add(1); 
    if (index >= 10000000) {
        throw std::runtime_error("MemoryPool: Capacity exceeded (10M nodes limit)");
    }
    // Return pointer to existing slot, no construction or size change required.
    FileNode* node = &nodes_[index];
    // Initialize the ID here so it is immediately valid
    node->nodeId = static_cast<uint32_t>(index);
    return &nodes_[index]; 
}

/**
 * @brief Deallocates the slot at the given index (No-op in Arena Allocator).
 * 
 * In a high-performance linear/arena allocator, individual deallocation is 
 * forbidden or highly inefficient. This is a no-op to guarantee O(1) 
 * performance and maximize cache utilization, only resetting the whole pool 
 * when the scan is complete.
 * 
 * @param index The index of the slot to conceptually 'deallocate'.
 */
void MemoryPool::deallocate(size_t index) {
    // No-op: Individual deallocation is not supported in this pattern.
    (void)index; // Suppresses unused parameter warning
}

/**
 * @brief Resets the entire pool, making all slots available again.
 * 
 * This efficiently resets the pool's logical state by clearing the content 
 * but preserving the total allocated memory capacity, thus avoiding future 
 * large memory reallocations between scan cycles.
 */
void MemoryPool::reset() {
    // Only reset the atomic counter to make all slots available again.
    next_available_index_.store(0);
}
