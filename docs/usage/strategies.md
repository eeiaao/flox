# Writing Strategies

Strategies in Flox are implemented by subclassing `IStrategy`, which defines a uniform interface for receiving market data and submitting orders. Strategies are market data subscribers with execution capability and dependency injection.

---

## Purpose

To encapsulate trading logic that reacts to real-time or historical market data, and interfaces with execution, validation, position, and risk systems.

---

## Interface Overview

```cpp
class IStrategy : public IMarketDataSubscriber {
 public:
  // Lifecycle
  virtual void onStart();
  virtual void onStop();

  // Market data event callbacks
  virtual void onCandle(const CandleEvent& candle) override;
  virtual void onTrade(const TradeEvent& trade) override;
  virtual void onBookUpdate(const BookUpdateEvent& bookUpdate) override;

  // MarketDataSubscriber identification
  SubscriberId id() const override;
  SubscriberMode mode() const override;

  // Dependencies
  void setRiskManager(IRiskManager*);
  void setPositionManager(IPositionManager*);
  void setOrderExecutor(IOrderExecutor*);
  void setOrderValidator(IOrderValidator*);
  void setKillSwitch(IKillSwitch*);

 protected:
  IOrderExecutor* GetOrderExecutor() const;
  IRiskManager* GetRiskManager() const;
  IPositionManager* GetPositionManager() const;
  IOrderValidator* GetOrderValidator() const;
  IKillSwitch* GetKillSwitch() const;
};
```

---

## Strategy Lifecycle

- `onStart()` — called by engine on startup (before any events)
- `onStop()` — called before engine shutdown

---

## Market Data Subscriptions

Each strategy is a `IMarketDataSubscriber` and automatically receives:
- `onBookUpdate(...)`
- `onTrade(...)`
- `onCandle(...)`

depending on subscription mode (`PUSH` or `PULL`).
Strategies explicitly define their pull or push behavior via `SubscriberMode`. Synchronous mode (enabled via compile-time flag) deliver events deterministically, but strategy logic remains the same across environments. Pull mode can be used in live mode as well (e.g., ML strategies).

---

## Order Submission Flow

Use the injected components to guard order flow:

```cpp
if (GetOrderValidator() && !GetOrderValidator()->validate(order, reason)) return;
if (GetRiskManager() && !GetRiskManager()->allow(order)) return;
GetOrderExecutor()->submitOrder(order);
```

This ensures:
- Pre-check with validator
- Risk compliance check
- Order is submitted only when both checks pass

---

## Dependency Injection

Set during initialization via:
- `setOrderExecutor()` — **required**, routes orders
- `setRiskManager()` — optional, risk filter
- `setPositionManager()` — optional, local position view
- `setOrderValidator()` — optional, stateless pre-check
- `setKillSwitch()` — optional, emergency stop integration

---

## Best Practices

- Keep all callbacks (`onTrade`, etc) **non-blocking** — each subscriber runs in its own thread with a dedicated queue, so one strategy cannot block others. However, minimizing latency is still important for responsiveness and scheduling fairness
- Avoid allocations and blocking I/O
- Track only minimal state per-symbol or per-tick
- Never retain raw pointers to event objects — they may be returned to a pool and reused after the callback returns
- Only use `PositionManager` if your strategy requires tracking positions internally. Stateless strategies or pure signal generators often don't need it and can avoid unnecessary dependencies

---

## Example Strategy

```cpp
class MyStrategy : public IStrategy {
public:
  void onBookUpdate(const BookUpdateEvent& update) override {
    if (!shouldEnter(update)) return;

    Order order = buildOrder(update);
    auto* validator = GetOrderValidator();
    auto* risk = GetRiskManager();
    auto* executor = GetOrderExecutor();

    std::string reason;
    if (validator && !validator->validate(order, reason)) return;
    if (risk && !risk->allow(order)) return;
    executor->submitOrder(order);
  }

private:
  bool shouldEnter(const BookUpdateEvent& update) const {
    // Simple trigger: spread is wide enough
    Price spread = update.bestAskPrice() - update.bestBidPrice();
    return spread >= MinSpread;
  }

  Order buildOrder(const BookUpdateEvent& update) const {
    // Attempt to enter passively near best bid
    Price price = update.bestBidPrice() + TickImprovement;
    Quantity qty = DefaultQuantity;
    return Order{/* symbol */ update.symbol(),
                 /* side */ Side::Buy,
                 /* price */ price,
                 /* qty */ qty,
                 /* type */ OrderType::Limit};
  }

  static constexpr Price TickImprovement = 0.01;
  static constexpr Price MinSpread = 0.03;
  static constexpr Quantity DefaultQuantity = 100;
};
```

---

## Integration with Engine

Strategies are registered via factory mechanisms (e.g. StrategyConfig) and assigned to specific symbols. Strategies operate identically in both live and backtest modes via unified interface.