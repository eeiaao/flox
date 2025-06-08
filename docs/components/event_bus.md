# EventBus

`EventBus<Event, Policy>` is the generic publish/subscribe queue used throughout Flox.
It coordinates fan-out of events to multiple listeners with minimal overhead.

## Definition

```cpp
template <typename Event, typename Policy>
class EventBus {
public:
  using Listener = typename ListenerType<Event>::type;
  using Queue = SPSCQueue<typename Policy::QueueItem, 4096>;

  void subscribe(std::shared_ptr<Listener> listener);
  void start();
  void stop();

  void publish(Event ev);
  Queue* getQueue(SubscriberId id);
  uint64_t currentTickId() const;
};
```

## Policies

- **AsyncPolicy** – dispatch without waiting. Each event is queued for every subscriber.
- **SyncPolicy** – uses a `TickBarrier` so the publisher waits until all subscribers finish processing.

## Subscriber Modes

- **PUSH** – bus spawns a worker thread and pushes events to the listener.
- **PULL** – listener calls `getQueue()` and processes events manually.

## Notes

- Every published event receives an incrementing tick sequence number.
- Specialized buses (`BookUpdateBus`, `TradeBus`, `CandleBus`, `OrderExecutionBus`) are typedefs of `EventBus` with concrete event types.
