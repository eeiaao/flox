# Market Data Bus

The `MarketDataBus` is a high-performance event distribution system used to fan out market data (e.g. book updates, trades) to multiple subscribers.

## Purpose

To asynchronously or synchronously deliver events to strategies, aggregators, and books in an efficient, non-blocking, and allocation-free manner.

---

## Interface Summary

```cpp
class MarketDataBus {
public:
  using Queue = BookUpdateBus::Queue;

  void subscribe(std::shared_ptr<IMarketDataSubscriber> subscriber);
  Queue* getQueue(SubscriberId id);

  void publish(EventHandle<BookUpdateEvent> ev);
  void publish(const TradeEvent& ev);

  void start();
  void stop();
};
```

## Responsibilities

- Maintain a dedicated `SPSCQueue` for each subscriber
- Deliver events with zero heap allocation
- Track subscribers by `SubscriberId`
- Use `EventHandle` to manage lifecycle of dispatched events
- Fan-out book and trade events via internal buses

## Modes

- **Asynchronous (default)**: Fan-out using lock-free queues
- **Synchronous (`USE_SYNC_MARKET_BUS`)**: Waits for all subscribers to complete tick processing via `TickBarrier`

## Notes

- Internally composed of `BookUpdateBus` and `TradeBus`
- Compatible with both `PUSH` and `PULL` subscriber modes
- Central to real-time book and trade distribution

