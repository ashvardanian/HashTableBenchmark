#include <string>
#include <exception>
#include <unordered_map>

#include <benchmark/benchmark.h>

#include "wrappers.hpp"

using namespace unum::hpc;

template <typename map_at>
void insert_bench(size_t count) {
    map_at map;
    for (size_t i = 0; i < count; ++i)
        map.insert(static_cast<unum::hpc::key_t>(i), static_cast<unum::hpc::val_t>(i));
}

void benchmark_target(std::string_view target, size_t count) {
    if (target == "std::unordered_map")
        insert_bench<std_unordered_map_t>(count);

    else if (target == "tsl::sparse_map")
        insert_bench<tsl_sparse_map_t>(count);
    else if (target == "tsl::hopscotch")
        insert_bench<tsl_hopscotch_map_t>(count);
    else if (target == "tsl::robin_map")
        insert_bench<tsl_robin_map_t>(count);

    else if (target == "google::dense_hash_map")
        insert_bench<google_dense_hash_map_t>(count);
    else if (target == "google::sparse_hash_map")
        insert_bench<google_sparse_hash_map_t>(count);

    else
        throw std::logic_error("Unsupported container type");
}

int main(int argc, char** argv) {

    size_t count_entries = 0;
    std::string_view target_map;
    if (argc != 5)
        throw std::logic_error("Invalid number of arguments");

    std::string_view arg_count_entries {argv[1]};
    if (arg_count_entries != "--count_entries")
        throw std::logic_error("Wrong argument order");
    std::sscanf(argv[2], "%zu", &count_entries);

    std::string_view arg_container_type {argv[3]};
    if (arg_container_type != "--container")
        throw std::logic_error("Wrong argument order");
    target_map = std::string_view {argv[4]};

    benchmark_target(target_map, count_entries);
    return 0;
}