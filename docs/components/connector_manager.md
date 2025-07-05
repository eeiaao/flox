# ConnectorManager

`ConnectorManager` owns multiple concrete `ExchangeConnector` instances and
coordinates their lifecycle and callback wiring.

~~~cpp
class ConnectorManager {
public:
  void registerConnector(std::shared_ptr<ExchangeConnector> connector);

  void startAll(ExchangeConnector::BookUpdateCallback onBookUpdate,
                ExchangeConnector::TradeCallback      onTrade);

private:
  std::map<std::string, std::shared_ptr<ExchangeConnector>> connectors; // id → connector
};
~~~

## Purpose
* Maintain a **collection** of active exchange connectors, indexed by
  `exchangeId()`.
* Provide a **single entry point** (`startAll`) to initialise callbacks and
  start every connector in one call.

## Responsibilities
| Action           | Behaviour                                                                                         |
|------------------|----------------------------------------------------------------------------------------------------|
| `registerConnector` | Stores the shared pointer under its `exchangeId()` key (overwrites duplicates).                |
| `startAll`          | Binds the supplied callbacks to each connector and calls `start()` on all of them.             |

## Internal Behaviour
* Connectors are kept in a `std::map` to guarantee deterministic iteration
  order when starting (useful for reproducible tests).
* Callbacks are forwarded via lambdas that capture the original `onBookUpdate`
  and `onTrade` by move; this avoids extra indirections and preserves mutability.

## Notes
* No explicit `stopAll` yet — ensure each connector stops itself on destruction
  or extend the manager if coordinated shutdown is required.
* Thread safety: registration and `startAll` should run from the same thread
  during engine bootstrap.
