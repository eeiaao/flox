# OrderExecutionListener (concept / trait / ref)

Compile-time contract and type-erased handle for components that react to **order-execution events**.

~~~cpp
// Concept
template <typename T>
concept OrderExecutionListener =
    Subsystem<T> && Subscriber<T> &&
    requires(T t, const Order& o, const Order& o2,
             const std::string& r, Quantity q) {
      t.onOrderSubmitted(o);
      t.onOrderAccepted(o);
      t.onOrderPartiallyFilled(o, q);
      t.onOrderFilled(o);
      t.onOrderCanceled(o);
      t.onOrderExpired(o);
      t.onOrderRejected(o, r);
      t.onOrderReplaced(o, o2);
    };
~~~

| Piece | Responsibility |
|-------|----------------|
| **`OrderExecutionListenerTrait`** | Builds a static v-table that joins **SubscriberTrait** + **SubsystemTrait** with eight order-callbacks. |
| **`OrderExecutionListenerRef`**   | Two-pointer handle `{void*, VTable*}` that forwards calls with no virtual overhead. |

## OrderExecutionListenerRef API
````cpp
SubscriberId   id()   const;
SubscriberMode mode() const;

void start() const;
void stop()  const;

void onOrderSubmitted     (const Order&)           const;
void onOrderAccepted      (const Order&)           const;
void onOrderPartiallyFilled(const Order&, Quantity)const;
void onOrderFilled        (const Order&)           const;
void onOrderCanceled      (const Order&)           const;
void onOrderExpired       (const Order&)           const;
void onOrderRejected      (const Order&, const std::string&) const;
void onOrderReplaced      (const Order& oldOrd, const Order& newOrd) const;
````

## Purpose

* Distribute order-state changes to multiple subsystems (PnL tracker, logger, GUI, risk monitor) through a uniform interface.
* Maintain zero-cost dispatch — one pointer indirection per call, resolved at compile time via `meta::wrap`.

## Notes

* `static_assert(concepts::OrderExecutionListener<OrderExecutionListenerRef>)` guarantees the handle itself satisfies the concept.
