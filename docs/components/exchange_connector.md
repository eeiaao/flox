# ExchangeConnector (interface)

Abstract base class that adapts a specific exchange’s WebSocket/REST feed to FLOX.

~~~cpp
class ExchangeConnector {
public:
  using BookUpdateCallback = std::move_only_function<void(const BookUpdateEvent&)>;
  using TradeCallback      = std::move_only_function<void(const TradeEvent&)>;

  virtual ~ExchangeConnector() = default;

  virtual void start()  = 0;       // connect / subscribe
  virtual void stop()   = 0;       // disconnect / clean up
  virtual std::string exchangeId() const = 0;  // “bybit-btcusdt”, etc.

  virtual void setCallbacks(BookUpdateCallback onBookUpdate,
                            TradeCallback      onTrade);

protected:
  void emitBookUpdate(const BookUpdateEvent& bu);
  void emitTrade      (const TradeEvent&     tr);

private:
  BookUpdateCallback _onBookUpdate;
  TradeCallback      _onTrade;
};
~~~

## Purpose
* Provide a **uniform interface** so the engine can treat all exchanges the same.
* Forward raw market data as strongly typed events (`BookUpdateEvent`, `TradeEvent`).

## Key Points
| Method                | Role                                                            |
|-----------------------|-----------------------------------------------------------------|
| `start()` / `stop()`  | Lifecycle hooks called by `ConnectorManager`.                   |
| `exchangeId()`        | Returns unique ID used as map key and log prefix.               |
| `setCallbacks()`      | Injects lambdas that publish into `EventBus` instances.         |
| `emitBookUpdate()` / `emitTrade()` | Protected helpers that invoke the stored callbacks. |

## Implementation Sketch
````cpp
class BybitConnector : public ExchangeConnector {
  void start() override { /* open ws, auth, subscribe */ }
  void stop()  override { /* close ws */                }
  std::string exchangeId() const override { return "bybit"; }

  // on incoming WS frame:
  void onMessage(...) {
    BookUpdateEvent ev{arena};
    // fill ev…
    emitBookUpdate(ev);
  }
};
````

## Notes

* Callbacks are `std::move_only_function` → avoids accidental copies of heavy lambdas.
* Thread safety left to concrete implementation (each connector may run its own I/O thread).
