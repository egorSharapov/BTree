#include <benchmark/benchmark.h>
#define NOTDEBUG
#include "Btree.hpp"
#include <chrono>
#include <set>

int range_query(const std::set<int64_t> &set, int64_t begin, int64_t end) {
    using it = std::set<int64_t>::iterator;
    it start = set.lower_bound(begin);
    it stop = set.upper_bound(end);
    return std::distance(start, stop);
}

static void BM_BTreeDistanceTest(benchmark::State &state) {
    for (auto _ : state) {
        state.PauseTiming();
        BTree<int64_t, 10> btree;
        for (int i = 0; i < state.range(0); ++i) {
            btree.insert(i);
        }
        state.ResumeTiming();

        int btree_distance = btree.distance(state.range(1), state.range(2));
        benchmark::DoNotOptimize(btree_distance);
    }
}

static void BM_SetDistanceTest(benchmark::State &state) {
    for (auto _ : state) {
        state.PauseTiming();
        std::set<int64_t> set;
        for (int64_t i = 0; i < state.range(0); ++i) {
            set.insert(i);
        }
        state.ResumeTiming();

        int set_distance = range_query(set, state.range(1), state.range(2));
        benchmark::DoNotOptimize(set_distance);
    }
}

BENCHMARK(BM_SetDistanceTest)->Args({1 << 11, 1 << 5, 1 << 10});
BENCHMARK(BM_BTreeDistanceTest)->Args({1 << 11, 1 << 5, 1 << 10});

BENCHMARK_MAIN();
