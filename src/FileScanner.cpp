#include "FileScanner.h"
#include "StringPool.h"
#include <filesystem>
#include <atomic>
#include <vector>

namespace fs = std::filesystem;

/**
 * @brief Constructor for FileScanner.
 * 
 * Initializes the scanner with references to the required system pools: 
 * the ThreadPool for concurrent task execution and the MemoryPool for node allocation.
 * These dependencies are crucial for managing high-performance, multi-threaded 
 * filesystem scanning processes.
 * 
 * @param pool Reference to the global ThreadPool managing task distribution.
 * @param node_pool Reference to the MemoryPool responsible for allocating FileNode structures.
 * 
 * @details Initializes the error counter to zero.
 */
FileScanner::FileScanner(ThreadPool& pool, MemoryPool& node_pool) 
    : thread_pool_(pool), node_pool_(node_pool), error_count_(0) {}

/**
 * @brief Entry point for the filesystem scan.
 * 
 * Resets the internal I/O error counter and enqueues the initial root directory scan 
 * into the ThreadPool. This kickstarts the recursive traversal process across the entire 
 * filesystem structure starting from the specified root path.
 * 
 * @param rootPath The absolute starting filesystem path for the discovery process.
 */
void FileScanner::scan(const std::string& rootPath) {
    error_count_.store(0);
    
    // Start initial scan with a null parent for the root
    thread_pool_.enqueue([this, rootPath] {
        this->scanDirectory(rootPath, 0, nullptr);
    });
}

/**
 * @brief Recursively scans a directory and builds the LCRS tree structure.
 * 
 * This method implements the core logic of the scanner engine:
 * 1. **Node Allocation:** Allocates a new FileNode from the MemoryPool.
 * 2. **String Storage:** Stores the filename using the StringPool for optimized, 
 *    cache-friendly indexing (Virtual Memory principle).
 * 3. **LCRS Linking:** Manages the Left-Child Right-Sibling (LCRS) pointers 
 *    (firstChildIndex and siblingIndex) to maintain a linked list structure 
 *    for efficient traversal.
 * 4. **Concurrency Management:** Uses the ThreadPool for asynchronous scanning 
 *    (if queue pressure is low) or falls back to synchronous scanning (back-pressure) 
 *    to prevent memory overcommit when system resources are strained.
 * 
 * @param path The absolute path of the directory currently being processed.
 * @param parentId The unique node ID of the parent directory.
 * @param parentNode Pointer to the parent FileNode used for establishing the 
 *                   first-child link (LCRS structure).
 * 
 * @note The LCRS model is essential for linear memory access, supporting 
 *       the overall high-performance goal of the Virtual Memory system.
 */
void FileScanner::scanDirectory(const std::string& path, uint32_t parentId, FileNode* parentNode) {
    FileNode* previousSibling = nullptr;
    uint32_t childCount = 0;

    try {
        fs::directory_options options = fs::directory_options::skip_permission_denied;
        
        for (const auto& entry : fs::directory_iterator(path, options)) {
            // 1. Allocation
            FileNode* node = node_pool_.allocate(); 
            uint32_t nodeIndex = node->nodeId; 
            
            // 2. Basic configuration
            node->parentId = parentId;
            node->nameOffset = StringPool::GetInstance().getOffset(entry.path().filename().string());
            node->siblingIndex = 0;
            node->firstChildIndex = 0;
            node->fileSize = 0; // Default to 0

            // 3. LCRS logic (parent/sibling links)
            if (childCount == 0) {
                if (parentNode) {
                    parentNode->firstChildIndex = nodeIndex;
                }
            } else if (previousSibling) {
                previousSibling->siblingIndex = nodeIndex;
            }
            previousSibling = node;

            // 4. Directory / file distinction
            if (entry.is_directory()) {
                node->fileFlags = 1; // Directory flag
                std::string subPath = entry.path().string();

                // ThreadPool pressure management (back-pressure)
                if (thread_pool_.getQueueSize() > 10000) {
                    this->scanDirectory(subPath, nodeIndex, node);
                } else {
                    thread_pool_.enqueue([this, subPath, nodeIndex, node] {
                        this->scanDirectory(subPath, nodeIndex, node);
                    });
                }
            } 
            else if (entry.is_regular_file()) {
                node->fileFlags = 2; // File flag
                // Directly store the actual file size
                try {
                    node->fileSize = entry.file_size();
                } catch (...) {
                    node->fileSize = 0; // Safety in case the file disappears during scan
                }
            }
            
            childCount++;
        }
    } catch (...) {
        error_count_++;
    }
}