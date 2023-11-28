#include <benchmark/benchmark.h>
#include "Btree.hpp"
#include <set>


static void custom_args(benchmark::internal::Benchmark* b) {
    b->Args({1 << 11, 1 << 5, 1 << 10});
    b->Args({1 << 15, 0, 1 << 11});
}


static int range_query(benchmark::State &state, const std::set<int64_t> &set, int64_t begin, int64_t end) {
    using it = std::set<int64_t>::iterator;
    state.PauseTiming();
    it start = set.lower_bound(begin);
    it stop = set.upper_bound(end);
    state.ResumeTiming();
    return std::distance(start, stop);
}


template <size_t N>
static void BM_BTreeDistanceTest(benchmark::State &state) {
    for (auto _ : state) {
        state.PauseTiming();
        BTree<int64_t, N> btree;
        for (int i = 0; i < state.range(0); ++i) {
            btree.insert(i);
        }
        state.ResumeTiming();

        size_t btree_distance = btree.distance(state.range(1), state.range(2));
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

        size_t set_distance = range_query(state, set, state.range(1), state.range(2));
        benchmark::DoNotOptimize(set_distance);
    }
}

BENCHMARK(BM_SetDistanceTest)->Apply(custom_args);
BENCHMARK(BM_BTreeDistanceTest<6>)->Apply(custom_args);
BENCHMARK(BM_BTreeDistanceTest<8>)->Apply(custom_args);
BENCHMARK(BM_BTreeDistanceTest<10>)->Apply(custom_args);

BENCHMARK_MAIN();
