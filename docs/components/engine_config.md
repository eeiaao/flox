# EngineConfig (and nested configs)

Defines the runtime configuration structures for FLOX: supported exchanges, kill-switch limits, and logging setup.

## Structures

### `SymbolConfig`
| Field      | Meaning                        |
|------------|--------------------------------|
| `symbol`   | Exchange symbol string (`"BTCUSDT"`). |
| `tickSize` | Minimum price increment for this symbol. |

### `ExchangeConfig`
| Field    | Meaning                                      |
|----------|----------------------------------------------|
| `name`   | Human-readable exchange name (`"Bybit"`).    |
| `type`   | Key used by `ConnectorFactory` (`"bybit"`).  |
| `symbols`| List of `SymbolConfig` handled by this connector. |

### `KillSwitchConfig`
| Field                 | Default | Meaning                                                |
|-----------------------|---------|--------------------------------------------------------|
| `maxOrderQty`         | 10 000  | Hard cap on a single order quantity.                  |
| `maxLoss`             | −1e6    | Loss threshold that triggers a global shutdown.       |
| `maxOrdersPerSecond`  | −1      | Rate-limit; negative disables this check.             |

### `EngineConfig`
| Field              | Meaning                                         |
|--------------------|-------------------------------------------------|
| `exchanges`        | Vector of `ExchangeConfig` entries.             |
| `killSwitchConfig` | Limits loaded into the run-time kill switch.    |
| `logLevel`         | Text log level (`"trace"`, `"debug"`, `"info"`, …). |
| `logFile`          | Path for file-based logging (empty → stderr).   |
