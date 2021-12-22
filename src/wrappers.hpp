#pragma once
#include <thread>
#include <unordered_map>

#include "sparsehash-2.0.4/dense_hash_map"
#include "sparsehash-2.0.4/sparse_hash_map"
#include "sparse-map-0.6.2/sparse_map.h"
#include "hopscotch-map-2.3.0/hopscotch_map.h"
#include "robin-map-0.6.3/robin_map.h"

namespace unum::hpc {

using key_t = int64_t;
using val_t = int64_t;

using hash_t = std::hash<key_t>;
using equal_t = std::equal_to<key_t>;

constexpr size_t initial_keys_cnt = 1024ull * 1024ull * 128ull;
constexpr size_t keys_cnt = initial_keys_cnt;
constexpr size_t overflow_mask = keys_cnt - 1;

/**
 * @brief  x86 generally have hyper-threading enabled,
 * which add extra context switches - negatively affecting perf.
 */
#if defined(__x86_64__)
static size_t const max_threads = std::thread::hardware_concurrency() / 2;
#else
static size_t const max_threads = std::thread::hardware_concurrency();
#endif

template <typename key_iterator_at = key_t const *, typename pair_at = std::pair<key_t, val_t>>
struct pair_iterator_gt {

    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = pair_at;
    using pointer = value_type*;
    using reference = value_type&;

    inline pair_iterator_gt(key_iterator_at const& key_iterator) : iter_(key_iterator) {}

    inline pair_iterator_gt& operator++() {
        ++iter_;
        return *this;
    }

    inline value_type operator*() { return {*iter_, 0}; }
    inline size_t operator-(pair_iterator_gt const& it) { return iter_ - it.iter_; }
    inline bool operator!=(pair_iterator_gt const& it) { return iter_ != it.iter_; }

    template <typename end_iter_at>
    inline bool operator!=(pair_iterator_gt<end_iter_at, pair_at> const& it) {
        return iter_ != it.iter_;
    }

  private:
    key_iterator_at iter_;
};

template <typename map_at>
struct map_wrapper_gt {
    using map_t = map_at;
    using pair_iterator_t = pair_iterator_gt<>;

    map_wrapper_gt() = default;
    map_wrapper_gt(key_t const * first, key_t const * last) : map_(pair_iterator_t {first}, pair_iterator_t {last}) {}

    inline bool insert(key_t key, val_t val) { return map_.insert({key, val}).first != map_.end(); }
    inline bool contains(key_t key) { return map_.find(key) != map_.end(); }
    inline bool empty() const { return map_.empty(); }
    inline void clear() { map_.clear(); }
    inline size_t size() { return map_.size(); }
    inline bool erase(key_t key) { return map_.erase(key); };

  private:
    map_at map_;
};

template <>
struct map_wrapper_gt<google::dense_hash_map<key_t, val_t, hash_t, equal_t>> {
    using map_t = google::dense_hash_map<key_t, val_t, hash_t, equal_t>;
    using pair_iterator_t = pair_iterator_gt<>;

    inline map_wrapper_gt() { map_.set_empty_key(std::numeric_limits<key_t>::max()); }
    map_wrapper_gt(key_t const * first, key_t const * last) : map_(pair_iterator_t {first}, pair_iterator_t {last}, 0) {}

    inline bool insert(key_t key, val_t val) { return map_.insert({key, val}).first != map_.end(); }
    inline bool contains(key_t key) { return map_.find(key) != map_.end(); }
    inline bool empty() const { return map_.empty(); }
    inline void clear() { map_.clear(); }
    inline size_t size() { return map_.size(); }
    inline bool erase(key_t key) { return map_.erase(key); };

  private:
    map_t map_;
};


using std_unordered_map_t = map_wrapper_gt<std::unordered_map<key_t, val_t, hash_t, equal_t>>;

using tsl_sparse_map_t = map_wrapper_gt<tsl::sparse_map<key_t, val_t, hash_t, equal_t>>;
using tsl_hopscotch_map_t = map_wrapper_gt<tsl::hopscotch_map<key_t, val_t, hash_t, equal_t>>;
using tsl_robin_map_t = map_wrapper_gt<tsl::robin_map<key_t, val_t, hash_t, equal_t>>;

using google_dense_hash_map_t = map_wrapper_gt<google::dense_hash_map<key_t, val_t, hash_t, equal_t>>;
using google_sparse_hash_map_t = map_wrapper_gt<google::sparse_hash_map<key_t, val_t, hash_t, equal_t>>;

} // namespace unum::hpc
