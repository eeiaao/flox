# OrderEvent

`OrderEvent` packages a single change in an order’s lifecycle and can forward itself
to any `OrderExecutionListener` via `dispatchTo()`.

````cpp
enum class OrderEventType {
  ACCEPTED, PARTIALLY_FILLED, FILLED,
  CANCELED, EXPIRED, REJECTED, REPLACED
};

struct OrderEvent {
  OrderEventType type;
  Order          order;        // original order
  Order          newOrder;     // for REPLACED
  Quantity       fillQty;      // for PARTIALLY_FILLED
  std::string    rejectionReason;
  uint64_t       tickSequence; // filled by EventBus
};
````

## Fields

| Field             | Used in event type(s) | Meaning                                  |
| ----------------- | --------------------- | ---------------------------------------- |
| `type`            | *all*                 | Kind of lifecycle transition.            |
| `order`           | *all*                 | Order being updated.                     |
| `newOrder`        | `REPLACED`            | Replacement order.                       |
| `fillQty`         | `PARTIALLY_FILLED`    | Quantity filled in this partial.         |
| `rejectionReason` | `REJECTED`            | Text reason from exchange / risk checks. |
| `tickSequence`    | *all*                 | Monotonic ID injected by `EventBus`.     |

## `dispatchTo()`

```cpp
template <typename ListenerT>
void dispatchTo(ListenerT& l) const {
  switch (type) {
    case OrderEventType::ACCEPTED:         l.onOrderAccepted(order);                 break;
    case OrderEventType::PARTIALLY_FILLED: l.onOrderPartiallyFilled(order, fillQty); break;
    case OrderEventType::FILLED:           l.onOrderFilled(order);                   break;
    case OrderEventType::CANCELED:         l.onOrderCanceled(order);                 break;
    case OrderEventType::EXPIRED:          l.onOrderExpired(order);                  break;
    case OrderEventType::REJECTED:         l.onOrderRejected(order, rejectionReason);break;
    case OrderEventType::REPLACED:         l.onOrderReplaced(order, newOrder);       break;
  }
}
```

*Compile-time guarded by `static_assert(concepts::OrderExecutionListener<ListenerT>)`
to ensure the listener implements every callback.*

## Purpose

* Provide a **single, strongly-typed object** for all order-execution notifications.
* Allow `EventBus` to deliver the event generically while each listener handles the specifics through `dispatchTo()`.
