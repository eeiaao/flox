# Writing Strategies

Strategies in Flox are implemented by subclassing `IStrategy`, which defines a uniform interface for receiving market data and submitting orders.

## Purpose

To encapsulate trading logic that reacts to real-time market data and interacts with execution, risk, position tracking, and validation systems.

---

## Interface Summary

```cpp
class IStrategy : public IMarketDataSubscriber {
public:
  virtual ~IStrategy() = default;

  virtual void onStart();
  virtual void onStop();

  virtual void onCandle(SymbolId symbol, const Candle &candle);
  virtual void onTrade(TradeEvent *trade);
  virtual void onBookUpdate(BookUpdateEvent *bookUpdate);

  void setPositionManager(IPositionManager *manager);
  void setRiskManager(IRiskManager *manager);
  void setOrderExecutor(IOrderExecutor *executor);
  void setOrderValidator(IOrderValidator *validator);

protected:
  IRiskManager *GetRiskManager();
  IPositionManager *GetPositionManager();
  IOrderExecutor *GetOrderExecutor();
  IOrderValidator *GetOrderValidator();
};
```

## Core Methods

- `onStart()` / `onStop()` — called at engine startup and shutdown
- `onBookUpdate(...)` — receives `BookUpdateEvent*` from `MarketDataBus`
- `onTrade(...)` — receives `TradeEvent*`
- `onCandle(...)` — receives completed candles per symbol

## Subscribing to Market Data

Each strategy is automatically subscribed to `MarketDataBus` via `subscribe()`.  
Events are dispatched through `onMarketData(...)`, which routes them internally to `onBookUpdate(...)`, `onTrade(...)`, etc.

## Order Submission

Strategies place orders by combining risk check, validation, and execution:

```cpp
if (GetOrderValidator() && !GetOrderValidator()->validate(order, reason)) return;
if (GetRiskManager() && !GetRiskManager()->allow(order)) return;
GetOrderExecutor()->submitOrder(order);
```

This guarantees only validated and permitted orders reach the market.

## Dependency Injection

Use the provided setters to connect your strategy with engine components:

- `setOrderExecutor()` — required to send orders
- `setRiskManager()` — optional safeguard
- `setPositionManager()` — optional state tracking
- `setOrderValidator()` — optional pre-check logic

## Best Practices

- Keep callbacks (`onTrade`, `onBookUpdate`) fast and non-blocking
- Avoid allocations and blocking I/O
- Prefer stateless or short-term state tracking
- Use `PositionManager` only when necessary
- Use pooled `EventHandle` safely by not storing raw pointers long-term

## Example

```cpp
class MyStrategy : public IStrategy {
public:
  void onBookUpdate(BookUpdateEvent *update) override {
    Order order = {/* ... */};
    if (GetOrderValidator() && !GetOrderValidator()->validate(order, _reason)) return;
    if (GetRiskManager() && !GetRiskManager()->allow(order)) return;
    GetOrderExecutor()->submitOrder(order);
  }
};
```

Use factory classes to load strategies dynamically from configuration and assign them to symbols.