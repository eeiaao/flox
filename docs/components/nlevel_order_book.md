# NLevelOrderBook\<MaxLevels>

Static-array order book that maintains up to **MaxLevels** price levels per side
without heap allocations.

~~~cpp
template <size_t MaxLevels = 8192>
class NLevelOrderBook {
public:
  explicit NLevelOrderBook(Price tickSize);

  void applyBookUpdate(const BookUpdateEvent& ev);

  std::optional<Price> bestBid() const;
  std::optional<Price> bestAsk() const;

  Quantity bidAtPrice(Price p) const;
  Quantity askAtPrice(Price p) const;

  void clear();

  static constexpr size_t MAX_LEVELS = MaxLevels;
};
~~~

## Purpose
* Offer a **cache-friendly**, zero-alloc alternative to map-based books by
  indexing directly into fixed arrays.

## Internal Layout
| Field         | Meaning                          |
|---------------|----------------------------------|
| `_tickSize`   | Smallest price increment.        |
| `_bids`/`_asks` | `std::array<Quantity, MAX_LEVELS>` storing size at each level. |
| Index bounds  | `_minBidIndex`, `_maxBidIndex`, `_minAskIndex`, `_maxAskIndex` track populated range for quick scans. |

## Key Operations
* **`applyBookUpdate`** — applies SNAPSHOT or DELTA; fills/zeros levels, updates min/max indices.
* **`bestBid` / `bestAsk`** — linear scan within known bounds for non-zero level.
* **`priceToIndex`** — `price / tickSize` → array index; **no bounds check** beyond caller’s max-level guard.

## Complexity
| Operation          | Cost            |
|--------------------|-----------------|
| Update (per level) | O(1)            |
| Best bid/ask       | O(depth range)  |
| Lookup by price    | O(1)            |

## Notes
* `MAX_LEVELS` must accommodate expected spread; excess levels are ignored.
* Not thread-safe; wrap with external synchronisation if required.
