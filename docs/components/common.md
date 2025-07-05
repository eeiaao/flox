# Common Types

`common.h` defines the fundamental enums and fixed-precision numeric aliases used across FLOX.

~~~cpp
enum class OrderType { LIMIT, MARKET };
enum class Side      { BUY,   SELL   };

using SymbolId = uint32_t;   // compact key for maps
using OrderId  = uint64_t;   // globally unique order identifier

// fixed-precision numbers built on Decimal<Tag, Scale, Disp>
struct PriceTag    {};
struct QuantityTag {};
struct VolumeTag   {};

// tick = 0.000001 for all three
using Price    = Decimal<PriceTag,    1'000'000, 1>;
using Quantity = Decimal<QuantityTag, 1'000'000, 1>;
using Volume   = Decimal<VolumeTag,   1'000'000, 1>;
~~~

## Purpose
* Provide **canonical IDs** (`SymbolId`, `OrderId`) and enums (`OrderType`, `Side`).
* Expose **fixed-precision** `Price`, `Quantity`, `Volume` types to avoid FP rounding and preserve arithmetic integrity.

## Decimal Parameters

| Type       | Scale     | Smallest unit |
|------------|-----------|---------------|
| `Price`    | 1 000 000 | 0.000001      |
| `Quantity` | 1 000 000 | 0.000001      |
| `Volume`   | 1 000 000 | 0.000001      |

## Notes
* All three `Decimal` aliases share the same tick size for simplicity; adjust scale if exchange precision differs.
* `SymbolId` keeps hot maps cache-friendly compared to `std::string` keys.
