#include <benchmark/benchmark.h>
#include "orderbook/FixedDepthOrderBook.h"

using namespace orderbook;

static void BM_SnapshotUpdate(benchmark::State& state) {
    const size_t depth = state.range(0);
    FixedDepthOrderBook book(depth);
    
    // Create a snapshot update
    BookUpdateEvent event{
        .type = BookUpdateEvent::Type::SNAPSHOT,
        .side = Side::BID,
        .price = 100.0,
        .volume = 10.0
    };
    
    for (auto _ : state) {
        book.applyUpdate(event);
        event.price += 0.01; // Move price to create different levels
        benchmark::DoNotOptimize(book);
    }
}
BENCHMARK(BM_SnapshotUpdate)->Range(5, 20);

static void BM_DeltaUpdate(benchmark::State& state) {
    const size_t depth = state.range(0);
    FixedDepthOrderBook book(depth);
    
    // Initialize with some data
    for (size_t i = 0; i < depth; ++i) {
        book.applyUpdate({
            .type = BookUpdateEvent::Type::SNAPSHOT,
            .side = Side::BID,
            .price = 100.0 - i * 0.01,
            .volume = 10.0
        });
    }
    
    // Create a delta update
    BookUpdateEvent event{
        .type = BookUpdateEvent::Type::DELTA,
        .side = Side::BID,
        .price = 100.0,
        .volume = 1.0
    };
    
    for (auto _ : state) {
        book.applyUpdate(event);
        benchmark::DoNotOptimize(book);
    }
}
BENCHMARK(BM_DeltaUpdate)->Range(5, 20);

static void BM_TopOfBook(benchmark::State& state) {
    const size_t depth = state.range(0);
    FixedDepthOrderBook book(depth);
    
    // Initialize with some data
    for (size_t i = 0; i < depth; ++i) {
        book.applyUpdate({
            .type = BookUpdateEvent::Type::SNAPSHOT,
            .side = Side::BID,
            .price = 100.0 - i * 0.01,
            .volume = 10.0
        });
        book.applyUpdate({
            .type = BookUpdateEvent::Type::SNAPSHOT,
            .side = Side::ASK,
            .price = 100.0 + i * 0.01,
            .volume = 10.0
        });
    }
    
    for (auto _ : state) {
        benchmark::DoNotOptimize(book.getBestBid());
        benchmark::DoNotOptimize(book.getBestAsk());
    }
}
BENCHMARK(BM_TopOfBook)->Range(5, 20);

BENCHMARK_MAIN(); 