# KillSwitch (concept / trait / ref)

Compile-time contract and type-erased handle for components that enforce **risk
limits** and can halt trading instantly.

~~~cpp
// Concept
template <typename T>
concept KillSwitch = requires(T ks, const Order& o, const std::string& reason) {
  ks.check(o);           // validate order, maybe trigger
  ks.trigger(reason);    // force-trigger
  { ks.isTriggered() } -> std::same_as<bool>;
  { ks.reason()     } -> std::same_as<std::string>;
};
~~~

| Piece | Responsibility |
|-------|----------------|
| **`KillSwitchTrait`** | Builds a static v-table with `check`, `trigger`, `isTriggered`, `reason`. |
| **`KillSwitchRef`**   | Two-pointer handle `{void*, VTable*}` that forwards calls without virtual inheritance. |

## KillSwitchRef API
````cpp
void check(const Order&);             // call before each order submit
void trigger(const std::string& msg); // external forced shutdown
bool isTriggered() const;             // query state
std::string reason() const;           // reason string
````

## Purpose

* Provide a pluggable **circuit-breaker** that strategies and executors can
  consult before sending orders.
* Centralise logic for position/size caps, PnL limits, rate limits, etc.

## Notes

* Zero runtime overhead beyond one pointer indirection.
* Extend by implementing a concrete class satisfying `concepts::KillSwitch`
  and pass it where a `KillSwitchRef` is expected.
