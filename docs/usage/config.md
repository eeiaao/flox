# Configuration

Runtime settings are supplied through the **`EngineConfig`** structure, usually
deserialized from JSON/TOML/YAML at startup.

## Purpose
* Describe exchanges, symbols, and kill-switch limits in a single document.
* Control logging verbosity and output destination.

## Fields

| Field                 | Type / Example                                    | Description                                                 |
|-----------------------|---------------------------------------------------|-------------------------------------------------------------|
| `logLevel`            | `"debug"`, `"info"`, `"warn"`                     | Minimum severity that appears in logs.                      |
| `logFile`             | `"flox.log"` (empty = stderr)                     | Optional file path for log output.                          |
| `exchanges[]`         | array of `ExchangeConfig`                         | Trading venues to connect.                                  |
| `killSwitchConfig`    | `KillSwitchConfig`                                | Hard limits that trigger immediate shutdown.                |

### `ExchangeConfig`

| Field       | Example              | Meaning                                |
|-------------|----------------------|----------------------------------------|
| `name`      | `"bybit"`            | Human-readable label for logs / UI.    |
| `type`      | `"mock"`             | Key used by `ConnectorFactory`.        |
| `symbols[]` | see below            | List of tradable symbols.              |

#### `SymbolConfig`

| Field               | Example   | Description                            |
|---------------------|-----------|----------------------------------------|
| `symbol`            | `"DOTUSDT"` | Exchange symbol code.                 |
| `tickSize`          | `0.001`   | Minimum price increment.               |

### `KillSwitchConfig`

| Field                | Default / Example | Purpose                                  |
|----------------------|-------------------|------------------------------------------|
| `maxOrderQty`        | `10000`           | Per-order quantity cap.                  |
| `maxLoss`            | `-5000`           | Total loss threshold before halt.        |
| `maxOrdersPerSecond` | `100`             | Rate limit; negative disables the check. |

## Minimal Example

````json
{
  "logLevel": "debug",
  "exchanges": [
    {
      "name": "bybit",
      "type": "mock",
      "symbols": [
        { "symbol": "DOTUSDT", "tickSize": 0.001 }
      ]
    }
  ],
  "killSwitchConfig": {
    "maxOrderQty": 10000,
    "maxLoss": -5000,
    "maxOrdersPerSecond": 100
  }
}
````