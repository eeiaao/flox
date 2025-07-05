#pragma once

#include "flox/book/book_update.h"
#include "flox/engine/market_data_subscriber_component.h"
#include "flox/util/memory/pool.h"

#include <cassert>
#include <memory_resource>

namespace flox
{

struct BookUpdateEvent : public pool::PoolableBase<BookUpdateEvent>
{
  using Listener = MarketDataSubscriberRef;

  BookUpdate update;
  uint64_t tickSequence = 0;

  BookUpdateEvent(std::pmr::memory_resource* res) : update(res)
  {
    assert(res != nullptr && "pmr::memory_resource is null!");
  }

  void clear()
  {
    update.bids.clear();
    update.asks.clear();
  }
};

static_assert(flox::concepts::Poolable<flox::BookUpdateEvent>);

}  // namespace flox
