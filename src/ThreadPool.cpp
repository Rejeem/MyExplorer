#include "ThreadPool.h"
#include <iostream>

// --- ThreadPool Constructor ---
ThreadPool::ThreadPool(unsigned int threads) {
    // Explicitly initialize atomics for added safety
    stop.store(false);
    active_task_count.store(0);

    for (unsigned int i = 0; i < threads; ++i) {
        workers.emplace_back(&ThreadPool::worker_loop, this);
    }
    std::cout << "[ThreadPool] Created " << threads << " worker threads." << std::endl;
}

// --- ThreadPool Destructor ---
ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();

    for (std::thread& worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    std::cout << "[ThreadPool] All worker threads stopped gracefully." << std::endl;
}

// --- Worker Loop Implementation ---
void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            
            // Wait until there is a task or the pool stops
            condition.wait(lock, [this] {
                return stop || !tasks.empty();
            });

            // Exit condition: stop enabled and no tasks left
            if (stop && tasks.empty()) {
                return;
            }
            
            // Retrieve the task
            task = std::move(tasks.front());
            tasks.pop();
            
            // IMPORTANT: Do NOT increment active_task_count here.
            // It was already incremented by enqueue() in the header
            
        } // Mutex is automatically released

        try {
            // Execute the scan or task
            if (task) {
                task();
            }
        } catch (const std::exception& e) {
            std::cerr << "[ThreadPool Error] Exception caught: " << e.what() << std::endl;
        }
        
        // Decrement once here because the job is complete
        active_task_count--;
    }
}
