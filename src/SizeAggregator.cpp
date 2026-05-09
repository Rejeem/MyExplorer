#include "SizeAggregator.h"
#include "MemoryPool.h"
#include "FileNode.h"

void SizeAggregator::compute() {
    MemoryPool& pool = MemoryPool::GetInstance();
    int totalNodes = static_cast<int>(pool.count());
    
    if (totalNodes <= 1) return;

    for (int i = totalNodes - 1; i > 0; --i) {
        FileNode* node = pool.get(i);
        
        if (node) {
            FileNode* parent = pool.get(node->parentId);
            
            if (parent && parent != node) {
                parent->fileSize += node->fileSize;
            }
        }
    }
}