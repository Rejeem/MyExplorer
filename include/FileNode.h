#pragma once

#include <cstdint>

/**
 * @struct FileNode
 * @brief Represents a single node in the file system tree structure.
 * 
 * This structure is designed as a Plain Old Data (POD) type to ensure 
 * optimal cache locality and predictable memory overhead within a contiguous 
 * memory pool. Manual padding is used to guarantee a fixed size of 32 bytes, 
 * which is critical for scaling to massive datasets of nodes.
 * 
 * The design prioritizes memory efficiency and speed, making it suitable for 
 * high-performance virtual file system indexing.
 * 
 * @note Target size: 32 bytes.
 */
struct FileNode {
    /**
     * @brief Unique identifier/index of the node within the memory pool.
     * 
     * This ID serves as the primary reference key for the node.
     */
    uint32_t nodeId; 
    
    /**
     * @brief Parent link ID.
     * 
     * Stores the ID of the parent node. A value of 0 typically represents 
     * the root directory of the entire file system.
     */
    uint32_t parentId; 
    
    /**
     * @brief Index of the first direct child node.
     * 
     * Points to the ID of the first node in this directory's child list.
     * If no children exist, this value is expected to be 0.
     */
    uint32_t firstChildIndex; 
    
    /**
     * @brief Index of the next sibling node.
     * 
     * Points to the ID of the next node at the same level in the hierarchy.
     * A value of 0 indicates that this is the last sibling.
     */
    uint32_t siblingIndex; 
    
    /**
     * @brief Depth level of this node in the file system hierarchy.
     * 
     * Used for calculating indentation or determining relative depth. The root 
     * node typically has a depth of 0.
     */
    uint32_t depthLevel; 
    
    /**
     * @brief File type and attribute flags.
     * 
     * A bitmask containing various status indicators (e.g., bit 0=Directory, 
     * bit 1=Regular File, bit 2=Symlink). This allows for quick runtime type checking.
     */
    uint32_t fileFlags; 
    
    /**
     * @brief Offset to the node's name.
     * 
     * This ID points to the starting byte offset of the node's name within 
     * the StringPool's underlying buffer, saving memory.
     */
    uint32_t nameOffset;

    /**
     * @brief Reserved padding field.
     * 
     * Used to ensure that the structure maintains a consistent size (32 bytes) 
     * for memory alignment and future feature expansion.
     */
    uint32_t reserved2;
};