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
MemoryPool &MemoryPool::GetInstance()
{
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
MemoryPool::MemoryPool()
{
    size_t total_reserve_size = max_nodes_ * sizeof(FileNode);

    base_ptr_ = static_cast<FileNode *>(
        VirtualAlloc(
            NULL,
            total_reserve_size,
            MEM_RESERVE,
            PAGE_READWRITE));

    if (!base_ptr_)
    {
        throw std::runtime_error("MemoryPool: Failed to RESERVE virtual memory space.");
    }

    size_t initial_commit_size = 4 * 1024 * 1024; // 4 MB
    void *check = VirtualAlloc(base_ptr_, initial_commit_size, MEM_COMMIT, PAGE_READWRITE);
    if (!check)
    {
        VirtualFree(base_ptr_, 0, MEM_RELEASE);
        throw std::runtime_error("MemoryPool: Failed to COMMIT initial memory chunk.");
    }

    committed_nodes_.store(initial_commit_size / sizeof(FileNode));
}

/**
 * @brief Destructor: Releases the virtual memory arena back to the OS.
 */
MemoryPool::~MemoryPool()
{
    if (base_ptr_)
    {
        VirtualFree(base_ptr_, 0, MEM_RELEASE);
    }
}

/**
 * @brief Allocates the next available slot dynamically committing memory as needed.
 * * Thread-safe, lock-free index acquisition, with a localized mutex guard only
 * when allocating new physical RAM pages.
 */
FileNode *MemoryPool::allocate()
{
    size_t index = next_available_index_.fetch_add(1);

    if (index >= max_nodes_)
    {
        throw std::runtime_error("MemoryPool: Capacity exceeded (10M nodes limit)");
    }

    if (index >= committed_nodes_.load(std::memory_order_relaxed))
    {

        std::lock_guard<std::mutex> lock(commit_mutex_);

        if (index >= committed_nodes_.load(std::memory_order_relaxed))
        {

            size_t chunk_size = 4 * 1024 * 1024;
            size_t current_committed = committed_nodes_.load();

            void *commit_target = static_cast<char *>(static_cast<void *>(base_ptr_)) + (current_committed * sizeof(FileNode));

            if (!VirtualAlloc(commit_target, chunk_size, MEM_COMMIT, PAGE_READWRITE))
            {
                throw std::runtime_error("MemoryPool: Failed to COMMIT additional memory chunk on the fly.");
            }

            committed_nodes_.fetch_add(chunk_size / sizeof(FileNode));
        }
    }

    FileNode *node = &base_ptr_[index];
    node->nodeId = static_cast<uint32_t>(index);

    return node;
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
void MemoryPool::deallocate(size_t index)
{
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
void MemoryPool::reset()
{
    // Only reset the atomic counter to make all slots available again.
    next_available_index_.store(0);
}
