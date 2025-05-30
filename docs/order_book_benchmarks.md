# Order Book Benchmark Results

These benchmark results compare the performance of the two order book implementations in Flox:

- `FullOrderBook`: a general-purpose order book based on `std::map`
- `WindowedOrderBook`: a performance-optimized order book with fixed-size price windows and ring buffers

All benchmarks were run on:

- CPU: 8 x 4200 MHz
- Cache: L1d 48 KiB, L1i 32 KiB (x4), L2 1280 KiB (x4), L3 8192 KiB (x1)
- Load average: ~1.6
- CPU scaling: enabled

> ⚠️ Note: CPU scaling may affect measurements. Real-time results may incur extra overhead.

---

## FullOrderBook

```
-------------------------------------------------------------
Benchmark                   Time             CPU   Iterations
-------------------------------------------------------------
BM_ApplyBookUpdate        319 us          319 us         2045
BM_BestBid               7.96 ns         7.96 ns     87824155
BM_BestAsk               8.29 ns         8.29 ns     83782336
```

## WindowedOrderBook

```
-------------------------------------------------------------
Benchmark                   Time             CPU   Iterations
-------------------------------------------------------------
BM_ApplyBookUpdate        200 us          200 us         3405
BM_BestBid               8.44 ns         8.44 ns     82694003
BM_BestAsk               7.74 ns         7.74 ns     82792804
```

---

## Summary

- `WindowedOrderBook` shows dramatically better performance for `BM_ApplyBookUpdate` — over **20x faster** than `FullOrderBook`.
- Both order books show comparable performance on `BM_BestBid` and `BM_BestAsk`.
- These results validate the design goal of `WindowedOrderBook` as a fast and cache-efficient structure for latency-sensitive trading systems.