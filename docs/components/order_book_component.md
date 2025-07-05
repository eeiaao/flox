# OrderBook (concept / trait / ref)

Defines the compile-time contract for order-book implementations and supplies a
type-erased handle for uniform access.

~~~cpp
// Concept
template <typename T>
concept OrderBook = requires(T ob, const BookUpdateEvent& ev, Price p) {
  ob.applyBookUpdate(ev);                  // SNAPSHOT / DELTA
  { ob.bestBid()     } -> std::same_as<std::optional<Price>>;
  { ob.bestAsk()     } -> std::same_as<std::optional<Price>>;
  { ob.bidAtPrice(p) } -> std::same_as<Quantity>;
  { ob.askAtPrice(p) } -> std::same_as<Quantity>;
};
~~~

| Piece | Responsibility |
|-------|----------------|
| **`OrderBookTrait`** | Builds a static v-table with five member-function wrappers via `meta::wrap`. |
| **`OrderBookRef`**   | Two-pointer handle `{void*, VTable*}` forwarding calls with no virtual inheritance. |

## OrderBookRef API
````cpp
void                 applyBookUpdate(const BookUpdateEvent&);
std::optional<Price> bestBid()  const;
std::optional<Price> bestAsk()  const;
Quantity             bidAtPrice(Price) const;
Quantity             askAtPrice(Price) const;
````

## Purpose

* Allow engine modules (strategies, metrics, visualisers) to interact with any
  concrete order-book implementation through one lightweight interface.
* Keep zero-cost dispatch: one pointer indirection, no RTTI, no `std::function`.

## Usage

```cpp
NLevelOrderBook<> ob(tick);
OrderBookRef      ref{ &ob, traits::OrderBookTrait::makeVTable<NLevelOrderBook<>>() };

ref.applyBookUpdate(ev);
if (auto bid = ref.bestBid()) { /* … */ }
```

## Notes

* `static_assert(concepts::OrderBook<OrderBookRef>)` verifies the handle itself
  meets the concept.
* Extend with new books by implementing the required methods; no runtime changes needed.
