#include <atomic>
#include <random>
#include <thread>
#include <vector>

#include <benchmark/benchmark.h>

#include "wrappers.hpp"

using namespace std::chrono_literals;
using namespace unum::hpc;

std::vector<unum::hpc::key_t> initial_keys;
std::vector<unum::hpc::key_t> keys;

void init_datasets() {
    initial_keys.resize(initial_keys_cnt);
    keys.resize(keys_cnt);

    std::iota(initial_keys.begin(), initial_keys.end(), 0);
    std::iota(keys.begin(), keys.end(), initial_keys_cnt);

    std::mt19937 generator {std::uint_fast32_t(time(NULL))};
    std::shuffle(initial_keys.begin(), initial_keys.end(), generator);
    std::shuffle(keys.begin(), keys.end(), generator);
}

void cleanup_datasets() {
    initial_keys.clear();
    keys.clear();
}

template <typename map_at>
map_at new_map() {
    return map_at {keys.data(), keys.data() + keys.size()};
}

#pragma mark - Different Workloads

template <typename map_at>
void construct_bench(benchmark::State& state) {
    for (auto _ : state)
        benchmark::DoNotOptimize(new_map<map_at>());

    state.counters["entries/core/s"] = benchmark::Counter(state.iterations() * keys.size(), benchmark::Counter::kIsRate);
}

template <typename map_at>
void insert_bench(benchmark::State& state) {
    map_at map;
    size_t ops_count = 0;
    for (auto _ : state)
        benchmark::DoNotOptimize(map.insert(keys[(++ops_count) & overflow_mask], 0));

    state.counters["entries/core/s"] = benchmark::Counter(ops_count, benchmark::Counter::kIsRate);
}

template <typename map_at>
void find_bench(benchmark::State& state) {

    static map_at map;
    static std::atomic<size_t> ops_count_global {0};
    size_t ops_count {0};
    size_t local_thread_offset {static_cast<size_t>(std::rand())};
    if (state.thread_index == 0) {
        map = new_map<map_at>();
        ops_count_global = 0;
    }

    for (auto _ : state)
        benchmark::DoNotOptimize(map.contains(keys[(++ops_count + local_thread_offset) & overflow_mask]));

    ops_count_global += ops_count;
    if (state.thread_index == 0) {
        std::this_thread::sleep_for(1s);
        state.counters["entries/core/s"] = benchmark::Counter(ops_count_global.load(), benchmark::Counter::kAvgThreadsRate);
        map.clear();
    }
}

template <typename map_at>
void erase_bench(benchmark::State& state) {
    auto map = new_map<map_at>();
    size_t ops_count = 0;
    for (auto _ : state)
        benchmark::DoNotOptimize(map.erase(keys[(++ops_count) & overflow_mask]));

    state.counters["entries/core/s"] = benchmark::Counter(ops_count, benchmark::Counter::kIsRate);
}

#pragma mark - Dispatch

void register_section(char const* section_name) {
    benchmark::RegisterBenchmark(section_name, [=](benchmark::State& state) {
        for (auto _ : state)
            ;
    });
}

auto register_one(char const* name, void (*func)(benchmark::State& state), size_t threads = 1, float min_dur = 10) {

    return benchmark::RegisterBenchmark(name, func)
        ->MinTime(min_dur)
        ->Threads(threads)
        ->Unit(benchmark::kNanosecond)
        ->UseRealTime();
}

void register_benchmarks() {

    auto threads = unum::hpc::max_threads;
    auto inserts = keys_cnt;
    auto deletes = keys_cnt / 8;

    register_section("Construct from range");
    register_one("std::unordered_map.construct", construct_bench<std_unordered_map_t>);
    register_one("tsl::sparse_map.construct", construct_bench<tsl_sparse_map_t>);
    register_one("tsl::hopscotch.construct", construct_bench<tsl_hopscotch_map_t>);
    register_one("tsl::robin_map.construct", construct_bench<tsl_robin_map_t>);
    register_one("google::dense_hash_map.construct", construct_bench<google_dense_hash_map_t>);
    register_one("google::sparse_hash_map.construct", construct_bench<google_sparse_hash_map_t>);

    register_section("Insert");
    register_one("std::unordered_map.insert", insert_bench<std_unordered_map_t>)->Iterations(inserts);
    register_one("tsl::sparse_map.insert", insert_bench<tsl_sparse_map_t>)->Iterations(inserts);
    register_one("tsl::hopscotch.insert", insert_bench<tsl_hopscotch_map_t>)->Iterations(inserts);
    register_one("tsl::robin_map.insert", insert_bench<tsl_robin_map_t>)->Iterations(inserts);
    register_one("google::dense_hash_map.insert", insert_bench<google_dense_hash_map_t>)->Iterations(inserts);
    register_one("google::sparse_hash_map.insert", insert_bench<google_sparse_hash_map_t>)->Iterations(inserts);

    register_section("Find");
    register_one("std::unordered_map.find", find_bench<std_unordered_map_t>, threads);
    register_one("tsl::sparse_map.find", find_bench<tsl_sparse_map_t>, threads);
    register_one("tsl::hopscotch.find", find_bench<tsl_sparse_map_t>, threads);
    register_one("tsl::robin_map.find", find_bench<tsl_sparse_map_t>, threads);
    register_one("google::dense_hash_map.find", find_bench<google_dense_hash_map_t>, threads);
    register_one("google::sparse_hash_map.find", find_bench<google_sparse_hash_map_t>, threads);

    register_section("Erase");
    register_one("std::unordered_map.erase", erase_bench<std_unordered_map_t>)->Iterations(deletes);
    register_one("tsl::sparse_map.erase", erase_bench<tsl_sparse_map_t>)->Iterations(deletes);
    register_one("tsl::hopscotch.erase", erase_bench<tsl_sparse_map_t>)->Iterations(deletes);
    register_one("tsl::robin_map.erase", erase_bench<tsl_sparse_map_t>)->Iterations(deletes);
    register_one("google::dense_hash_map.erase", erase_bench<google_dense_hash_map_t>)->Iterations(deletes);
    register_one("google::sparse_hash_map.erase", erase_bench<google_sparse_hash_map_t>)->Iterations(deletes);
}

int main(int argc, char** argv) {

    std::printf("Will initialize with %zu unique keys\n", unum::hpc::initial_keys_cnt);
    std::printf("Benchmark workload size for insertions is %zu keys\n", unum::hpc::keys_cnt);

    init_datasets();
    register_benchmarks();

    benchmark::Initialize(&argc, argv);
    benchmark::RunSpecifiedBenchmarks();

    cleanup_datasets();
    return 0;
}
