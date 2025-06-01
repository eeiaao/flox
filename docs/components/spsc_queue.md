# SPSCQueue

The `SPSCQueue` is a lock-free, single-producer single-consumer queue optimized for high-throughput, low-latency communication.

## Purpose

To provide efficient inter-thread message passing without dynamic memory allocation or locks.

---

## Template Parameters

```cpp
template <typename T, size_t Capacity>
class SPSCQueue;
```

- `T`: Type of element (must be nothrow destructible)
- `Capacity`: Power-of-two queue size

---

## Interface Summary

```cpp
bool push(const T &item);
bool emplace(T &&item);
template <typename... Args> bool try_emplace(Args&&...);
bool pop(T &out);
T* try_pop();
std::optional<std::reference_wrapper<T>> try_pop_ref();
bool empty() const;
bool full() const;
size_t size() const;
```

## Responsibilities

- Store a bounded ring buffer of `T`
- Allow single-producer push and single-consumer pop without contention
- Maintain high cache locality and eliminate allocations

## Internals

- Based on a circular buffer
- `_head` and `_tail` are padded to avoid false sharing
- Uses `std::aligned_storage_t` to store elements without constructing them prematurely

## Notes

- Capacity must be a power of two
- Used internally in `MarketDataBus` to deliver events to each subscriber
- Destructs all remaining elements on shutdown

## Example Use

```cpp
SPSCQueue<int, 1024> queue;
queue.push(42);
int x;
if (queue.pop(x)) { /* use x */ }
```