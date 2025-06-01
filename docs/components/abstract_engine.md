# Engine Interface

The `IEngine` interface defines the top-level runtime control for the Flox trading system.

## Purpose

To provide lifecycle management for the entire engine, including startup and shutdown sequences.

---

## Interface Summary

```cpp
class IEngine {
public:
  virtual ~IEngine() = default;
  virtual void start() = 0;
  virtual void stop() = 0;
};
```

## Responsibilities

- Initialize and start all registered subsystems (strategies, bus, sink, etc.)
- Control shutdown procedure for graceful termination

## Implementations

- Constructed via `IEngineBuilder`

## Notes

- Entry point for running Flox in production or demo mode
- Should be triggered after system configuration and strategy wiring