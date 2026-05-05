#include "FileNode.h"
#include <iostream>
#include <cstdint>
#include <cstddef>

/**
 * @brief Unit test to verify the memory layout of the FileNode structure.
 * 
 * This test confirms that the manual padding and member alignment in FileNode.h
 * successfully maintain a fixed size of 32 bytes. 
 * Note: On 64-bit systems, uint32_t is 4 bytes, and we aim for a power-of-two 
 * size to optimize CPU cache line usage.
 */
void testFileNodeSize() {
    // sizeof() returns the total size of the structure in bytes.
    size_t nodeSize = sizeof(FileNode);
    
    std::cout << "--- Test: FileNode Memory Layout ---\n";
    std::cout << "FileNode struct size (calculated): " << nodeSize << " bytes.\n";

    if (nodeSize == 32) {
        std::cout << "SUCCESS: Size is exactly 32 bytes. Cache locality is preserved.\n";
    } else {
        std::cout << "FAILURE: Size is " << nodeSize << " bytes. Padding adjustment required.\n";
    }
}

int main() {
    testFileNodeSize();
    return 0;
}