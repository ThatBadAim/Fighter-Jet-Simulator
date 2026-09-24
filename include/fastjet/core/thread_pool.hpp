#pragma once

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace fastjet::core {

/// @brief Fixed pool of worker threads for CPU-bound jobs.
///
/// Two ways in: submit() runs one job and hands back a future, and
/// parallel_for() splits an index range across the workers *and* the calling
/// thread. The caller always works through chunks itself, so parallel_for
/// finishes even when every worker is busy (or when it is called from a
/// worker), and can never deadlock on the pool.
///
/// Rendering stays on the thread that owns the GL context; jobs here must
/// not make GL calls.
class ThreadPool {
public:
    /// @param workers Worker threads to start (the caller of parallel_for
    ///                makes one more)
    explicit ThreadPool(unsigned workers) {
        threads_.reserve(workers);
        for (unsigned i = 0; i < workers; ++i) {
            threads_.emplace_back([this] { worker_loop(); });
        }
    }

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (std::thread& t : threads_) t.join();
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /// @brief Process-wide pool: one worker per hardware thread beyond the
    /// caller's, and at least one so background jobs always make progress.
    static ThreadPool& shared() {
        static ThreadPool pool(default_workers());
        return pool;
    }

    static unsigned default_workers() noexcept {
        const unsigned hw = std::thread::hardware_concurrency();
        return hw > 1 ? hw - 1 : 1;
    }

    [[nodiscard]] unsigned worker_count() const noexcept { return static_cast<unsigned>(threads_.size()); }

    /// @brief Runs `fn` on a worker; the future carries its result or exception.
    template <class F>
    auto submit(F&& fn) -> std::future<std::invoke_result_t<std::decay_t<F>>> {
        using R = std::invoke_result_t<std::decay_t<F>>;
        auto task = std::make_shared<std::packaged_task<R()>>(std::forward<F>(fn));
        std::future<R> result = task->get_future();
        enqueue([task] { (*task)(); });
        return result;
    }

    /// @brief Calls body(i) for every i in [begin, end), spread over the pool
    /// and the calling thread, and returns once all have run.
    ///
    /// Iterations must be independent. Work is handed out in chunks of
    /// `grain` indices from a shared counter, so uneven iterations balance
    /// themselves.
    template <class F>
    void parallel_for(int begin, int end, F&& body, int grain = 1) {
        if (end <= begin) return;
        grain = std::max(grain, 1);
        const int total = end - begin;
        const int chunks = (total + grain - 1) / grain;
        if (chunks == 1 || threads_.empty()) {
            for (int i = begin; i < end; ++i) body(i);
            return;
        }

        struct Shared {
            std::atomic<int> next;
            std::atomic<int> remaining;
            std::mutex m;
            std::condition_variable cv;
        };
        auto shared = std::make_shared<Shared>();
        shared->next.store(begin);
        shared->remaining.store(total);

        // Helpers reach `body` through a pointer; they only dereference it
        // while an index is still unclaimed, which cannot outlive this call.
        auto* fn = &body;
        auto run = [shared, fn, end, grain] {
            for (;;) {
                const int i0 = shared->next.fetch_add(grain);
                if (i0 >= end) return;
                const int i1 = std::min(i0 + grain, end);
                for (int i = i0; i < i1; ++i) (*fn)(i);
                if (shared->remaining.fetch_sub(i1 - i0) == i1 - i0) {
                    std::lock_guard<std::mutex> lock(shared->m);
                    shared->cv.notify_all();
                }
            }
        };

        const int helpers = std::min<int>(static_cast<int>(threads_.size()), chunks - 1);
        for (int h = 0; h < helpers; ++h) enqueue(run);
        run();

        std::unique_lock<std::mutex> lock(shared->m);
        shared->cv.wait(lock, [&] { return shared->remaining.load() == 0; });
    }

private:
    void enqueue(std::function<void()> job) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push_back(std::move(job));
        }
        cv_.notify_one();
    }

    void worker_loop() {
        for (;;) {
            std::function<void()> job;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this] { return stop_ || !queue_.empty(); });
                if (stop_ && queue_.empty()) return;
                job = std::move(queue_.front());
                queue_.pop_front();
            }
            job();
        }
    }

    std::vector<std::thread> threads_;
    std::deque<std::function<void()>> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_ = false;
};

} // namespace fastjet::core
