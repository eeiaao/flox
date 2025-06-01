# Abstract Engine Builder

The `IEngineBuilder` interface defines the contract for constructing the Flox engine and its subsystems.

## Purpose

To allow flexible configuration and construction of an `IEngine` instance with all required subsystems and strategies.

## Interface Definition

```cpp
class IEngineBuilder {
public:
  virtual ~IEngineBuilder() = default;

  virtual std::unique_ptr<IEngine> build() = 0;
};
```

## Responsibilities

- Encapsulate the logic of creating and wiring together all engine components
- Provide a customizable way to build different configurations of the engine

## Notes

- Typically used in main application code to construct the runtime engine
- Ensures separation of construction and runtime execution phases