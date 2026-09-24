/**
 * @file test_thread_pool.cpp
 * @brief Worker pool: parallel_for coverage, nesting, futures, and that the
 *        parallel terrain bake matches a serial one exactly.
 */
#include "fastjet/core/thread_pool.hpp"
#include "fastjet/graphics/terrain_field.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

using fastjet::core::ThreadPool;

void test_parallel_for_covers_range() {
    std::cout << "[Test] parallel_for: every index exactly once... " << std::flush;
    ThreadPool pool(3);
    for (int grain : {1, 7, 64, 5000}) {
        std::vector<std::atomic<int>> hits(4096);
        pool.parallel_for(0, 4096, [&](int i) { hits[static_cast<size_t>(i)].fetch_add(1); }, grain);
        for (const auto& h : hits) assert(h.load() == 1);
    }
    // Empty and offset ranges.
    int calls = 0;
    pool.parallel_for(5, 5, [&](int) { ++calls; });
    assert(calls == 0);
    std::atomic<long> sum{0};
    pool.parallel_for(100, 200, [&](int i) { sum += i; });
    assert(sum.load() == (100 + 199) * 100 / 2);
    std::cout << "PASS\n";
}

void test_parallel_for_uses_workers() {
    std::cout << "[Test] parallel_for: work spreads across threads... " << std::flush;
    ThreadPool pool(3);
    std::atomic<int> running{0};
    std::atomic<int> peak{0};
    pool.parallel_for(0, 8, [&](int) {
        const int now = ++running;
        int p = peak.load();
        while (now > p && !peak.compare_exchange_weak(p, now)) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        --running;
    });
    assert(peak.load() >= 2);
    std::cout << "PASS (peak concurrency " << peak.load() << ")\n";
}

void test_nested_and_busy_pool() {
    std::cout << "[Test] parallel_for: nested and with a saturated pool, no deadlock... " << std::flush;
    ThreadPool pool(2);
    // Occupy every worker; parallel_for must still finish on the caller.
    std::atomic<bool> release{false};
    std::vector<std::future<void>> blockers;
    for (unsigned i = 0; i < pool.worker_count(); ++i) {
        blockers.push_back(pool.submit([&] { while (!release.load()) std::this_thread::yield(); }));
    }
    std::atomic<int> n{0};
    pool.parallel_for(0, 100, [&](int) { ++n; });
    assert(n.load() == 100);
    release = true;
    for (auto& b : blockers) b.get();

    // A parallel_for inside a submitted job (as ModelGLB::prepare does).
    auto outer = pool.submit([&] {
        std::atomic<int> inner{0};
        pool.parallel_for(0, 1000, [&](int) { ++inner; });
        return inner.load();
    });
    assert(outer.get() == 1000);
    std::cout << "PASS\n";
}

void test_submit_future() {
    std::cout << "[Test] submit: results and exceptions reach the future... " << std::flush;
    ThreadPool pool(2);
    auto a = pool.submit([] { return 6 * 7; });
    assert(a.get() == 42);
    auto b = pool.submit([]() -> int { throw std::runtime_error("boom"); });
    bool threw = false;
    try { b.get(); } catch (const std::runtime_error&) { threw = true; }
    assert(threw);
    std::cout << "PASS\n";
}

void test_parallel_terrain_is_deterministic() {
    std::cout << "[Test] Terrain: parallel height bake equals the serial one bit for bit... " << std::flush;
    constexpr int N = 256;
    std::vector<float> serial(N * N), parallel(N * N);
    auto sample = [](int i, int j) { return fastjet::graphics::TerrainField::height(i * 37.0f - 4000.0f, j * 41.0f - 5000.0f); };
    for (int j = 0; j < N; ++j) {
        for (int i = 0; i < N; ++i) serial[static_cast<size_t>(j * N + i)] = sample(i, j);
    }
    ThreadPool::shared().parallel_for(0, N, [&](int j) {
        for (int i = 0; i < N; ++i) parallel[static_cast<size_t>(j * N + i)] = sample(i, j);
    });
    assert(std::memcmp(serial.data(), parallel.data(), serial.size() * sizeof(float)) == 0);
    std::cout << "PASS\n";
}

int main() {
    std::cout << "=== Thread Pool ===\n";
    test_parallel_for_covers_range();
    test_parallel_for_uses_workers();
    test_nested_and_busy_pool();
    test_submit_future();
    test_parallel_terrain_is_deterministic();
    std::cout << "All thread pool tests passed.\n";
    return 0;
}
