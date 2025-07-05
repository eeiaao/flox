# Writing Strategies

In FLOX a **strategy** is any class that satisfies the  
`concepts::Strategy` contract – i.e. it is both a `MarketDataSubscriber`
and a `Subsystem`.

````cpp
template <typename T>
concept Strategy =
    MarketDataSubscriber<T> && Subsystem<T>;
````

You do **not** inherit from a common base; you simply implement the
required methods and expose a static factory (or use `make<YourStrategy>()`)
to obtain a `StrategyRef`.

## Minimal Contract

| Category                  | Methods / Fields                                                                                                        |
| ------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| **Lifecycle**             | `void start()`, `void stop()`                                                                                           |
| **Subscriber Identity**   | `SubscriberId id()`, `SubscriberMode mode()`                                                                            |
| **Market-data callbacks** | `void onBookUpdate(const BookUpdateEvent&)`<br>`void onTrade(const TradeEvent&)`<br>`void onCandle(const CandleEvent&)` |

Anything beyond that – risk, execution, validation – is plugged in via
constructor parameters or setter functions **you** define.

## Dependency Injection Pattern

```cpp
class MyStrategy {
public:
  MyStrategy(OrderExecutorRef exec,
             RiskManagerRef   risk = {},
             OrderValidatorRef val  = {})
    : _exec(exec), _risk(risk), _val(val) {}

  /* required interface … */

private:
  OrderExecutorRef _exec;
  RiskManagerRef   _risk;
  OrderValidatorRef _val;
};
```

The builder injects the refs when the strategy instance is created.

## Recommended Workflow

```cpp
void onBookUpdate(const BookUpdateEvent& u) {
  if (!shouldTrade(u)) return;

  Order ord = buildOrder(u);

  std::string reason;
  if (_val  && !_val.validate(ord, reason)) return;
  if (_risk && !_risk.allow(ord))           return;

  _exec.submitOrder(ord);
}
```

1. **Signal** → decide if you want to trade.
2. **Build** an `Order` (limit/market, side, price, qty).
3. **Validate** → `OrderValidatorRef` (optional).
4. **Risk Check** → `RiskManagerRef` (optional).
5. **Submit** via `OrderExecutorRef`.

## Best Practices

* **Non-blocking callbacks** – your strategy runs on a dedicated queue,
  but long tasks still add latency.
* **No heap churn** – reuse objects or use the FLOX memory pools.
* **Stateless first** – only keep state you absolutely need; use
  `PositionManagerRef` when you must track positions.
* **Pool safety** – never keep raw pointers to events beyond the callback.

## Example Skeleton

```cpp
class MeanRevert final {
public:
  explicit MeanRevert(OrderExecutorRef exec) : _exec(exec) {}

  /* Subscriber identity */
  SubscriberId id()   const { return 42; }
  SubscriberMode mode() const { return SubscriberMode::PUSH; }

  /* Lifecycle */
  void start() {}
  void stop()  {}

  /* Market-data */
  void onTrade(const TradeEvent& t) {}
  void onCandle(const CandleEvent& c) {}
  void onBookUpdate(const BookUpdateEvent& u) {
    if (!spreadOK(u)) return;
    Order o = makeOrder(u);
    _exec.submitOrder(o);
  }

private:
  bool spreadOK(const BookUpdateEvent& u) const { /* … */ }
  Order makeOrder(const BookUpdateEvent& u) const { /* … */ }

  OrderExecutorRef _exec;
};
```

Register it in your builder with:

```cpp
auto strat = make<MeanRevert>(executorRef);
marketDataBus.subscribe(strat);
subsystems.push_back(strat);
```

The engine starts and stops the strategy automatically via the `Subsystem`
interface.
