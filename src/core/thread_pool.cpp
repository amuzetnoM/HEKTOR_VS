// ============================================================================
// VectorDB - Thread Pool Implementation
// ============================================================================

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>

namespace vdb {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = 0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 4;
        }
        
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { worker_loop(); });
        }
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
    
    // Non-copyable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    
    /// Submit a task and get a future for the result
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<ReturnType> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (stop_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        
        cv_.notify_one();
        return result;
    }
    
    /// Execute function in parallel over range [0, count)
    template<typename F>
    void parallel_for(size_t count, F&& func) {
        if (count == 0) return;
        
        size_t num_threads = workers_.size();
        size_t chunk_size = (count + num_threads - 1) / num_threads;
        
        std::vector<std::future<void>> futures;
        futures.reserve(num_threads);
        
        for (size_t t = 0; t < num_threads; ++t) {
            size_t start = t * chunk_size;
            size_t end = std::min(start + chunk_size, count);
            
            if (start >= count) break;
            
            futures.push_back(submit([&func, start, end]() {
                for (size_t i = start; i < end; ++i) {
                    func(i);
                }
            }));
        }
        
        for (auto& f : futures) {
            f.get();
        }
    }
    
    /// Get number of threads
    [[nodiscard]] size_t size() const { return workers_.size(); }
    
    /// Get pending task count
    [[nodiscard]] size_t pending() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return tasks_.size();
    }
    
    /// Wait for all tasks to complete
    void wait_all() {
        std::unique_lock<std::mutex> lock(mutex_);
        done_cv_.wait(lock, [this] {
            return tasks_.empty() && active_tasks_ == 0;
        });
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                
                if (stop_ && tasks_.empty()) {
                    return;
                }
                
                task = std::move(tasks_.front());
                tasks_.pop();
                ++active_tasks_;
            }
            
            task();
            
            {
                std::unique_lock<std::mutex> lock(mutex_);
                --active_tasks_;
                if (tasks_.empty() && active_tasks_ == 0) {
                    done_cv_.notify_all();
                }
            }
        }
    }
    
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable done_cv_;
    
    std::atomic<size_t> active_tasks_{0};
    bool stop_ = false;
};

// Global thread pool for parallel operations
inline ThreadPool& global_thread_pool() {
    static ThreadPool pool;
    return pool;
}

} // namespace vdb
