# Subscriber

Minimal identity interface for any component that receives events.

~~~cpp
using SubscriberId = std::uint64_t;

enum class SubscriberMode { PUSH, PULL };
~~~

## Concept
````cpp
template <typename T>
concept Subscriber =
    requires(T s) {
      { s.id()   } -> std::same_as<SubscriberId>;
      { s.mode() } -> std::same_as<SubscriberMode>;
    };
````

## Trait & Handle

* **`SubscriberTrait`** builds a static v-table wrapping `id()` and `mode()` via `meta::wrap`.
* **`SubscriberRef`** is a two-pointer handle `{void*, VTable*}` providing:

```cpp
SubscriberId   id()   const;
SubscriberMode mode() const;
```

## Purpose

* Give `EventBus` a uniform way to reference listeners, independent of their concrete type.
* Distinguish **PUSH** (dedicated thread) vs **PULL** (queue polled) delivery models at runtime.

*Zero virtuals; each call is one pointer indirection.*
