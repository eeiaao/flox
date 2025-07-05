# Subsystem

Smallest lifecycle interface: **start** / **stop**.

~~~cpp
template <typename T>
concept Subsystem = requires(T s) {
  s.start();   // initialise resources, spawn threads, etc.
  s.stop();    // graceful shutdown
};
~~~

## Trait & Handle

| Piece | Responsibility |
|-------|----------------|
| **`SubsystemTrait`** | Builds a static v-table with `start` and `stop` via `meta::wrap`. |
| **`SubsystemRef`**   | Two-pointer handle `{void*, VTable*}`; calls into the v-table with zero virtuals. |

### SubsystemRef API
```cpp
void start() const;
void stop()  const;
```

## Purpose

Provide a common lifecycle contract so the **Engine** can orchestrate diverse components (buses, strategies, sinks, risk managers) in a deterministic order.

*Zero overhead: one pointer indirection per call.*
