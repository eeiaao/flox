# CandleEvent

`CandleEvent` is the immutable message passed through `CandleBus` to deliver a completed OHLCV candle to every subscribed component.

~~~cpp
struct CandleEvent {
  using Listener = MarketDataSubscriberRef;

  SymbolId symbol;   // numeric identifier of the trading pair
  Candle   candle;   // OHLCV data for the interval
  uint64_t tickSequence; // global sequence number (monotonic per feed)
};
~~~

## Purpose
* Convey the finalised **candle data** for one symbol and interval.
* Provide a lightweight, trivially copyable payload for lock-free queues.

## Responsibilities

| Field           | Role                                                    |
|-----------------|---------------------------------------------------------|
| `symbol`        | Allows fast lookup of per-symbol state without strings. |
| `candle`        | Holds OHLCV (open, high, low, close, volume).           |
| `tickSequence`  | Preserves ordering across mixed event streams.          |

## Notes
* The struct itself performs **no dynamic allocations**.
* Ownership: published by `CandleAggregator`, consumed by strategies, loggers, metrics.
