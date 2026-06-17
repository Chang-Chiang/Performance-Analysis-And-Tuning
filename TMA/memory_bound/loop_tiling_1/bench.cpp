
#include "benchmark/benchmark.h"
#include "solution.hpp"
#include "solution_flat.hpp"

static void bench1(benchmark::State& state) {
    constexpr int N = 2000;

    MatrixOfDoubles in;
    MatrixOfDoubles out;
    in.resize(N, std::vector<double>(N, 0.0));
    out.resize(N, std::vector<double>(N, 0.0));

    // Init benchmark data
    initMatrix(in);

    // Run the benchmark
    for (auto _ : state) {
        auto output = solution(in, out);
        benchmark::DoNotOptimize(output);
    }
}

static void bench_flat(benchmark::State& state) {
    constexpr int N = 2000;

    FlatMatrix in(N * N, 0.0);
    FlatMatrix out(N * N, 0.0);

    // Init benchmark data
    initFlatMatrix(in, N);

    // Run the benchmark
    for (auto _ : state) {
        auto output = solution_flat(in, out, N);
        benchmark::DoNotOptimize(output);
    }
}

// Register the function as a benchmark and measure time in microseconds
BENCHMARK(bench1)->Unit(benchmark::kMillisecond);
BENCHMARK(bench_flat)->Unit(benchmark::kMillisecond);

// Run the benchmark
BENCHMARK_MAIN();
