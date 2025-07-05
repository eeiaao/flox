# MarketDataSubscriber (concept / trait / ref)

Compile-time contract and type-erased handle for components that consume **market-data events**.

~~~cpp
// Concept
template <typename T>
concept MarketDataSubscriber =
    Subscriber<T> &&
    requires(T t,
             const BookUpdateEvent&  b,
             const TradeEvent&       tr,
             const CandleEvent&      c) {
      t.onBookUpdate(b);
      t.onTrade(tr);
      t.onCandle(c);
    };
~~~

| Piece | Responsibility |
|-------|----------------|
| **`MarketDataSubscriberTrait`** | Builds a static v-table combining **Subscriber** base info with three event callbacks (`onBookUpdate`, `onTrade`, `onCandle`). |
| **`MarketDataSubscriberRef`**   | Two-pointer handle `{void*, VTable*}` that forwards calls without virtual inheritance. |

## MarketDataSubscriberRef API
````cpp
SubscriberId   id()   const;
SubscriberMode mode() const;

void onTrade     (const TradeEvent&);      // trade prints
void onBookUpdate(const BookUpdateEvent&); // L2 updates
void onCandle    (const CandleEvent&);     // finished OHLCV
````

## Purpose

* Uniform interface for strategies, loggers, metrics collectors to receive real-time market data.
* Decouples `EventBus` dispatch logic from concrete subscriber implementations via compile-time resolution.

## Notes

* Zero runtime overhead beyond one pointer indirection per call.
* `static_assert(concepts::MarketDataSubscriber<MarketDataSubscriberRef>)` ensures the handle itself meets the concept for seamless composition.
