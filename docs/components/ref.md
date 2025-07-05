# Ref / RefBase / PoolAllocator

Type-erasure helpers that let FLOX pass around **trait-based handles** without
virtual inheritance, plus a compile-time allocator helper for small object
pools.

## ```Ref<Trait>```

````cpp
template <typename Trait>
class Ref {
public:
  using VTable = typename Trait::VTable;

  template <typename T>        static Ref from(T* obj);                  // build v-table
  template <typename OtherTrait> typename RefFor<OtherTrait>::type as() const; // cast to another trait
  template <typename T>        T& get() const;                           // downcast

  void*        raw()    const;
  const VTable* vtable() const;
};
````

* Stores `{void* _ptr, VTable* _vtable}` (two pointers).
* `as<OtherTrait>()` re-wraps the same object with a different trait view.

## ```RefBase<Derived, Trait>```

```cpp
template <typename Derived, typename Trait>
struct RefBase : Ref<Trait> {
  static Derived from(void* ptr, const VTable* vt);   // used by traits
  template <typename Impl> static Derived from(Impl*); // convenience
};
```

* CRTP layer that concrete handle types inherit from (e.g. `OrderBookRef`).
* Provides protected ctor so only the trait machinery can instantiate.

## ```make<T>(…)```

Factory that:

1. Looks up `T::Trait` and the allocator alias `T::Allocator`.
2. Calls `Allocator::create<RefType, T>(args…)` which constructs the concrete
   object and returns the corresponding `Ref`.

```cpp
auto obRef = make<NLevelOrderBook<>>(tickSize);   // returns OrderBookRef
```

## ```PoolAllocator<Trait, N>```

Compile-time **per-thread freelist** allocator used by most components.

```cpp
template <typename Trait, size_t N = 8>
struct PoolAllocator {
  template <typename RefType, typename Impl, typename... Args>
  static RefType create(Args&&...);
};
```

* Internally owns `FreeListAllocator<Impl, N>` (TLS) and returns objects via
  `RefType::from(obj)`.
* Eliminates heap calls for up to **N** live instances per thread.

## Workflow Overview

```
┌──────────────┐            make<T>()            ┌───────────────┐
│  Concrete T  │  --placement-new--> pool ---->  │   Impl* ptr   │
└──────────────┘                                 └───────────────┘
        │ vtable generated via Trait::makeVTable<T>() │
        └──────────────────────────────► Ref<Trait> handle
```

`Ref` objects travel through the engine; when they need another view they call
`as<OtherTrait>()`, reusing the same underlying instance without extra storage.
