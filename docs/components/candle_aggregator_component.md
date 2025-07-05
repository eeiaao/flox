# CandleAggregator

`CandleAggregator` is a **concept / trait / ref trio** that plugs a candle-building
component into the FLOX engine with zero virtuals and full compile-time checks.

~~~cpp
// Concept
template <typename T>
concept CandleAggregator = requires(T t, const T& ct,
                                    const TradeEvent&  te,
                                    const BookUpdateEvent& be,
                                    const CandleEvent& ce) {
  { ct.id()   } -> std::same_as<SubscriberId>;
  { ct.mode() } -> std::same_as<SubscriberMode>;
  { t.onTrade(te)      };
  { t.onBookUpdate(be) };
  { t.onCandle(ce)     };
  { t.start()          };
  { t.stop()           };
};
~~~

## Purpose
* Define the **minimal API** a candle-aggregating subsystem must expose.  
* Provide a **type-erased handle** (`CandleAggregatorRef`) that forwards calls
  through a statically generated **v-table** without virtual inheritance.  
* Compose existing traits (`MarketDataSubscriberTrait`, `SubsystemTrait`)
  into a single aggregate trait.

## Key Pieces

| Piece | Role |
|-------|------|
| **`CandleAggregator` (concept)** | Compile-time contract for any implementation. |
| **`traits::CandleAggregatorTrait`** | Generates a `VTable` combining subscriber + subsystem methods. |
| **`CandleAggregatorRef`** | Lightweight handle: stores `{void* ptr, VTable* vt}` and offers the expected public API. |

## Public Interface (handle)

~~~cpp
struct CandleAggregatorRef : RefBase<...> {
  SubscriberId   id()   const;
  SubscriberMode mode() const;

  void onTrade     (const TradeEvent& ev)       const;
  void onBookUpdate(const BookUpdateEvent& ev)  const;
  void onCandle    (const CandleEvent& ev)      const;

  void start() const;
  void stop()  const;
};
~~~

## Notes
* **No RTTI / virtuals** – dispatch is one pointer indirection via the static v-table.  
* The handle itself is **POD + two pointers**; pass by value.  
* Compile-time `static_assert(concepts::CandleAggregator<CandleAggregatorRef>)`
  guarantees the ref also meets the concept for uniform usage.
