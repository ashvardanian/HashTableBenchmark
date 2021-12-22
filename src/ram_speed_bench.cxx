#include <atomic>
#include <random>
#include <thread>
#include <vector>
#include <cstring>

#include <benchmark/benchmark.h>

#include "wrappers.hpp"

using namespace std::chrono_literals;
using namespace unum::hpc;

std::vector<size_t> container_sizes {
    1024ull * 1024ull * 16ull,
    1024ull * 1024ull * 32ull,
    1024ull * 1024ull * 64ull,
    1024ull * 1024ull * 128ull,
    1024ull * 1024ull * 256ull,
};
std::vector<unum::hpc::key_t> pregenerated_keys;
static constexpr size_t bytes_per_entry = sizeof(unum::hpc::key_t) + sizeof(unum::hpc::val_t);

#pragma mark - Different Workloads

template <typename map_at>
void find_bench(benchmark::State& state) {

    size_t const usable_bytes = state.range(0);
    size_t const wanted_iterations = state.range(1);
    size_t const map_size = usable_bytes / bytes_per_entry;
    size_t const overflow_mask = map_size - 1;

    // Each thread picks its own random subrange of pregenerated
    // keys and locates only them.
    size_t local_start_point = static_cast<size_t>(std::rand());
    size_t ops_count = 0;
    static map_at map;

    // Let's continue filling this map from where we have left.
    if (state.thread_index == 0) {
        auto it = pregenerated_keys.begin() + map.size();
        auto end = pregenerated_keys.begin() + map_size;
        for (; it != end; ++it)
            map.insert(*it, 0);
    }

    for (auto _ : state) {
        // Every lookup is fast operation, often around 30ns on small containers,
        // so we can't afford to generate random numbers on the fly, and can't
        // query the `state.titerations()`. That's why we keeep the `ops_count`.
        benchmark::DoNotOptimize(map.contains(pregenerated_keys[(local_start_point + ops_count) & overflow_mask]));
        ops_count++;
    }

    if (state.thread_index == 0) {
        auto lookups = wanted_iterations * state.threads;
        auto bytes = bytes_per_entry * lookups;
        state.counters["lookups/s"] = benchmark::Counter(lookups, benchmark::Counter::kIsRate);
        state.counters["bytes/s"] =
            benchmark::Counter(bytes, benchmark::Counter::kIsRate, benchmark::Counter::OneK::kIs1024);

        // Flush the container and reclaim some memory for next type benchmarks.
        if (map.size() == container_sizes.back())
            map.clear();
    }
}

void memcpy_bench(benchmark::State& state) {

    size_t const bytes_per_copy = state.range(0);
    size_t const wanted_iterations = state.range(1);
    size_t const entries_per_copy = bytes_per_copy / sizeof(unum::hpc::key_t);
    size_t const total_chunks = pregenerated_keys.size() / entries_per_copy;
    size_t const chunk_idx_mask = total_chunks - 1;

    std::vector<unum::hpc::key_t> local_copy;
    local_copy.resize(entries_per_copy);

    auto t = std::time(nullptr);
    std::mt19937_64 generator {std::uint_fast64_t(t)};

    for (auto _ : state) {
        size_t chunk_idx = generator() & chunk_idx_mask;
        benchmark::DoNotOptimize(
            std::memcpy(local_copy.data(), pregenerated_keys.data() + chunk_idx * entries_per_copy, bytes_per_copy));
    }

    if (state.thread_index == 0) {
        auto copies = wanted_iterations * state.threads;
        auto bytes = bytes_per_copy * copies;
        state.counters["copies/s"] = benchmark::Counter(copies, benchmark::Counter::kIsRate);
        state.counters["bytes/s"] =
            benchmark::Counter(bytes, benchmark::Counter::kIsRate, benchmark::Counter::OneK::kIs1024);
    }
}

#pragma mark - Dispatch

void register_class_sizes(char const* name, void (*func)(benchmark::State& state), size_t threads = 1) {

    constexpr size_t iterations = 1000000;
    for (auto size : container_sizes)
        benchmark::RegisterBenchmark(name, func)
            ->Iterations(iterations)
            ->Threads(threads)
            ->Unit(benchmark::kNanosecond)
            ->UseRealTime()
            ->Args({static_cast<long long>(size), static_cast<long long>(iterations)});
}

void register_classes(size_t threads = 1) {

    register_class_sizes("std::unordered_map.find", &find_bench<std_unordered_map_t>, threads);

    // TSL fastest hash-map is the Robin-Map.
    // register_class_sizes("tsl::sparse_map.find", &find_bench<tsl_sparse_map_t>, threads);
    // register_class_sizes("tsl::hopscotch.find", &find_bench<tsl_sparse_map_t>, threads);
    register_class_sizes("tsl::robin_map.find", &find_bench<tsl_sparse_map_t>, threads);

    // Googles fastest hash-map is the Dense one.
    // register_class_sizes("google::sparse_hash_map.find", &find_bench<google_sparse_hash_map_t>, threads);
    register_class_sizes("google::dense_hash_map.find", &find_bench<google_dense_hash_map_t>, threads);
}

void register_memcopies(size_t threads = 1) {

    constexpr size_t total_bytes_to_copy = 1024ull * 1024ull * 1024ull * 1024ull; // 1 TB

    for (auto size : {1024ull, 1024ull * 4ull, 1024ull * 1024ull, 1024ull * 1024ull * 4ull}) {
        size_t const iterations = total_bytes_to_copy / (size * threads);
        benchmark::RegisterBenchmark("std::memcpy", &memcpy_bench)
            ->Iterations(iterations)
            ->Threads(threads)
            ->Unit(benchmark::kNanosecond)
            ->UseRealTime()
            ->Args({static_cast<long long>(size), static_cast<long long>(iterations)});
    }
}

void generate_random_keys() {

    size_t const usable_bytes = container_sizes.back();
    size_t const map_size = usable_bytes / bytes_per_entry;

    auto t = std::time(nullptr);
    std::srand(t);
    std::mt19937_64 generator {std::uint_fast64_t(t)};

    pregenerated_keys.resize(map_size);
    for (size_t i = 0; i != pregenerated_keys.size(); ++i)
        pregenerated_keys[i] = static_cast<unum::hpc::key_t>(generator());
}

int main(int argc, char** argv) {

    generate_random_keys();
    register_memcopies(unum::hpc::max_threads);
    register_classes(unum::hpc::max_threads);

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    return 0;
}
