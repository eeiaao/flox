# SymbolRegistry

In-memory bijective mapping between human-readable `(exchange, symbol)` pairs
(e.g. `"bybit", "BTCUSDT"`) and compact `SymbolId` integers used throughout the
engine.

~~~cpp
class SymbolRegistry {
public:
  SymbolId registerSymbol(const std::string& exchange,
                          const std::string& symbol);

  std::optional<SymbolId>
  getSymbolId(const std::string& exchange,
              const std::string& symbol) const;

  std::pair<std::string, std::string>
  getSymbolName(SymbolId id) const;
};
~~~

## Purpose
* Provide fast look-ups from textual symbols → numeric IDs for hot-path data
  structures.
* Allow reverse resolution for logging, UI, or persistence layers.

## Responsibilities

| Method            | Behaviour                                                     |
|-------------------|----------------------------------------------------------------|
| `registerSymbol`  | If the `(exchange,symbol)` pair is new, assigns a fresh `SymbolId` and stores the mapping; returns existing ID otherwise. |
| `getSymbolId`     | Thread-safe lookup; returns `std::nullopt` if pair is unknown. |
| `getSymbolName`   | Reverse lookup; throws/asserts if `id` out of range.          |

## Internal Data

| Field      | Type                                                       | Note                         |
|------------|------------------------------------------------------------|------------------------------|
| `_mutex`   | `std::mutex`                                               | Guards both maps for write/read concurrency. |
| `_map`     | `unordered_map<std::string, SymbolId>`                     | Key is `"exchange:symbol"` concatenated.     |
| `_reverse` | `std::vector<std::pair<std::string,std::string>>`          | Index = `SymbolId`; value is original pair.  |

## Notes
* `SymbolId` is `uint32_t` (from **common.h**) — small enough for array indexing.
* Registry is typically filled at startup from `EngineConfig`; reads dominate afterwards.
