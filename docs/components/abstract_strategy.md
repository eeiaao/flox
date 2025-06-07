# Strategy Interface

The `IStrategy` interface defines the base class for implementing trading strategies in Flox.  
It combines market data subscription, lifecycle management, and dependency injection for execution, risk, and validation.

## Purpose

To allow developers to create modular strategies that respond to market data and place orders via injected subsystems.

---

## Interface Definition

```cpp
class IStrategy : public IMarketDataSubscriber {
public:
  virtual ~IStrategy() = default;

  // Lifecycle
  virtual void onStart();
  virtual void onStop();

  // Market data events
  virtual void onCandle(SymbolId symbol, const Candle &candle);
  virtual void onTrade(TradeEvent *trade) override;
  virtual void onBookUpdate(BookUpdateEvent *bookUpdate) override;

  // Subscription identity
  SubscriberId id() const override;
  SubscriberMode mode() const override;

  // Dependency injection
  void setRiskManager(IRiskManager *manager);
  void setPositionManager(IPositionManager *manager);
  void setOrderExecutor(IOrderExecutor *executor);
  void setOrderValidator(IOrderValidator *validator);

protected:
  IRiskManager *GetRiskManager();
  IPositionManager *GetPositionManager();
  IOrderExecutor *GetOrderExecutor();
  IOrderValidator *GetOrderValidator();
};
```

## Responsibilities

- Implements `IMarketDataSubscriber` with internal dispatching to typed events
- Owns a unique `SubscriberId` for queue-based delivery
- Reacts to market data with event handlers (`onTrade`, `onBookUpdate`, `onCandle`)
- Lifecycle hooks (`onStart`, `onStop`) are optional

## Notes

- Subscribes in `PUSH` mode to `MarketDataBus`
- Uses `EventHandle` dispatch flow for safe memory reuse
- All dependencies are injected via setter methods
- Order flow can be guarded by validator and risk manager before execution

## Dispatch Logic

```cpp
void onMarketData(const IMarketDataEvent &event) override {
  switch (event.eventType()) {
    case MarketDataEventType::TRADE:
      onTrade(static_cast<TradeEvent *>(...));
      break;
    case MarketDataEventType::BOOK:
      onBookUpdate(static_cast<BookUpdateEvent *>(...));
      break;
    default:
      break;
  }
}
```

## Example Order Submission

```cpp
if (GetOrderValidator() && !GetOrderValidator()->validate(order, reason)) return;
if (GetRiskManager() && !GetRiskManager()->allow(order)) return;
GetOrderExecutor()->submitOrder(order);
```
