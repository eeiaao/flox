# StorageSink (concept / trait / ref)

Compile-time contract and type-erased handle for subsystems that **persist data** (orders, fills, custom events) to an external store.

~~~cpp
// Concept
template <typename T>
concept StorageSink =
    Subsystem<T> &&
    requires(T sink, const Order& o) {
      { sink.store(o) } -> std::same_as<void>;
    };
~~~

| Piece | Responsibility |
|-------|----------------|
| **`StorageSinkTrait`** | Generates a static v-table combining `SubsystemTrait` with a single `store()` method. |
| **`StorageSinkRef`**   | Two-pointer handle `{void*, VTable*}` that forwards `start/stop` and `store()` with zero virtuals. |

## StorageSinkRef API
````cpp
void start() const;             // lifecycle
void stop()  const;
void store(const Order&) const; // persist order to DB / file / queue
````

## Purpose

* Provide a **pluggable output backend** (Postgres, Kafka, Parquet, stdout, …) without coupling engine code to specific libraries.
* Ensure every sink is also a `Subsystem`, so it can be started and stopped deterministically by the engine.

## Notes

* One pointer indirection per call; no runtime overhead beyond that.
* Implementers only need to satisfy `concepts::StorageSink`; wrap with `StorageSinkRef` for injection.
