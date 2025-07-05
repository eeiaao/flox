# Order

Immutable data record representing a single trading order and its execution state.

~~~cpp
enum class OrderStatus {
  NEW, PENDING, PARTIALLY_FILLED, FILLED,
  CANCELED, EXPIRED, REPLACED, REJECTED
};

struct Order {
  OrderId   id;
  Side      side;                // BUY / SELL
  Price     price;               // limit price
  Quantity  quantity;            // requested size
  OrderType type;                // LIMIT / MARKET
  SymbolId  symbol;              // numeric symbol key

  OrderStatus status = OrderStatus::NEW;
  Quantity    filledQuantity{0};

  std::chrono::steady_clock::time_point              createdAt;
  std::optional<std::chrono::steady_clock::time_point> exchangeTimestamp;
  std::optional<std::chrono::steady_clock::time_point> lastUpdated;
  std::optional<std::chrono::steady_clock::time_point> expiresAfter;
};
~~~

## Purpose
* Hold all fields required by executors, validators, and trackers to
  follow the order’s lifecycle from *creation* to *terminal* state.

## Key Fields

| Field              | Description                                           |
|--------------------|-------------------------------------------------------|
| `id`               | Engine-wide unique identifier (`uint64_t`).           |
| `side`             | BUY / SELL direction.                                 |
| `price` / `quantity` | Limit price & size (zero for MARKET price).         |
| `type`             | LIMIT or MARKET (extendable).                         |
| `status`           | Current life-cycle stage (`OrderStatus`).             |
| `filledQuantity`   | Cumulated executed size.                              |
| `createdAt`        | Local timestamp when order object was created.        |
| `exchangeTimestamp`| Timestamp reported by the exchange for last update.   |
| `lastUpdated`      | Local time of last status change.                     |
| `expiresAfter`     | Optional auto-cancel deadline (GTC/GTD logic).        |

## Notes
* `std::optional<>` times stay `std::nullopt` until the corresponding event occurs.
* Decimal wrappers (`Price`, `Quantity`) guarantee fixed-precision arithmetic.
