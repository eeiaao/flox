# CandleBus

`CandleBus` publishes completed candle information from `CandleAggregator` to interested subscribers.
It uses the generic `EventBus` and can operate in synchronous or asynchronous mode.

## Definition

```cpp
#ifdef USE_SYNC_CANDLE_BUS
using CandleBus = EventBus<CandleEvent, SyncPolicy<CandleEvent>>;
#else
using CandleBus = EventBus<CandleEvent, AsyncPolicy<CandleEvent>>;
#endif
```

## Responsibilities

- Deliver `CandleEvent` objects produced by `CandleAggregator`
- Provide lock-free queues per subscriber
- Assign a tick sequence number when publishing

## Notes

- Independent from the market data bus
- Controlled by the `USE_SYNC_CANDLE_BUS` compile definition

