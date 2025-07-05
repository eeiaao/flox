# RiskManager (concept / trait / ref)

Compile-time contract and type-erased handle for subsystems that decide whether an **order is allowed** before it is sent.

~~~cpp
// Concept
template <typename T>
concept RiskManager =
    Subsystem<T> &&
    requires(T rm, const Order& o) {
      { rm.allow(o) } -> std::same_as<bool>;
    };
~~~

| Piece | Responsibility |
|-------|----------------|
| **`RiskManagerTrait`** | Builds a static v-table combining `SubsystemTrait` with the single `allow()` check. |
| **`RiskManagerRef`**   | Two-pointer handle `{void*, VTable*}` forwarding `start/stop` and `allow()` with no virtual overhead. |

## RiskManagerRef API
````cpp
void start() const;                 // lifecycle
void stop()  const;

bool allow(const Order&) const;     // true → pass, false → block
````

## Purpose

* Centralise **pre-trade risk checks** (position limits, PnL threshold, kill-switch integration).
* Provide a uniform interface so strategies / executors can query risk with one call.

## Notes

* Zero runtime cost beyond one pointer indirection.
* Implementers simply satisfy `concepts::RiskManager`, then wrap in `RiskManagerRef` for dependency injection.
