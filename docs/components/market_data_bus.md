# Market Data Bus

The `MarketDataBus` is a high-performance event distribution system used to fan out market data (e.g. book updates, trades) to multiple subscribers.

## Purpose

To asynchronously or synchronously deliver events to strategies, aggregators, and books in an efficient, non-blocking, and allocation-free manner.

---

## Interface Summary

```cpp
class MarketDataBus {
public:
  void subscribe(std::shared_ptr<IMarketDataSubscriber> subscriber);
  SPSCQueue<EventHandle<IMarketDataEvent>>* getQueue(SubscriberId id);
  void start();
  void stop();

  template <typename T>
  void publish(EventHandle<T> event);
};
```

## Responsibilities

- Maintain a dedicated `SPSCQueue` for each subscriber
- Deliver events with zero heap allocation
- Track subscribers by `SubscriberId`
- Use `EventHandle` to manage lifecycle of dispatched events

## Modes

- **Asynchronous (default)**: Fan-out using lock-free queues
- **Synchronous (`USE_SYNC_MARKET_BUS`)**: Waits for all subscribers to complete tick processing via `TickBarrier`

## Notes

- Internally implemented using a `MarketDataBus::Impl` with per-subscriber queue maps
- Compatible with both `PUSH` and `PULL` subscriber modes
- Central to all real-time data distribution in the engine