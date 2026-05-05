#pragma once

#include <string>
#include <atomic>
#include <cstdint>
#include <memory>
#include <filesystem>
#include "ThreadPool.h"
#include "MemoryPool.h"
#include "StringPool.h"

/**
 * @class FileScanner
 * @brief Orchestrates a multi-threaded, recursive file system discovery engine.
 * 
 * FileScanner manages the high-speed traversal of a file system structure. It coordinates
 * concurrent tasks using the ThreadPool, maintains node persistence via the MemoryPool, 
 * and utilizes the StringPool for optimized, cache-friendly name storage. The directory 
 * structure is represented and traversed using the Left-Child Right-Sibling (LCRS) 
 * tree model, ensuring high performance suitable for Virtual Memory operations.
 */
class FileScanner {
public:
    /**
     * @brief Initializes the scanner with required system pools.
     * 
     * Establishes dependencies on the global ThreadPool and MemoryPool instances
     * necessary for executing concurrent scan tasks and persisting node data.
     * 
     * @param pool Reference to the global ThreadPool managing task distribution.
     * @param node_pool Reference to the MemoryPool responsible for allocating FileNode structures.
     */
    FileScanner(ThreadPool& pool, MemoryPool& node_pool);
    
    /**
     * @brief Deleted copy constructor and assignment operator.
     * 
     * Prevents unauthorized copying, ensuring the singleton-like management 
     * of system resources and thread pools.
     */
    FileScanner(const FileScanner&) = delete;
    FileScanner& operator=(const FileScanner&) = delete;

    /**
     * @brief Entry point for the file system scanning process.
     * 
     * Resets internal state and enqueues the initial directory scanning task 
     * into the ThreadPool. This initiates the recursive traversal from the root path.
     * 
     * @param rootPath The absolute starting filesystem path for the discovery process.
     */
    void scan(const std::string& rootPath);

    /**
     * @brief Retrieves the total count of I/O errors encountered during scanning.
     * 
     * Includes errors such as 'Access Denied' or file not found errors, 
     * allowing for robust error reporting in high-performance environments.
     * @return uint32_t The current total count of non-critical I/O errors.
     */
    uint32_t getErrorCount() const { return error_count_.load(); }
    
private:
    ThreadPool& thread_pool_;   ///< Reference to the thread pool for asynchronous task execution.
    MemoryPool& node_pool_;     ///< Reference to the memory pool for persistent FileNode allocation.
    
    /**
     * @brief Internal recursive function for directory traversal.
     * 
     * This core function performs directory iteration, manages LCRS linking 
     * (first-child/next-sibling pointers), creates new FileNode instances, and 
     * submits new sub-directory scanning tasks to the ThreadPool. 
     * It includes a back-pressure mechanism to fall back to synchronous scanning 
     * if the asynchronous task queue is saturated.
     * 
     * @param path The absolute path of the directory currently being processed.
     * @param parentId The unique node ID of the parent directory.
     * @param parentNode Pointer to the parent FileNode used for establishing the 
     *                   first-child link (LCRS structure).
     */
    void scanDirectory(const std::string& path, uint32_t parentId, FileNode* parentNode);

    /**
     * @brief Atomic counter tracking the total number of I/O errors encountered.
     * 
     * Ensures thread-safe reporting of scan failures.
     */
    std::atomic<uint32_t> error_count_{0}; 
};