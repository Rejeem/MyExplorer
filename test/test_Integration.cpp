#include <iostream>
#include <cassert>
#include <cstring>
#include <string>
#include "StringPool.h"
#include "MemoryPool.h"
#include "FileNode.h"
#include <filesystem>
namespace fs = std::filesystem;

int main() {
    std::cout << "--- Starting Integration Test ---" << std::endl;

    // 1. Get Instances and Reset
    StringPool& sp = StringPool::GetInstance();
    MemoryPool& mp = MemoryPool::GetInstance();
    sp.reset();
    mp.reset();

    // 2. Test Data
    fs::path currentFile = __FILE__;
    std::string testPath = "C:/Windows/System32/kernel32.dll";
    assert(!testPath.empty());
    // 3. Store string and get offset
    uint32_t offset = sp.getOffset(testPath);
    std::cout << "[StringPool] Stored path at offset: " << offset << std::endl;

    // 4. Allocate Node from MemoryPool
    FileNode* node = mp.allocate();
    if (!node) {
        std::cerr << "Failed to allocate FileNode" << std::endl;
        return 1;
    }

    // 5. Link the two pools
    node->nodeId = 101;
    node->nameOffset = offset;
    std::cout << "[MemoryPool] Allocated node ID: " << node->nodeId << std::endl;

    // 6. Verification: Manual pointer arithmetic
    const char* retrievedName = sp.getBuffer() + node->nameOffset;
    
    std::cout << "[Verify] Original:  " << testPath << std::endl;
    std::cout << "[Verify] Retrieved: " << retrievedName << std::endl;

    if (testPath == retrievedName) {
        std::cout << "\n>>> SUCCESS: MemoryPool and StringPool are correctly linked! <<<" << std::endl;
    } else {
        std::cerr << "\n>>> FAILURE: Data mismatch! <<<" << std::endl;
        return 1;
    }

    return 0;
}