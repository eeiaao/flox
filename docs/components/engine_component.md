# Engine / EngineBuilder Concepts

The header defines two C++20 concepts used to enforce type-safety around engine composition.

~~~cpp
namespace flox::concepts {

template <typename T>
concept Engine = Subsystem<T>;               // any type satisfying Subsystem

template <typename T>
concept EngineBuilder = requires(T b) {      // must expose build()
  { b.build() } -> Engine;
};

} // namespace flox::concepts
~~~

## Purpose
* **`Engine`** – shorthand to require that a type fulfils the broader `Subsystem` concept (start/stop, etc.).  
* **`EngineBuilder`** – constrains factory objects that produce an `Engine` via `build()`.

## Usage Example
```cpp
struct MyEngine : /* implements SubsystemTrait */ { /* … */ };

struct MyBuilder {
  MyEngine build() { return MyEngine{ /* cfg */ }; }
};

static_assert(flox::concepts::Engine<MyEngine>);
static_assert(flox::concepts::EngineBuilder<MyBuilder>);
```

## Notes
* By chaining concepts, compile-time errors point exactly to missing `Subsystem` requirements or absent `build()` methods.
