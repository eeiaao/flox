# TradeBus

`TradeBus` distributes `TradeEvent` objects to subscribers. It is implemented via the generic `EventBus` template.

## Definition

```cpp
#ifdef USE_SYNC_MARKET_BUS
using TradeBus = EventBus<TradeEvent, SyncPolicy<TradeEvent>>;
#else
using TradeBus = EventBus<TradeEvent, AsyncPolicy<TradeEvent>>;
#endif
```

## Responsibilities

- Broadcast trades to strategies and analytics modules
- Provide sequence numbering when publishing

## Notes

- Combined with `BookUpdateBus` in `MarketDataBus`
- Controlled by the `USE_SYNC_MARKET_BUS` compile definition

