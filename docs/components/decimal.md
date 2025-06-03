# Decimal<T>

The `Decimal<Tag, Scale, TickSize>` template represents a fixed-point decimal number with compile-time scaling and optional tick size rounding. It is used to represent precise quantities like price or volume in trading systems, avoiding floating-point errors.

---

## Template Parameters

```cpp
template <typename Tag, int Scale, int64_t TickSize = 1>
class Decimal;
```

- `Tag`: Empty struct type used to distinguish different `Decimal` types (e.g. `PriceTag`, `VolumeTag`)
- `Scale`: Integer scale factor (e.g. 1_000_000 for 6 decimal places)
- `TickSize`: Optional tick size used for rounding and discretization

---

## Purpose

- Provide a safe and precise alternative to `double` in financial applications
- Ensure that prices and quantities are stored as integers internally
- Avoid runtime ambiguity between different uses of decimals (e.g., price vs quantity)

---

## Interface Summary

```cpp
static constexpr Decimal fromDouble(double val);
static constexpr Decimal fromRaw(int64_t raw);
constexpr double toDouble() const;
constexpr int64_t raw() const;
constexpr Decimal roundToTick() const;

constexpr bool isZero() const;

constexpr Decimal operator+(Decimal d) const;
constexpr Decimal operator-(Decimal d) const;
constexpr Decimal &operator+=(const Decimal &other);
constexpr Decimal &operator-=(const Decimal &other);
constexpr auto operator<=>(const Decimal &) const = default;
```

---

## Behavior

- Internally stores the value as a raw integer (`int64_t _raw`)
- `fromDouble()` converts a double to internal integer by multiplying with `Scale` and rounding
- `toDouble()` converts back to `double` for display or logging
- `roundToTick()` aligns the internal value to the nearest multiple of `TickSize`
- `isZero()` checks if the internal value is exactly zero

---

## Notes

- This type is constexpr-friendly and usable in low-latency paths
- The `Tag` type is not used at runtime, but enforces type-safety at compile time
- All operators and conversions are explicit to avoid accidental misuse

---

## Example Use

```cpp
struct PriceTag {};
struct VolumeTag {};

using Price = Decimal<PriceTag, 1'000_000>;
using Volume = Decimal<VolumeTag, 1'000_000>;

Price p = Price::fromDouble(12345.67);
Volume v = Volume::fromDouble(0.005);

double pv = p.toDouble();
int64_t raw = v.raw();
```