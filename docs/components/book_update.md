# BookUpdate

`BookUpdate` is a PMR-backed container that transports either a **snapshot** or **delta** of an order book for a single symbol.

~~~cpp
struct BookUpdate {
  SymbolId symbol;
  BookUpdateType type;                // SNAPSHOT or DELTA
  std::pmr::vector<BookLevel> bids;   // bid side
  std::pmr::vector<BookLevel> asks;   // ask side
  std::chrono::steady_clock::time_point timestamp;

  explicit BookUpdate(std::pmr::memory_resource* res);
};
~~~

## Purpose
* Hold the minimal data needed to rebuild or adjust an in-memory order book.  
* Avoid dynamic allocations by storing bid/ask vectors in a caller-supplied `std::pmr::memory_resource`.

## Responsibilities

| Aspect       | Details                                                                                         |
|--------------|-------------------------------------------------------------------------------------------------|
| **Payload**  | `bids` and `asks` vectors of `BookLevel` (price + quantity).                                    |
| **Type tag** | `type` distinguishes full **SNAPSHOT** from incremental **DELTA**.                              |
| **Timing**   | `timestamp` captures arrival time for latency measurement.                                      |
| **Memory**   | Uses the provided PMR; no heap allocations if the resource is an arena or pre-allocated buffer. |

## Internal Behaviour
1. Constructor stores the memory resource in both vectors, ensuring identical allocation strategy.  
2. `BookLevel` is POD, so vectors remain trivially relocatable.  
3. The struct itself is plain data; all validation and sequencing happen in surrounding logic.

## Notes
* Thread safety is delegated to the owning component (e.g., `BookUpdateBus`).  
* Typical sources: WebSocket parser, historical replay loader.  
* Passed by `pool::Handle<BookUpdateEvent>` to avoid copies in the hot path.
