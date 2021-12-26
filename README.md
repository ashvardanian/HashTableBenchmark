# Hash-Tables Benchmarks

This repository contains a bunch of extendable benchmarks mostly for following containers:

* `std:unordered_map` from STL.
* `google::dense_hash_map` from [`sparsehash/sparsehash`](https://github.com/sparsehash/sparsehash) v2.0.4.
* `google::sparse_hash_map` from [`sparsehash/sparsehash`](https://github.com/sparsehash/sparsehash) v2.0.4.
* `tsl::sparse_map` from [`tessil/sparse-map`](https://github.com/Tessil/sparse-map) v0.6.2.
* `tsl::hopscotch_map` from [`tessil/hopscotch-map`](https://github.com/Tessil/hopscotch-map) v2.3.0.
* `tsl::robin_map` from [`tessil/robin-map`](https://github.com/Tessil/robin-map) v0.6.3.

The keys and values are generally set to 64-bit integers to mimic most workloads.
Keys are generated with the uniform ditribution. The hash function is set to `std::hash`.

## Build

Running `bash -i build.sh` will build following executables:

* `./build/build/bin/mixed_ops` - speed test comparing insertions, removals and lookups across containers.
* `./build/build/bin/ram_usage` - a benchmark analyzing the space overhead of each container, which shouldn't be called directly, only through `python ram_usage_bench.py`.
* `./build/build/bin/ram_speed` - a [memory-bandwidth utilization](https://unum.cloud/post/2021-12-21-macbook/) benchmark.
