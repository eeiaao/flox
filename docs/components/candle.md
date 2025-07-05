# Candle

`Candle` stores a single OHLCV record for a time-boxed interval.

~~~cpp
struct Candle {
  Price  open;
  Price  high;
  Price  low;
  Price  close;
  Volume volume;
  std::chrono::steady_clock::time_point startTime;
  std::chrono::steady_clock::time_point endTime;

  Candle() = default;

  Candle(std::chrono::steady_clock::time_point ts,
         Price  price,
         Volume qty)
      : open(price),
        high(price),
        low(price),
        close(price),
        volume(qty),
        startTime(ts),
        endTime(ts) {}
};
~~~

## Purpose
* Represent aggregated trade data (Open, High, Low, Close, Volume) for charting,
  strategy signals, and analytics.

## Responsibilities
| Field       | Description                                  |
|-------------|----------------------------------------------|
| `open`      | First trade price in the interval            |
| `high`      | Maximum trade price                          |
| `low`       | Minimum trade price                          |
| `close`     | Last trade price                             |
| `volume`    | Sum of traded quantity                       |
| `startTime` | Interval start (aligned by aggregator)       |
| `endTime`   | Interval end (updated as trades arrive)      |

## Internal Behaviour
* `Candle(intervalStart, price, qty)` initialises **all** price fields to the
  first trade price and sets both time points to `intervalStart`; volume starts
  with the first trade quantity.
* High/low/close/volume fields are updated by the aggregator as additional
  trades arrive during the interval.

## Notes
* Trivially copyable; no dynamic allocations.
* Time points use `steady_clock` for monotonicity in latency-sensitive code.
