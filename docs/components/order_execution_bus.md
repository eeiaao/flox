# OrderExecutionBus

`OrderExecutionBus` dispatches `OrderEvent` objects to execution listeners.
It provides the same asynchronous or synchronous policies as other buses.

## Definition

```cpp
#ifdef USE_SYNC_ORDER_BUS
using OrderExecutionBus = EventBus<OrderEvent, SyncPolicy<OrderEvent>>;
#else
using OrderExecutionBus = EventBus<OrderEvent, AsyncPolicy<OrderEvent>>;
#endif
```

## Responsibilities

- Fan-out order lifecycle events (fills, rejections, cancellations)
- Allow multiple listeners such as `PositionManager` and latency trackers

## Notes

- Used directly by executors to report order status
- Controlled by the `USE_SYNC_ORDER_BUS` compile definition

