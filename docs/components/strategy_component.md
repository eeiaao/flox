# Strategy (concept / trait / ref)

Defines the contract and type-erased handle for **trading strategies** that both
*subscribe* to market data and behave as engine subsystems.

~~~cpp
// Concept
template <typename T>
concept Strategy =
    MarketDataSubscriber<T> && Subsystem<T>;
~~~

| Piece | Responsibility |
|-------|----------------|
| **`StrategyTrait`** | Combines `SubsystemTrait` (lifecycle) and `MarketDataSubscriberTrait` (onTrade/onBookUpdate/onCandle) into one v-table. |
| **`StrategyRef`**   | Two-pointer handle `{void*, VTable*}` forwarding lifecycle and market-data callbacks with zero virtuals. |

## StrategyRef API
```cpp
// Lifecycle
void start() const;
void stop()  const;

// Subscriber identity
SubscriberId   id()   const;
SubscriberMode mode() const;

// Market-data callbacks
void onTrade     (const TradeEvent&)      const;
void onBookUpdate(const BookUpdateEvent&) const;
void onCandle    (const CandleEvent&)     const;
```

## Purpose

* Allow the engine to manage arbitrary strategies uniformly, regardless of their internal logic.
* Enable strategies to receive ticks through `EventBus` while the engine controls their start/stop sequencing.

## Notes

* Zero runtime overhead beyond one pointer indirection per call.
* Implementers need only satisfy `concepts::Strategy`; wrap with `StrategyRef` for dependency injection.
