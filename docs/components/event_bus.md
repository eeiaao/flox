# EventBus\<Event, Policy, QueueSize>

Generic lock-free fan-out bus delivering `Event` objects to multiple subscribers,
with compile-time **Policy** (`SyncPolicy`/`AsyncPolicy`) and configurable
`QueueSize` (default `config::DEFAULT_EVENTBUS_QUEUE_SIZE`).

~~~cpp
template <typename Event,
          typename Policy,
          size_t   QueueSize = config::DEFAULT_EVENTBUS_QUEUE_SIZE>
class EventBus;
~~~

## Highlights
| Aspect              | Details |
|---------------------|---------|
| **Queue type**      | `SPSCQueue<QueueItem, QueueSize>` per subscriber (lock-free). |
| **Subscriber modes**| `PUSH` (own thread) or `PULL` (caller polls queue via `getQueue`). |
| **Sync vs Async**   | Sync adds `TickBarrier*` and waits; async enqueues and returns immediately. |
| **Tick numbering**  | `_tickCounter` auto-fills `event.tickSequence` if present. |
| **Drain-on-stop**   | Call `enableDrainOnStop()` → bus finishes remaining items before exit. |

## Public API
```cpp
void subscribe(Listener);
void start();              // spin up PUSH threads
void stop();               // signal shutdown, join threads
void publish(Event);       // broadcast to every queue
auto getQueue(SubscriberId)
  -> std::optional<std::reference_wrapper<Queue>>;
uint64_t currentTickId() const;
void enableDrainOnStop();
````

## Internal Flow

1. `publish()` stamps sequence, builds `QueueItem` via `Policy::makeItem()`.
2. Each subscriber’s queue receives the item; in **sync** mode a local
   `TickBarrier` enforces deterministic completion.
3. PUSH listeners loop on `try_pop()` → `Policy::dispatch()` until `_running`
   becomes `false`, then optionally drain remaining items.
4. `stop()` swaps `_subs` into a local map so destructors join safely without
   holding the global `_mutex`.

## Trait & Handle

* `traits::EventBusTrait<Event, Queue>` generates a static v-table for any bus
  satisfying the `concepts::EventBus` contract.
* `EventBusRef<Event, Queue>` is a two-pointer type-erased handle that forwards
  all calls without virtual inheritance.

## Notes

* No heap allocations after construction; queues are fixed-size.
* Thread-safe: publisher acquires `_mutex` only for queue pointers, not for
  per-event operations.
