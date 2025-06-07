/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/events/book_update_event.h"

namespace flox
{

MarketDataEventType BookUpdateEvent::eventType() const noexcept
{
  return MarketDataEventType::BOOK;
}

void BookUpdateEvent::dispatchTo(IMarketDataSubscriber& sub) const
{
  sub.onMarketData(*this);
}

}  // namespace flox