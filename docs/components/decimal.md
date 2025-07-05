# Decimal\<Tag, Scale, TickSize>

`Decimal` is FLOX’s fixed-point number: a 64-bit scaled integer with strong typing by **Tag**, compile-time **Scale** (denominator), and optional **TickSize** (minimum step).

~~~cpp
template <typename Tag,
          int      Scale,            // e.g. 1'000'000  ⇒ 6 decimals
          int64_t  TickSize = 1>     // minimal increment
class Decimal { /* scaled-int impl. */ };

using Price    = Decimal<struct PriceTag,    1'000'000>; // tick 0.000001
using Quantity = Decimal<struct QuantityTag, 1'000'000>;
using Volume   = Decimal<struct VolumeTag,   1'000'000>;
~~~

## Purpose
* Eliminate floating-point rounding in price/quantity arithmetic.  
* Provide **strong unit safety** (`Price`, `Quantity`, `Volume` are incompatible types).

## Key Points

| Aspect     | Detail                                   |
|------------|------------------------------------------|
| **Scale**  | Denominator (`1'000'000` ⇒ 6 decimal places). |
| **TickSize** | Smallest allowed increment (default 1). |
| **Tag**    | Distinguishes units to prevent mixing.   |

## Typical Operations
* `fromDouble()`, `toDouble()` for I/O.  
* `roundToTick()` floors to nearest multiple of `TickSize`.  
* Standard `+ - * /` with `int64_t` scaling.

## Example
~~~cpp
Price p = Price::fromDouble(12.345678);
auto rounded = p.roundToTick();  // 12.345678 (already multiple of tick)
~~~

## Notes
* Entire API is `constexpr`, so calculations can be compile-time.  
* No implicit conversions between differently-tagged decimals, avoiding unit mix-ups.
