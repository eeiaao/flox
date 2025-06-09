# Strategy Interface

The `IStrategy` interface defines the base class for implementing trading strategies in Flox.  
It combines market data subscription, lifecycle management, and dependency injection for execution, risk, and validation.

## Purpose

To allow developers to create modular strategies that respond to market data and place orders via injected subsystems.

---

## Interface Definition

```cpp
class IStrategy : public IMarketDataSubscriber
{
 public:
  // Lifecycle
  virtual void onStart();
  virtual void onStop();

  // Event hooks
  virtual void onCandle(const CandleEvent& candle) override;
  virtual void onTrade(const TradeEvent& trade) override;
  virtual void onBookUpdate(const BookUpdateEvent& bookUpdate) override;

  SubscriberId id() const override;
  SubscriberMode mode() const override;

  // Dependency injection
  void setRiskManager(IRiskManager* manager);
  void setPositionManager(IPositionManager* manager);
  void setOrderExecutor(IOrderExecutor* executor);
  void setOrderValidator(IOrderValidator* validator);
  void setKillSwitch(IKillSwitch* killSwitch);
};
```

## Responsibilities

- Implements `IMarketDataSubscriber` with internal dispatching to typed events
- Owns a unique `SubscriberId` for queue-based delivery
- Reacts to market data with event handlers (`onTrade`, `onBookUpdate`, `onCandle`)
- Lifecycle hooks (`onStart`, `onStop`) are optional

## Notes

- By default, subscribes in `PUSH` mode to `MarketDataBus`
- All dependencies are injected via setter methods
- Order flow can be guarded by validator, risk manager and killswitch before execution

## Event Dispatch

Events are delivered via dedicated callbacks without runtime type checks:

```cpp
void onCandle(const CandleEvent& candle) override {
  // handle candle
}

void onTrade(const TradeEvent &trade) override {
  // handle trade
}

void onBookUpdate(const BookUpdateEvent &update) override {
  // handle book
}
```

## Example Order Submission

```cpp
if (GetOrderValidator() && !GetOrderValidator()->validate(order, reason)) return;
if (GetRiskManager() && !GetRiskManager()->allow(order)) return;
GetOrderExecutor()->submitOrder(order);
```

