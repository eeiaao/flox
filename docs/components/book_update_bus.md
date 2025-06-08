# BookUpdateBus

`BookUpdateBus` delivers `BookUpdateEvent` objects to all subscribed market data consumers.
It is based on the generic `EventBus` and supports synchronous or asynchronous dispatch.

## Definition

```cpp
#ifdef USE_SYNC_MARKET_BUS
using BookUpdateBus = EventBus<EventHandle<BookUpdateEvent>, SyncPolicy<EventHandle<BookUpdateEvent>>>;
#else
using BookUpdateBus = EventBus<EventHandle<BookUpdateEvent>, AsyncPolicy<EventHandle<BookUpdateEvent>>>;
#endif
```

## Responsibilities

- Fan-out book updates with minimal latency
- Provide per-subscriber queues (`SPSCQueue`)
- Assign tick sequence numbers to events

## Notes

- Used internally by `MarketDataBus`
- Controlled by the `USE_SYNC_MARKET_BUS` compile definition

