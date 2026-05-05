#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <stdexcept>
#include <memory>
#include <type_traits>

/**
 * @class ThreadPool
 * @brief High-performance Thread Pool designed for intensive I/O and file scanning.
 * 
 * This pool manages a fixed set of worker threads and a task queue. It includes
 * specialized tracking of active tasks to allow the main thread to monitor 
 * scan progress in real-time.
 */
class ThreadPool {
public:
    /**
     * @brief Initializes the pool with a specific number of workers.
     * @param threads Number of concurrent worker threads to spawn.
     */
    explicit ThreadPool(unsigned int threads);
    
    /**
     * @brief Signal all threads to stop and joins them.
     */
    ~ThreadPool();
    
    // Disable copying for thread safety and resource ownership
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Enqueues a new task for asynchronous execution.
     * 
     * Uses modern C++ type traits to deduce return types. Increments the 
     * active task counter immediately to prevent premature "idle" detection.
     * 
     * @tparam F Function type.
     * @tparam Args Argument types.
     * @param f Function to execute.
     * @param args Arguments for the function.
     * @return std::future associated with the task result.
     */
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<std::invoke_result_t<F, Args...>>;

    /**
     * @brief Atomic access to the current workload count.
     * @return Number of tasks currently running or queued.
     */
    unsigned int getActiveTasks() const { return active_task_count.load(); }

    /**
     * @brief thread-safe check of the queue size.
     * 
     * Used by the FileScanner back-pressure mechanism to decide between 
     * async and sync execution.
     * 
     * @return Current number of pending tasks in the queue.
     */
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queue_mutex));
        return tasks.size();
    }
    
    /**
     * @brief Internal method called by worker threads to decrement task count.
     * Should only be used internally or by specialized task wrappers.
     */
    void decrementActiveTasks() { active_task_count--; }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queue_mutex;
    std::condition_variable condition;
    
    std::atomic<bool> stop{false};
    std::atomic<unsigned int> active_task_count{0};

    void worker_loop();
};

/**
 * @brief Template implementation for task enqueuing.
 */
template<typename F, typename... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) 
    -> std::future<std::invoke_result_t<F, Args...>> {
    
    using return_type = std::invoke_result_t<F, Args...>;
    
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> res = task->get_future();
    
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        
        if (stop) {
            throw std::runtime_error("ThreadPool stopped: cannot enqueue new tasks.");
        }
        
        // Atomic increment before queuing
        active_task_count++; 
        
        tasks.emplace([task]() { (*task)(); });
    }
    
    condition.notify_one();
    return res;
}