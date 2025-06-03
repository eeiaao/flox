#include <benchmark/benchmark.h>
#include "orderbook/FixedDepthOrderBook.hpp"
#include <random>

using namespace orderbook;

// Helper function to generate random price levels
std::vector<PriceLevel> generateRandomLevels(
    size_t count,
    double basePrice,
    double priceStep,
    std::mt19937& gen) {
    
    std::uniform_real_distribution<> volumeDist(1.0, 1000.0);
    std::vector<PriceLevel> levels;
    levels.reserve(count);
    
    for (size_t i = 0; i < count; ++i) {
        double price = basePrice + (i * priceStep);
        levels.push_back({price, volumeDist(gen)});
    }
    
    return levels;
}

static void BM_SnapshotUpdate(benchmark::State& state) {
    const size_t depth = state.range(0);
    const size_t updateSize = state.range(1);
    
    std::mt19937 gen(42); // Fixed seed for reproducibility
    FixedDepthOrderBook book(depth);
    
    for (auto _ : state) {
        state.PauseTiming();
        BookUpdateEvent update;
        update.type = UpdateType::SNAPSHOT;
        update.bids = generateRandomLevels(updateSize, 100.0, -0.01, gen);
        update.asks = generateRandomLevels(updateSize, 100.1, 0.01, gen);
        state.ResumeTiming();
        
        book.applyUpdate(update);
    }
}

static void BM_DeltaUpdate(benchmark::State& state) {
    const size_t depth = state.range(0);
    const size_t updateSize = state.range(1);
    
    std::mt19937 gen(42);
    FixedDepthOrderBook book(depth);
    
    // Initial snapshot
    BookUpdateEvent snapshot;
    snapshot.type = UpdateType::SNAPSHOT;
    snapshot.bids = generateRandomLevels(depth, 100.0, -0.01, gen);
    snapshot.asks = generateRandomLevels(depth, 100.1, 0.01, gen);
    book.applyUpdate(snapshot);
    
    for (auto _ : state) {
        state.PauseTiming();
        BookUpdateEvent update;
        update.type = UpdateType::DELTA;
        update.bids = generateRandomLevels(updateSize, 100.0, -0.01, gen);
        update.asks = generateRandomLevels(updateSize, 100.1, 0.01, gen);
        state.ResumeTiming();
        
        book.applyUpdate(update);
    }
}

static void BM_GetLevels(benchmark::State& state) {
    const size_t depth = state.range(0);
    
    std::mt19937 gen(42);
    FixedDepthOrderBook book(depth);
    
    // Setup initial state
    BookUpdateEvent snapshot;
    snapshot.type = UpdateType::SNAPSHOT;
    snapshot.bids = generateRandomLevels(depth, 100.0, -0.01, gen);
    snapshot.asks = generateRandomLevels(depth, 100.1, 0.01, gen);
    book.applyUpdate(snapshot);
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.getLevels(Side::BID));
        benchmark::DoNotOptimize(book.getLevels(Side::ASK));
    }
}

// Register benchmarks with different depths and update sizes
BENCHMARK(BM_SnapshotUpdate)
    ->Args({5, 10})   // depth=5, updateSize=10
    ->Args({10, 20})  // depth=10, updateSize=20
    ->Args({20, 40}); // depth=20, updateSize=40

BENCHMARK(BM_DeltaUpdate)
    ->Args({5, 2})    // depth=5, deltaSize=2
    ->Args({10, 5})   // depth=10, deltaSize=5
    ->Args({20, 10}); // depth=20, deltaSize=10

BENCHMARK(BM_GetLevels)
    ->Arg(5)    // depth=5
    ->Arg(10)   // depth=10
    ->Arg(20);  // depth=20

BENCHMARK_MAIN(); 