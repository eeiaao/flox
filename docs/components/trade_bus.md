# TradeBus

`TradeBus` is an `EventBus` alias specialized for `TradeEvent`.  
Delivery mode is selected by the `USE_SYNC_MARKET_BUS` macro:

~~~cpp
#ifdef USE_SYNC_MARKET_BUS
using TradeBus = EventBus<TradeEvent, SyncPolicy<TradeEvent>>;
#else
using TradeBus = EventBus<TradeEvent, AsyncPolicy<TradeEvent>>;
#endif

using TradeBusRef =
    EventBusRef<TradeEvent, TradeBus::Queue>;
~~~

## Purpose
* Broadcast individual **trade prints** (tick trades) from exchange connectors
  to strategies, metrics, and loggers.

## Responsibilities
| Aspect        | Details                                                         |
|---------------|-----------------------------------------------------------------|
| **Fan-out**   | One lock-free SPSC queue per subscriber.                        |
| **Policy**    | `SyncPolicy` → barrier-synchronised for deterministic replay.<br>`AsyncPolicy` → low-latency for live trading. |
| **Reference** | `TradeBusRef` lets publishers and consumers hold a lightweight, type-erased handle. |

## Notes
* Define/undefine `USE_SYNC_MARKET_BUS` **before** including the header to switch modes.
* No heap allocations after construction; queues are pre-sized.
