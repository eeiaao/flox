# EventDispatcher\<Event>

Static helper that routes an **event object** to the appropriate handler method
on a subscriber / listener, resolved entirely at compile time via template
specialisation.

````cpp
template <typename Event>
struct EventDispatcher {
  static void dispatch(const Event& ev, auto& sub);   // default undefined
};

// Handles pooled events transparently
template <typename T>
struct EventDispatcher<pool::Handle<T>> {
  static void dispatch(const pool::Handle<T>& ev, auto& sub) {
    EventDispatcher<T>::dispatch(*ev, sub);
  }
};

// Concrete specialisations
template <> struct EventDispatcher<BookUpdateEvent> {
  static void dispatch(const BookUpdateEvent& ev, auto& sub) { sub.onBookUpdate(ev); }
};
template <> struct EventDispatcher<TradeEvent> {
  static void dispatch(const TradeEvent& ev, auto& sub)     { sub.onTrade(ev); }
};
template <> struct EventDispatcher<CandleEvent> {
  static void dispatch(const CandleEvent& ev, auto& sub)    { sub.onCandle(ev); }
};
template <> struct EventDispatcher<OrderEvent> {
  static void dispatch(const OrderEvent& ev, auto& listener){ ev.dispatchTo(listener); }
};
````

## Purpose

* Decouple `EventBus` from event-specific logic: the bus calls
  `EventDispatcher<Event>::dispatch(ev, listener)` and remains oblivious to
  event type or handler name.

## Key Points

| Feature                  | Detail                                                                |
| ------------------------ | --------------------------------------------------------------------- |
| **pool::Handle support** | Unwraps the handle and forwards the underlying event.                 |
| **Zero runtime cost**    | All dispatch decisions are resolved at compile time.                  |
| **Extensible**           | Add a new `<Event>` specialisation to support additional event types. |

## Usage inside EventBus

```cpp
Policy::dispatch(*item, listener); // resolved to correct onBookUpdate/onTrade/…
```
