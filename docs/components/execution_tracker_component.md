# ExecutionTracker (concept / trait / ref)

Compile-time contract and type-erased handle for components that track **order-execution lifecycle events**.

~~~cpp
// Concept
template <typename T>
concept ExecutionTracker =
    Subscriber<T> && Subsystem<T> &&
    requires(T t, const Order& o, const Order& o2,
             Quantity q,
             std::chrono::steady_clock::time_point ts,
             const std::string& reason) {
      t.onOrderSubmitted(o, ts);
      t.onOrderAccepted(o, ts);
      t.onOrderPartiallyFilled(o, q, ts);
      t.onOrderFilled(o, ts);
      t.onOrderCanceled(o, ts);
      t.onOrderExpired(o, ts);
      t.onOrderRejected(o, reason, ts);
      t.onOrderReplaced(o, o2, ts);
    };
~~~

## Architecture

| Piece | Responsibility |
|-------|----------------|
| **`ExecutionTrackerTrait`** | Builds a static v-table bundling **Subscriber** + **Subsystem** methods and eight order-event callbacks. |
| **`ExecutionTrackerRef`**   | Lightweight handle `{void*, VTable*}` that forwards calls without virtual inheritance. |

### ExecutionTrackerRef API
````cpp
SubscriberId   id()   const;
SubscriberMode mode() const;

void start() const;
void stop()  const;

void onOrderSubmitted      (const Order&, std::chrono::steady_clock::time_point) const;
void onOrderAccepted       (const Order&, std::chrono::steady_clock::time_point) const;
void onOrderPartiallyFilled(const Order&, Quantity,
                            std::chrono::steady_clock::time_point) const;
void onOrderFilled         (const Order&, std::chrono::steady_clock::time_point) const;
void onOrderCanceled       (const Order&, std::chrono::steady_clock::time_point) const;
void onOrderExpired        (const Order&, std::chrono::steady_clock::time_point) const;
void onOrderRejected       (const Order&, const std::string& reason,
                            std::chrono::steady_clock::time_point) const;
void onOrderReplaced       (const Order& oldOrd, const Order& newOrd,
                            std::chrono::steady_clock::time_point) const;
````

## Purpose

* Centralise **PnL / metrics / audit** updates triggered by order state changes.
* Allow strategies, risk managers, or loggers to subscribe via `SubscriberTrait`
  while still being controlled as subsystems.

## Notes

* No runtime overhead beyond a single pointer indirection per call.
* `static_assert(concepts::ExecutionTracker<ExecutionTrackerRef>)` guarantees the
  ref also satisfies the concept.
