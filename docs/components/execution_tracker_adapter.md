# ExecutionTrackerAdapter

The `ExecutionTrackerAdapter` bridges the `IOrderExecutionListener` and `IExecutionTracker` interfaces.  
It allows components to track execution latency without modifying their existing callback chains.

## Purpose

To inject latency tracking into systems that already rely on `IOrderExecutionListener` without duplicating logic.

## Class Definition

```cpp
class ExecutionTrackerAdapter : public IOrderExecutionListener {
public:
  ExecutionTrackerAdapter(SubscriberId id, IExecutionTracker *tracker);

  void onOrderAccepted(const Order &order) override;
  void onOrderPartiallyFilled(const Order &order, Quantity qty) override;
  void onOrderFilled(const Order &order) override;
  void onOrderCanceled(const Order &order) override;
  void onOrderExpired(const Order &order) override;
  void onOrderRejected(const Order &order, const std::string &reason) override;
  void onOrderReplaced(const Order &oldOrder, const Order &newOrder) override;
};
```

## Responsibilities

- Converts order lifecycle callbacks into `IExecutionTracker` calls
- Automatically attaches a timestamp using `std::chrono::steady_clock::now()`
- Forwards execution events with timing metadata

## Use Cases

- Wrap a `PositionManager` with tracking support
- Enable runtime profiling of order latency without coupling strategy code to timing
- Reuse tracker logic across multiple listeners

## Notes

- The adapter does nothing if `_tracker` is `nullptr`
- All timestamps are taken at the moment the event is received