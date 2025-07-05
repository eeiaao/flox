# Engine

`Engine` is the top-level coordinator: it owns every **bus**, **subsystem** and **exchange connector**, starts them in the correct order, and stops them on shutdown.

```cpp
class Engine {
public:
  Engine(const EngineConfig& cfg,
         std::vector<SubsystemRef> buses,
         std::vector<SubsystemRef> subsystems,
         std::vector<std::shared_ptr<ExchangeConnector>> connectors);

  void start();   // initialise & run all components
  void stop();    // graceful shutdown
};
````

### Related Type

| Struct             | Purpose                                                                                                      |
| ------------------ | ------------------------------------------------------------------------------------------------------------ |
| `ExchangeInstance` | Bundles connector metadata (`exchangeType`, `name`, `symbol`) with the concrete `ExchangeConnector` pointer. |

### Purpose

* Centralise ownership of every runtime component.
* Guarantee deterministic startup / shutdown sequencing.
* Keep strong separation between **infrastructure** (buses, subsystems) and **external I/O** (connectors).

### Responsibilities

| Phase   | Action                                                                 |
| ------- | ---------------------------------------------------------------------- |
| `start` | 1) Start buses → 2) start subsystems → 3) start connectors.            |
| `stop`  | Reverse order: connectors → subsystems → buses.                        |
| State   | Store the immutable `EngineConfig` and vectors of injected components. |

### Notes

* Fulfils `concepts::Engine`, so it can be referenced anywhere a generic `Subsystem` is expected.
* Construction is **dependency-injected**: the builder outside the class instantiates buses/subsystems/connectors and passes them in, enabling test harnesses or alternative compositions.
