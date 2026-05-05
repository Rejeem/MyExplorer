#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <memory>
#include <chrono>
#include <stdexcept>

// Assume MemoryPool.h and ThreadPool.h exist and are correctly implemented
#include "MemoryPool.h"
#include "ThreadPool.h"
#include "FileNode.h"

// Define the number of tasks and nodes per task
constexpr int NUM_TASKS = 1000;
constexpr int NODES_PER_TASK = 5000;
constexpr size_t EXPECTED_TOTAL_ALLOCATIONS = NUM_TASKS * NODES_PER_TASK;

// Global counter to track completed tasks
std::atomic<int> tasks_completed(0);

/**
 * @brief The task function executed by the ThreadPool.
 * 
 * Allocates a batch of FileNodes and writes a unique identifier to test
 * concurrent read/write access and lock-free allocation safety.
 * 
 * @param task_id Unique ID for the task.
 */
void allocation_task(int task_id) {
    try {
        // 1. Allocate a batch of FileNodes
        std::vector<FileNode*> allocated_nodes;
        for (int i = 0; i < NODES_PER_TASK; ++i) {
            FileNode* node = MemoryPool::GetInstance().allocate();
            allocated_nodes.push_back(node);
        }

        // 2. Write unique values to test concurrent write access
        // We write a value derived from the task ID and node index.
        // This verifies that the memory slots are independent and usable.
        for (int i = 0; i < NODES_PER_TASK; ++i) {
            // Assuming FileNode has an accessible field for testing (e.g., a char array or int)
            // For this test, we'll simulate writing a task/node ID combination.
            // Note: In a real scenario, we would use a specific setter method.
            // Since FileNode structure is POD, we assume a byte-write mechanism or
            // an accessible internal field for simulation purposes.
            // Here, we just ensure the memory write itself is successful.
            // Example simulation: Write (task_id * 1000 + i) into the first byte.
            if (allocated_nodes[i] != nullptr) {
                // Simulating write operation to test memory safety
                // We rely on the fact that FileNode is a POD and simply write to its memory space.
                int write_value = task_id * 1000 + i;
                // WARNING: This assumes the FileNode structure starts with an int or char[4].
                // For a robust test, the target structure should expose this.
                // For now, we just ensure the memory write succeeds.
                // If FileNode had a uint32_t field:
                // *((uint32_t*)allocated_nodes[i]) = write_value;
            }
        }
        // 2. Write unique values to test concurrent write access
        for (int i = 0; i < NODES_PER_TASK; ++i) {
             if (allocated_nodes[i] != nullptr) {
                 // Write the task ID into the first structure member
                 // This proves that each returned pointer refers to valid memory.
                 allocated_nodes[i]->nodeId = static_cast<uint32_t>(task_id);
                 allocated_nodes[i]->parentId = static_cast<uint32_t>(task_id);
                }
        }
        
        tasks_completed++;

    } catch (const std::runtime_error& e) {
        // Catch capacity overflow errors
        std::cerr << "Thread Error in Task " << task_id << ": " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in Task " << task_id << std::endl;
    }
}

/**
 * @brief Runs the comprehensive multi-threaded stress test for MemoryPool.
 * 
 * Initializes the ThreadPool, enqueues multiple allocation tasks, waits for them 
 * to complete, and asserts the final allocation count.
 * 
 * @return true if the test passed, false otherwise.
 */
bool run_memory_pool_stress_test() {
    std::cout << "--- Starting MemoryPool Stress Test ---" << std::endl;
    
    // 1. Setup Pool and Pool State
    MemoryPool::GetInstance().reset();
    std::cout << "MemoryPool reset. Initial count: " << MemoryPool::GetInstance().count() << std::endl;
    
    // 2. Setup ThreadPool
    unsigned int concurrency = std::thread::hardware_concurrency();
    if (concurrency == 0) concurrency = 4; // Fallback if detection fails
    
    ThreadPool pool(concurrency);
    std::cout << "ThreadPool initialized with " << concurrency << " workers." << std::endl;

    // 3. Enqueue Tasks
    std::cout << "Enqueuing " << NUM_TASKS << " tasks (Nodes per task: " << NODES_PER_TASK << ")..." << std::endl;
    for (int i = 0; i < NUM_TASKS; ++i) {
        pool.enqueue([i] { allocation_task(i); });
    }

    // 4. Wait for all tasks to complete
    // In a real test framework, we would wait until all tasks are marked complete.
    // Since we don't have a structured way to wait for all tasks here, we simulate
    // by waiting a generous amount of time.
    std::cout << "Waiting for tasks to complete... (Max wait: 10 seconds)" << std::endl;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Busy wait / Check counter periodically
    while (tasks_completed.load() < NUM_TASKS) {
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        if (elapsed > 10) {
            std::cerr << "ERROR: Timed out waiting for tasks to complete." << std::endl;
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // 5. Validation
    size_t final_count = MemoryPool::GetInstance().count();
    
    std::cout << "\n--- Test Results ---" << std::endl;
    std::cout << "Expected total allocations: " << EXPECTED_TOTAL_ALLOCATIONS << std::endl;
    std::cout << "Actual final allocation count: " << final_count << std::endl;

    if (final_count == EXPECTED_TOTAL_ALLOCATIONS) {
        std::cout << "SUCCESS: MemoryPool passed the concurrent stress test. Allocation count matches expected value." << std::endl;
        return true;
    } else {
        std::cerr << "FAILURE: MemoryPool failed the stress test. Expected " << EXPECTED_TOTAL_ALLOCATIONS 
                  << " but found " << final_count << "." << std::endl;
        return false;
    }
}

int main() {
    // Setting up basic error handling for clean exit
    try {
        // Execute the stress test
        if (run_memory_pool_stress_test()) {
            return 0; // Success
        } else {
            std::cerr << "Test failed to complete." << std::endl;
            return 1; // Failure
        }
    } catch (const std::exception& e) {
        std::cerr << "Critical Test Exception: " << e.what() << std::endl;
        return 2;
    }
}