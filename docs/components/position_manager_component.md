# PositionManager (concept / trait / ref)

Compile-time contract and type-erased handle for components that **track per-symbol positions** based on order-execution events.

~~~cpp
// Concept
template <typename T>
concept PositionManager =
    OrderExecutionListener<T> && Subsystem<T> &&
    requires(T pm, SymbolId sym) {
      { pm.getPosition(sym) } -> std::same_as<Quantity>;
    };
~~~

| Piece | Responsibility |
|-------|----------------|
| **`PositionManagerTrait`** | Builds a static v-table by extending `OrderExecutionListenerTrait` with `getPosition()`. |
| **`PositionManagerRef`**   | Two-pointer handle `{void*, VTable*}` that forwards order callbacks **and** position queries without virtuals. |

## PositionManagerRef API
````cpp
// Lifecycle & subscription (inherited)
SubscriberId   id()   const;
SubscriberMode mode() const;
void start() const;
void stop()  const;

// Order-execution fan-out (inherited)
void onOrderSubmitted     (const Order&)           const;
void onOrderAccepted      (const Order&)           const;
void onOrderPartiallyFilled(const Order&, Quantity)const;
void onOrderFilled        (const Order&)           const;
void onOrderCanceled      (const Order&)           const;
void onOrderExpired       (const Order&)           const;
void onOrderRejected      (const Order&, const std::string&) const;
void onOrderReplaced      (const Order&, const Order&) const;

// Position query
Quantity getPosition(SymbolId) const;
````

## Purpose

* Maintain real-time **net position** per symbol for PnL, risk checks, and strategy decisions.
* React to every order update through the inherited `OrderExecutionListener` interface, updating internal position state.

## Notes

* Zero runtime overhead beyond one pointer indirection per call.
* Implementers just need to satisfy `concepts::PositionManager`; no runtime registration required.
