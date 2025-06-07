/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/events/trade_event.h"
#include "flox/engine/abstract_market_data_subscriber.h"

namespace flox
{

MarketDataEventType TradeEvent::eventType() const noexcept
{
  return MarketDataEventType::TRADE;
}

void TradeEvent::dispatchTo(IMarketDataSubscriber& sub) const
{
  sub.onMarketData(*this);
}

}  // namespace flox