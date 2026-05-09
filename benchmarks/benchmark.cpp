#include "FileScanner.h"
#include <iostream>

void run_test(const std::string& name, const std::string& path, unsigned int threads) {
    ThreadPool pool(threads);
    MemoryPool& nodePool = MemoryPool::GetInstance();
    nodePool.reset();
    StringPool::GetInstance().reset();
    
    FileScanner scanner(pool, nodePool);
    
    auto start = std::chrono::high_resolution_clock::now();
    scanner.scan(path);
    
    while (pool.getActiveTasks() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    
    std::cout << "| " << std::setw(15) << name 
          << " | " << std::setw(10) << threads << " threads | " 
          << std::setw(12) << nodePool.count() << " nodes | "
          << std::setw(10) << diff.count() << "s |" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string testPath;

    // Check if path was passed as a command-line argument
    if (argc > 1) {
        testPath = argv[1];
    } else {
        // Interactive mode: Ask the user for the path
        std::cout << "Please enter the directory path to benchmark (or '.' for current): ";
        std::getline(std::cin, testPath);

        if (testPath.empty()) {
            testPath = ".";
        }
    }
    
    run_test("Single", testPath, 1);
    run_test("Dual", testPath, 2);
    run_test("Quad", testPath, 4);
    run_test("Max", testPath, std::thread::hardware_concurrency());
    
    std::cout << "----------------------------------------------------------" << std::endl;
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();
    return 0;
}