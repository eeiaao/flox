# Order Book Interface

The `IOrderBook` interface defines the minimal contract for any order book implementation within Flox.  
It supports applying market data updates and querying top-of-book and level-specific liquidity.

## Purpose

To abstract the representation and access to order book state, allowing different implementations (e.g., full depth, windowed).

## Interface Definition

```cpp
class IOrderBook {
public:
  virtual ~IOrderBook() = default;

  virtual void applyBookUpdate(const BookUpdate &update) = 0;
  virtual std::optional<double> bestBid() const = 0;
  virtual std::optional<double> bestAsk() const = 0;

  virtual double bidAtPrice(double price) const = 0;
  virtual double askAtPrice(double price) const = 0;
};
```

## Responsibilities

- `applyBookUpdate(...)`: update internal state with snapshot or delta updates
- `bestBid()` / `bestAsk()`: return top-of-book prices, or `std::nullopt` if empty
- `bidAtPrice(...)` / `askAtPrice(...)`: return quantity available at a given price level

## Implementations

- `FullOrderBook`: full-depth representation
- `WindowedOrderBook`: limited tick-range book for performance-sensitive applications

## Notes

- Implementations must ensure thread safety if used in concurrent environments
- Used directly by strategies and execution components
- Implementations should inline performance-critical methods for optimal performance.