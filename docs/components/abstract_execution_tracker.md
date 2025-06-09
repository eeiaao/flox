# Execution Tracker

The `IExecutionTracker` interface is responsible for tracking the lifecycle of orders in the system with high-resolution timestamps.

## Purpose

This interface enables components to collect precise latency metrics and audit trails for order submissions, fills, and rejections.

## Interface Definition

```cpp
class IExecutionTracker {
public:
  virtual ~IExecutionTracker() = default;

  virtual void onOrderSubmitted(const Order &order,
                                std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderAccepted(const Order &order,
                               std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderPartiallyFilled(const Order &order, Quantity qty,
                                      std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderFilled(const Order &order,
                             std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderCanceled(const Order &order,
                               std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderExpired(const Order &order,
                              std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderRejected(const Order &order, const std::string &reason,
                               std::chrono::steady_clock::time_point ts) = 0;

  virtual void onOrderReplaced(const Order &oldOrder, const Order &newOrder,
                               std::chrono::steady_clock::time_point ts) = 0;
};
```

## Use Cases

- Measuring round-trip latency from order submission to execution
- Diagnosing slow fills or rejections
- Auditing execution behavior across multiple venues

## Notes

- Timestamps are based on `std::chrono::steady_clock` to ensure monotonicity
- Can be combined with `ExecutionTrackerAdapter` for flexible dispatching