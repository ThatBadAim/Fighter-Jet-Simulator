#include "../include/fastjet/fdm/rk4_integrator.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <new>

// Allocation tracking mechanism
static std::atomic<bool> g_track_allocations{false};
static std::atomic<size_t> g_allocation_count{0};

void* operator new(size_t size) {
    if (g_track_allocations.load(std::memory_order_relaxed)) {
        g_allocation_count.fetch_add(1, std::memory_order_relaxed);
    }
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    return p;
}

void operator delete(void* p) noexcept {
    std::free(p);
}

void operator delete(void* p, size_t) noexcept {
    std::free(p);
}

using namespace fastjet::math;
using namespace fastjet::fdm;

void test_zero_garbage_allocation() {
    std::cout << "[Test] Validating Zero Heap Allocation in Hot Integration Loop... " << std::flush;

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005); // 200 Hz

    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -10000.0);
    state.vel_b = Vector3(250.0, 0.0, 0.0);
    state.omega_b = Vector3(0.1, 0.05, 0.02);
    state.q_att = Quaternion::identity();

    double time = 0.0;
    const AircraftForces forces(Vector3(50000.0, 1000.0, -90000.0), Vector3(1000.0, 2000.0, 500.0));

    // Reset allocation counter and enable tracking
    g_allocation_count.store(0);
    g_track_allocations.store(true);

    // Run 50,000 hot loop integration steps (corresponds to 250 seconds of flight)
    constexpr int STEPS = 50000;
    for (int i = 0; i < STEPS; ++i) {
        integrator.step(state, time, mass, forces);
    }

    g_track_allocations.store(false);
    const size_t total_allocations = g_allocation_count.load();

    // Verify zero allocations occurred
    assert(total_allocations == 0);

    std::cout << "PASSED\n"
              << "  Executed " << STEPS << " RK4 steps with EXACTLY "
              << total_allocations << " dynamic heap allocations (0 bytes garbage)!\n";
}

void benchmark_throughput() {
    std::cout << "[Benchmark] 6-DoF RK4 Integration Throughput... " << std::flush;

    const MassProperties mass = MassProperties::create_clean_f16();
    const RK4Integrator integrator(0.005);

    FlightState state;
    state.pos_ned = Vector3(0.0, 0.0, -10000.0);
    state.vel_b = Vector3(250.0, 0.0, 0.0);
    state.omega_b = Vector3(0.05, 0.02, 0.01);
    state.q_att = Quaternion::identity();

    double time = 0.0;
    const AircraftForces forces(Vector3(45000.0, 500.0, -91000.0), Vector3(500.0, 1000.0, -200.0));

    constexpr int BENCHMARK_STEPS = 2000000; // 2 million steps (10,000 simulated seconds @ 200 Hz)

    const auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < BENCHMARK_STEPS; ++i) {
        integrator.step(state, time, mass, forces);
#ifndef _MSC_VER
        asm volatile("" : "+m"(state), "+m"(time) : : "memory");
#endif
    }

    const auto finish = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double> elapsed = finish - start;

    // Prevent compiler dead-code elimination
    volatile double sink = state.pos_ned.x + state.vel_b.x + state.omega_b.x + state.q_att.w;
    (void)sink;

    const double steps_per_sec = BENCHMARK_STEPS / elapsed.count();
    const double sim_seconds_per_sec = steps_per_sec * integrator.dt;
    const double time_per_step_ns = (elapsed.count() * 1e9) / BENCHMARK_STEPS;
    const double realtime_factor = sim_seconds_per_sec; // vs 1 sec realtime

    std::cout << "DONE\n"
              << "  Total Steps:      " << BENCHMARK_STEPS << "\n"
              << "  Elapsed Time:     " << elapsed.count() << " seconds\n"
              << "  Throughput:       " << steps_per_sec << " steps/sec\n"
              << "  Latency per Step: " << time_per_step_ns << " ns\n"
              << "  Real-Time Speedup:" << realtime_factor << "x faster than 200 Hz real-time\n";

    // Ensure latency is well below 10 microseconds (10,000 ns) per step
    assert(time_per_step_ns < 10000.0);
}

int main() {
    std::cout << "=== Zero-Garbage & Performance Benchmark ===\n";
    test_zero_garbage_allocation();
    benchmark_throughput();
    std::cout << "All Performance tests passed successfully!\n\n";
    return 0;
}
