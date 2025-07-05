# CandleBus

`CandleBus` is a compile-time alias of `EventBus` specialised for `CandleEvent`.
Build-time macro `USE_SYNC_CANDLE_BUS` selects deterministic **sync** delivery or
low-latency **async** fan-out.

~~~cpp
#ifdef USE_SYNC_CANDLE_BUS
using CandleBus = EventBus<CandleEvent, SyncPolicy<CandleEvent>>;
#else
using CandleBus = EventBus<CandleEvent, AsyncPolicy<CandleEvent>>;
#endif

using CandleBusRef = EventBusRef<CandleEvent, CandleBus::Queue>;
~~~

## Purpose
* Broadcast finalised OHLCV candles from `CandleAggregator` to strategies,
  loggers, or metrics collectors with zero extra allocations.

## Responsibilities

| Aspect        | Details                                                         |
|---------------|-----------------------------------------------------------------|
| Fan-out       | One SPSC queue per subscriber.                                  |
| Policy switch | `SyncPolicy` → tick-barrier for backtests; `AsyncPolicy` → lock-free for live. |
| Reference     | `CandleBusRef` lets producers publish without owning the bus.   |

## Internal Behaviour
* The bus itself performs no heap allocations after initial queue setup.  
* In sync mode the publisher waits until every subscriber processes the tick
  before publishing the next candle.  
* In async mode the publisher enqueues and returns immediately.

## Notes
* Thread safety is provided by the chosen policy’s guarantees.  
* Define/undefine `USE_SYNC_CANDLE_BUS` **before** including the header to
  switch modes.
