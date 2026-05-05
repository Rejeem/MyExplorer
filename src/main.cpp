#include <iostream>
#include <chrono>
#include <iomanip>
#include "FileScanner.h"
#include "ThreadPool.h"
#include "MemoryPool.h"
#include "StringPool.h"

int main() {
    try {
        // 1. Resource configuration
        unsigned int numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 4;

        std::cout << "--- High-Performance File Discovery Engine ---" << std::endl;
        std::cout << "[System] Threads detected: " << numThreads << std::endl;
        std::cout << "[System] MemoryPool capacity: 10,000,000 nodes (" << (10000000 * sizeof(FileNode)) / (1024 * 1024) << " MB)" << std::endl;
        std::cout << "[System] StringPool capacity: 1 GB (Virtual Memory)" << std::endl;
        
        // 2. Initialize components
        ThreadPool pool(numThreads);
        MemoryPool& nodePool = MemoryPool::GetInstance();
        FileScanner scanner(pool, nodePool);

        // 3. Define the path to scan
        std::string pathBuffer;
        std::cout << "\nEnter the path to scan (e.g., C:/Windows): ";
        std::getline(std::cin, pathBuffer);
        
        if (pathBuffer.empty()) pathBuffer = "C:/Windows";

        std::cout << "\n[Action] Starting scan on: " << pathBuffer << "..." << std::endl;

        // 4. Start timer and scan
        auto start = std::chrono::high_resolution_clock::now();
        
        scanner.scan(pathBuffer);

        // 5. Wait for work completion (active wait)
        // Monitor active task count in the pool
        while (pool.getActiveTasks() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;

        // 6. Display benchmark results
        size_t totalNodes = nodePool.count();
        uint32_t totalErrors = scanner.getErrorCount();
        uint32_t totalStringsSize = StringPool::GetInstance().getCurrentOffset();

        std::cout << "\n------------------ RESULTS ------------------" << std::endl;
        std::cout << "[Done] Scan completed in: " << std::fixed << std::setprecision(3) << elapsed.count() << " seconds" << std::endl;
        std::cout << "[Stat] Total Files/Folders found: " << totalNodes << std::endl;
        std::cout << "[Stat] Strings Data size: " << totalStringsSize / 1024 << " KB" << std::endl;
        std::cout << "[Stat] Access Errors: " << totalErrors << std::endl;
        
        if (elapsed.count() > 0) {
            double speed = static_cast<double>(totalNodes) / elapsed.count();
            std::cout << "[Perf] Processing speed: " << static_cast<uint64_t>(speed) << " nodes/sec" << std::endl;
        }
        std::cout << "---------------------------------------------" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "\n[Critical Error] " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}