# CandleAggregator

The `CandleAggregator` is a subsystem that aggregates trade data into time-based candlesticks.  
It emits complete `Candle` objects for each symbol at regular intervals via a user-defined callback.

## Purpose

To convert streaming trade data into OHLCV candles for analysis, strategy input, or logging.

## Class Definition

```cpp
class CandleAggregator : public ISubsystem {
public:
  using CandleCallback = std::function<void(SymbolId, const Candle &)>;

  CandleAggregator(std::chrono::seconds interval, CandleCallback callback);

  void start() override;
  void stop() override;

  void onTrade(const Trade &trade);
};
```

## Responsibilities

- Receives trades via `onTrade(...)`
- Aggregates OHLCV values within a time interval (`_interval`)
- Calls the registered `CandleCallback` when a new candle is completed
- Tracks per-symbol candle state internally
- Publishes completed candles to `CandleBus`

## Internal Design

- Uses `std::unordered_map<SymbolId, PartialCandle>` to accumulate state
- Candle emission is based on aligned timestamps (`alignToInterval(...)`)
- Implements `ISubsystem` for system lifecycle compatibility

## Use Cases

- Used by strategies that rely on time-based candles (e.g., moving average strategies)
- Enables plotting or historical export of candlestick data
- Provides compact representation of market activity

## Notes

- The component is stateless between restarts unless externally persisted
- Candle timestamps are system-clock-based
