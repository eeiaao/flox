# EventBus (generic)

Header defines the **type-erased event bus layer** that powers FLOX fan-out
delivery, plus two compile-time policies and supporting concepts/traits.

## Key Pieces

| Piece | Role |
|-------|------|
| **`SyncPolicy<Event>`**  | Wraps each event with a `TickBarrier*`, enforcing *all subscribers finished* semantics (deterministic backtests). |
| **`AsyncPolicy<Event>`** | Pass-through event delivery — publisher returns immediately (live trading). |
| **`concepts::EventBus`** | Compile-time contract (`publish`, `subscribe`, `getQueue`, `currentTickId`, `enableDrainOnStop`). |
| **`traits::EventBusTrait`** | Generates a static v-table for any concrete bus that satisfies the concept. |
| **`EventBusRef<Event, Queue>`** | Lightweight handle `{void*, VTable*}` that forwards calls without virtual inheritance. |

## EventBusRef API

````cpp
EventBusRef<Event, Queue> bus = …;

bus.start();                       // subsystem lifecycle
bus.publish(event);                // fan-out
bus.subscribe(listener);           // add subscriber
auto q  = bus.getQueue(id);        // direct queue access (optional)
auto t  = bus.currentTickId();     // monotonic tick counter
bus.enableDrainOnStop();           // wait for queues to empty on stop()
````

## Choosing a Policy

```cpp
using BookUpdateBus = EventBus<
    pool::Handle<BookUpdateEvent>,
#ifdef USE_SYNC_MARKET_BUS
    SyncPolicy<pool::Handle<BookUpdateEvent>>
#else
    AsyncPolicy<pool::Handle<BookUpdateEvent>>
#endif
>;
```

*Define / undefine `USE_SYNC_*` macros **before** including the header to switch
between deterministic and low-latency modes.*

## Notes

* Neither policy allocates after construction; queues are pre-sized.
* `EventBusRef` itself is trivially copyable and can be passed by value.
