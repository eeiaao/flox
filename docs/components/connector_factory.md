# ConnectorFactory

`ConnectorFactory` is a singleton registry that maps **connector type strings**
to **factory functions** returning `std::shared_ptr<ExchangeConnector>`.

~~~cpp
class ConnectorFactory {
public:
  using CreatorFunc =
      std::move_only_function<std::shared_ptr<ExchangeConnector>(const std::string& symbol)>;

  static ConnectorFactory& instance();                        // global access

  void registerConnector(const std::string& type,
                         CreatorFunc creator);                // add mapping

  std::optional<std::shared_ptr<ExchangeConnector>>
  createConnector(const std::string& type,
                  const std::string& symbol);                 // get new connector

private:
  ConnectorFactory() = default;
  std::unordered_map<std::string, CreatorFunc> _creators;     // type → factory
};
~~~

## Purpose
* Decouple **engine configuration** from concrete `ExchangeConnector` classes.
* Enable dynamic creation of connectors (Bybit, Binance, Mock, …) via config strings.

## Responsibilities
| Action             | Behaviour                                                                |
|--------------------|---------------------------------------------------------------------------|
| `registerConnector`| Stores a *move-only* `CreatorFunc` under its type key.                    |
| `createConnector`  | Looks up the type; if found, returns a new instance for the given symbol. |
| Singleton lifetime | `instance()` guarantees one global registry for the entire process.      |

## Internal Behaviour
* `_creators` is an `unordered_map<std::string, CreatorFunc>`; look-ups are O(1) average.
* `CreatorFunc` is `std::move_only_function`, ensuring no accidental copies of heavy lambdas / bindings.
* If a type is missing, `createConnector` returns `std::nullopt` so callers can handle errors gracefully.

## Usage Example
~~~cpp
// registration (usually in connector module init)
ConnectorFactory::instance().registerConnector(
    "bybit",
    [](const std::string& sym) {
      return std::make_shared<BybitExchangeConnector>(sym);
    });

// creation (engine startup)
auto connOpt = ConnectorFactory::instance().createConnector("bybit", "DOTUSDT");
if (connOpt) engine.addConnector(*connOpt);
~~~

## Notes
* **Thread safety**: registration is typically done at startup; concurrent reads are fine after that.
* Factory lambdas can capture config (API keys, WS endpoints) before registration.
