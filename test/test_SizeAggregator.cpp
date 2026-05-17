#include <iostream>
#include <cassert>
#include "MemoryPool.h"
#include "SizeAggregator.h"
#include "FileNode.h"

/**
 * Manually builds a tree in the MemoryPool:
 * Root (ID 1)
 * ├── File_A (ID 2, size: 500)
 * └── Folder_B (ID 3)
 * ├── File_C (ID 4, size: 300)
 * └── File_D (ID 5, size: 200)
 * * Total expected: Folder_B = 500, Root = 1000
 */
void test_logic() {
    MemoryPool& pool = MemoryPool::GetInstance();
    pool.reset();

    FileNode* root = pool.allocate(); 
    root->nodeId = 0; 
    root->parentId = 0xFFFFFFFF;

    FileNode* fileA = pool.allocate();
    fileA->nodeId = 1;
    fileA->parentId = 0; 
    fileA->fileSize = 500;

    FileNode* folderB = pool.allocate();
    folderB->nodeId = 2;
    folderB->parentId = 0; 
    folderB->fileSize = 0;

    FileNode* fileC = pool.allocate();
    fileC->nodeId = 3;
    fileC->parentId = 2; 
    fileC->fileSize = 300;

    FileNode* fileD = pool.allocate();
    fileD->nodeId = 4;
    fileD->parentId = 2; 
    fileD->fileSize = 200;

    std::cout << "[Step 1] Tree built in MemoryPool. Running compute()..." << std::endl;

    // Run the aggregator
    SizeAggregator::compute();

    uint64_t sizeB = pool.get(2)->fileSize; 

    uint64_t sizeRoot = pool.get(0)->fileSize; 

    std::cout << "[Step 2] Results -> Folder_B: " << sizeB << " bytes, Root: " << sizeRoot << " bytes" << std::endl;

    assert(sizeB == 500 && "Folder_B should be 300 + 200");
    assert(sizeRoot == 1000 && "Root should be File_A (500) + Folder_B (500)");

    std::cout << ">>> TEST SUCCESSFUL! <<<" << std::endl;
}

int main() {
    test_logic();
    return 0;
}