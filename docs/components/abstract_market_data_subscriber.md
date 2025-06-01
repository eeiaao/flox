# Market Data Subscriber

The `IMarketDataSubscriber` interface defines the contract for components that receive market data from the bus.

## Purpose

To enable decoupled and efficient delivery of market data (e.g., book updates, trades) to consumers such as strategies or aggregators.

## Interface Definition

```cpp
class IMarketDataSubscriber {
public:
  virtual ~IMarketDataSubscriber() = default;
  virtual void onMarketData(const IMarketDataEvent &event) = 0;

  virtual SubscriberId id() const = 0;
  virtual SubscriberMode mode() const { return SubscriberMode::PUSH; }
};
```

## Responsibilities

- Consume events dispatched from `MarketDataBus`
- Identify each subscriber with a unique `id()`
- Indicate whether it operates in `PUSH` or `PULL` mode

## Notes

- `SubscriberId` is used to track and route messages
- Default mode is `PUSH`; override `mode()` to return `PULL` when polling is preferred