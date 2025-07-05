# OrderValidator (concept / trait / ref)

Compile-time contract and type-erased handle for components that **validate orders** before submission.

~~~cpp
// Concept
template <typename T>
concept OrderValidator =
    requires(T v, const Order& o, std::string& r) {
      { v.validate(o, r) } -> std::same_as<bool>;
    };
~~~

| Piece | Responsibility |
|-------|----------------|
| **`OrderValidatorTrait`** | Builds a static v-table exposing a single `validate` method via `meta::wrap`. |
| **`OrderValidatorRef`**   | Two-pointer handle `{void*, VTable*}` forwarding the call without virtual inheritance. |

## OrderValidatorRef API
````cpp
bool validate(const Order& order, std::string& reason) const;
````

*Returns* `true` when the order passes all checks; otherwise `false` and sets `reason`.

## Purpose

* Enforce **pre-trade risk rules** (price bands, size limits, account balance, etc.).
* Decouple validation logic from order generators and executors while keeping zero overhead (one pointer indirection).
