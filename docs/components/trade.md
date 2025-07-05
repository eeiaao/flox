# Trade

Lightweight POD struct representing a single executed trade.

```cpp
struct Trade {
  SymbolId  symbol;     // numeric symbol identifier
  Price     price;      // traded price
  Quantity  quantity;   // traded size
  bool      isBuy;      // true = buyer-initiated
  std::chrono::steady_clock::time_point timestamp; // exchange or arrival time
};
````

## Purpose

Hold minimal, fixed-precision data for each tick trade so it can be routed
through `TradeEvent` and aggregated into candles or analytics without
allocations.

## Notes

* `Price` / `Quantity` are fixed-point `Decimal` types → no FP rounding.
* `timestamp` uses `steady_clock` to avoid wall-clock glitches.
