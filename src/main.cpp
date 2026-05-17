#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdlib>
#include "FileScanner.h"
#include "ThreadPool.h"
#include "MemoryPool.h"
#include "StringPool.h"
#include "SizeAggregator.h"

// Size formatting utility
std::string formatSize(uint64_t size) {
    double s = static_cast<double>(size);
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    while (s >= 1024 && i < 4) {
        s /= 1024;
        i++;
    }
    char buffer[32];
    sprintf(buffer, "%.2f %s", s, units[i]);
    return std::string(buffer);
}

void printProgressBar(uint64_t currentSize, uint64_t totalSize) {
    int width = 20;
    double ratio = (totalSize > 0) ? (static_cast<double>(currentSize) / totalSize) : 0;
    int filled = static_cast<int>(ratio * width);

    std::cout << "[";
    for (int i = 0; i < width; ++i) {
        if (i < filled) std::cout << "#";
        else std::cout << "_";
    }
    std::cout << "] " << std::fixed << std::setprecision(1) << (ratio * 100) << "%";
}

int main() {
    try {
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;

        std::cout << "--- MyExplorer: High-Performance Navigator ---\n";
        
        ThreadPool pool(numThreads);
        MemoryPool& nodePool = MemoryPool::GetInstance();
        StringPool& sPool = StringPool::GetInstance();
        FileScanner scanner(pool, nodePool);

        nodePool.allocate(); 

        std::string pathBuffer;
        std::cout << "Enter the path to scan (Default: C:/): ";
        std::getline(std::cin, pathBuffer);
        if (pathBuffer.empty()) pathBuffer = "C:/";

        FileNode* rootNode = nodePool.allocate(); 
        uint32_t rootIdx = rootNode->nodeId;
        
        rootNode->nameOffset = sPool.getOffset(pathBuffer);
        rootNode->parentId = 0;
        rootNode->fileFlags = 1; // Directory
        rootNode->firstChildIndex = 0;
        rootNode->siblingIndex = 0;
        rootNode->fileSize = 0;

        auto start = std::chrono::high_resolution_clock::now();
        scanner.scanDirectory(pathBuffer, rootIdx, rootNode);

        const char spinner[] = {'|', '/', '-', '\\'};
        int spinnerIdx = 0;
        std::cout << "\n";

        while (pool.getActiveTasks() > 0) {
            uint32_t count = nodePool.count();
            std::cout << "\r[Status] " << spinner[spinnerIdx] << " Nodes found: " << count << " (scanning...)" << std::flush;
            spinnerIdx = (spinnerIdx + 1) % 4;
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }

        SizeAggregator::compute();

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "\n [System] Scan completed in " << elapsed.count() << "s\n";

        uint32_t currentIdx = rootIdx; 
        std::string input;
        bool showAll = false; 

        while (true) {
            FileNode* current = nodePool.get(currentIdx);
            if (!current) break;

            std::cout << "\n----------------------------------------------------------\n";
            std::cout << " LOCATION: " << sPool.get(current->nameOffset) << "\n";
            std::cout << " TOTAL SIZE: " << formatSize(current->fileSize) << "\n";
            std::cout << " FILTER: " << (showAll ? "OFF (Showing everything)" : "ON (Grouping < 100KB)") << "\n";
            std::cout << "----------------------------------------------------------\n";

            std::vector<uint32_t> children;
            uint32_t childIdx = current->firstChildIndex;
            while (childIdx != 0) {
                children.push_back(childIdx);
                childIdx = nodePool.get(childIdx)->siblingIndex;
            }

            std::sort(children.begin(), children.end(), [&](uint32_t a, uint32_t b) {
                return nodePool.get(a)->fileSize > nodePool.get(b)->fileSize;
            });

            uint64_t smallFilesTotalSize = 0;
            int smallFilesCount = 0;
            const uint64_t THRESHOLD = 100 * 1024; // 100 KB

            // Display
            for (uint32_t idx : children) {
                FileNode* child = nodePool.get(idx);
                bool isDir = (child->fileFlags == 1);

                if (isDir || child->fileSize > THRESHOLD || showAll) {
                    std::string name = sPool.get(child->nameOffset);
                    std::cout << std::setw(10) << (isDir ? "[DIR]" : "[FILE]") << " "
                              << std::setw(30) << (name.length() > 27 ? name.substr(0, 27) + "..." : name) << " "
                              << std::setw(12) << formatSize(child->fileSize) << " ";
                    
                    printProgressBar(child->fileSize, current->fileSize);
                    std::cout << "\n";
                } else {
                    smallFilesTotalSize += child->fileSize;
                    smallFilesCount++;
                }
            }

            if (!showAll && smallFilesCount > 0) {
                std::cout << std::setw(10) << "[MISC]" << " "
                          << std::setw(30) << (std::to_string(smallFilesCount) + " small files grouped...") << " "
                          << std::setw(12) << formatSize(smallFilesTotalSize) << " ";
                
                printProgressBar(smallFilesTotalSize, current->fileSize);
                std::cout << "\n";
            }

            if (showAll != true) std::cout << "\nCommands: [cd name] [cd ..] [show] [exit]\n> ";
            else std::cout << "\nCommands: [cd name] [cd ..] [hide] [exit]\n> ";
            
            std::getline(std::cin, input);

            if (input == "exit") break;
            
            if (input == "show" || input == "hide") {
                showAll = !showAll;
            } 
            else if (input == "cd ..") {
                if (current->parentId != 0) {
                    currentIdx = current->parentId;
                } 
            } 
            else if (input.substr(0, 3) == "cd ") {
                std::string target = input.substr(3);
                bool found = false;
                for (uint32_t idx : children) {
                    FileNode* child = nodePool.get(idx);
                    if (child->fileFlags == 1 && sPool.get(child->nameOffset) == target) {
                        currentIdx = idx;
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cout << "Directory not found. Press Enter...";
                    std::string dummy;
                    std::getline(std::cin, dummy);
                }
            }
            system("cls");
        }
    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
    }
    return 0;
}