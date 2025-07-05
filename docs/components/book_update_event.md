# BookUpdateEvent

`BookUpdateEvent` is the lightweight, pool-recyclable message object that carries a single order-book update through the FLOX market-data pipeline.

```cpp
struct BookUpdateEvent : public pool::PoolableBase<BookUpdateEvent> {
  using Listener = MarketDataSubscriberRef;

  BookUpdate update;       // bids / asks vectors (PMR-powered)
  uint64_t   tickSequence; // monotonically increasing exchange tick

  explicit BookUpdateEvent(std::pmr::memory_resource* res);
  void clear();            // zero-cost recycle
};
```

## Purpose

* Encapsulate one **level-2 book snapshot / delta** together with its **tick sequence number**.  
* Be **zero-allocation** after construction by storing bids/asks inside a caller-provided `std::pmr::memory_resource`.  
* Integrate with FLOX’s object pool so events can be reused between ticks.

## Responsibilities

| Aspect            | Details                                                                                                   |
|-------------------|-----------------------------------------------------------------------------------------------------------|
| **Data**          | Owns a `BookUpdate` instance (`bids`, `asks`, meta) and a `tickSequence`.                                 |
| **Memory**        | Must be constructed with a valid PMR; asserts if `res == nullptr`.                                        |
| **Recycling**     | `clear()` wipes vectors without releasing their capacity, ready for the next reuse.                       |
| **Pool contract** | Inherits `PoolableBase` and satisfies `concepts::Poolable`, enabling placement-new from `pool::Pool<T>`.  |

## Internal Behaviour

1. **Constructor** stores the memory resource inside `update`, enabling fast vector growth without global `new`.  
2. **clear()** calls `update.bids.clear()` and `update.asks.clear()` only — no deallocation; capacity is kept for future ticks.  
3. `static_assert(concepts::Poolable<…>)` guarantees compile-time compatibility with FLOX pools.

## Notes

* One event per tick per symbol; reuse via pool avoids heap churn.  
* Not thread-safe by itself — concurrency is handled by the surrounding `EventBus` queues.
