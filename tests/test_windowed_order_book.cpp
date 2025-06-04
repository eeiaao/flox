/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/book/windowed_order_book.h"
#include "flox/book/windowed_order_book_factory.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <gtest/gtest.h>

using namespace flox;

TEST(WindowedOrderBookTest, ApplySnapshot)
{
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1),
                                             Price::fromDouble(100.0)});

  EventPool<BookUpdateEvent, 3> pool;
  auto snapshot = pool.acquire();
  snapshot->type = BookUpdateType::SNAPSHOT;
  snapshot->bids = {{Price::fromDouble(20000), Quantity::fromDouble(5)},
                    {Price::fromDouble(19990), Quantity::fromDouble(3)}};
  snapshot->asks = {{Price::fromDouble(20010), Quantity::fromDouble(2)},
                    {Price::fromDouble(20020), Quantity::fromDouble(4)}};
  book->applyBookUpdate(*snapshot);

  ASSERT_EQ(book->bestBid(), Price::fromDouble(20000));
  ASSERT_EQ(book->bestAsk(), Price::fromDouble(20010));
}

TEST(WindowedOrderBookTest, ApplyDelta)
{
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)});

  EventPool<BookUpdateEvent, 3> pool;
  auto snapshot = pool.acquire();
  snapshot->type = BookUpdateType::SNAPSHOT;
  snapshot->bids = {{Price::fromDouble(1500), Quantity::fromDouble(1)}};
  snapshot->asks = {{Price::fromDouble(1505), Quantity::fromDouble(1)}};
  book->applyBookUpdate(*snapshot);

  auto delta = pool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{Price::fromDouble(1500), Quantity::fromDouble(0)},
                 {Price::fromDouble(1495), Quantity::fromDouble(2)}};
  delta->asks = {{Price::fromDouble(1505), Quantity::fromDouble(3)}};
  book->applyBookUpdate(*delta);

  ASSERT_EQ(book->bestBid(), Price::fromDouble(1495));
  ASSERT_EQ(book->bestAsk(), Price::fromDouble(1505));
}

TEST(WindowedOrderBookTest, SnapshotRemovesStaleLevels)
{
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)});

  EventPool<BookUpdateEvent, 3> pool;
  auto snap1 = pool.acquire();
  snap1->type = BookUpdateType::SNAPSHOT;
  snap1->bids = {{Price::fromDouble(20000), Quantity::fromDouble(5)},
                 {Price::fromDouble(19990), Quantity::fromDouble(3)}};
  book->applyBookUpdate(*snap1);
  ASSERT_EQ(book->bestBid(), Price::fromDouble(20000));

  auto snap2 = pool.acquire();
  snap2->type = BookUpdateType::SNAPSHOT;
  snap2->bids = {{Price::fromDouble(19990), Quantity::fromDouble(7)}};
  book->applyBookUpdate(*snap2);
  ASSERT_EQ(book->bestBid(), Price::fromDouble(19990));
}

TEST(WindowedOrderBookTest, PriceIndexRoundTrip)
{
  WindowedOrderBookFactory factory;
  auto book = static_cast<WindowedOrderBook*>(
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)}));

  Price p = Price::fromDouble(20000.0);
  size_t index = book->priceToIndex(p);
  Price back = book->indexToPrice(index);
  ASSERT_EQ(p, back);
}

TEST(WindowedOrderBookTest, BestBidAskEmptyAfterErase)
{
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)});

  EventPool<BookUpdateEvent, 3> pool;
  auto snap = pool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{Price::fromDouble(100), Quantity::fromDouble(1)}};
  snap->asks = {{Price::fromDouble(101), Quantity::fromDouble(1)}};
  book->applyBookUpdate(*snap);

  auto delta = pool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{Price::fromDouble(100), Quantity::fromDouble(0)}};
  delta->asks = {{Price::fromDouble(101), Quantity::fromDouble(0)}};
  book->applyBookUpdate(*delta);

  ASSERT_FALSE(book->bestBid().has_value());
  ASSERT_FALSE(book->bestAsk().has_value());
}

TEST(WindowedOrderBookTest, DeltaAddsNewLevel)
{
  WindowedOrderBookFactory factory;
  auto book = static_cast<WindowedOrderBook*>(
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)}));

  EventPool<BookUpdateEvent, 3> pool;
  auto snap = pool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1)}};
  book->applyBookUpdate(*snap);

  auto delta = pool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{Price::fromDouble(99.9), Quantity::fromDouble(2)}};
  book->applyBookUpdate(*delta);

  ASSERT_EQ(book->bestBid(), Price::fromDouble(100.0));
  ASSERT_EQ(book->bidAtPrice(Price::fromDouble(99.9)), Quantity::fromDouble(2));
}

TEST(WindowedOrderBookTest, DeltaRemovesLevel)
{
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)});

  EventPool<BookUpdateEvent, 3> pool;
  auto snap = pool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1)},
                {Price::fromDouble(99.9), Quantity::fromDouble(2)}};
  book->applyBookUpdate(*snap);

  auto delta = pool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(0)}};
  book->applyBookUpdate(*delta);

  ASSERT_EQ(book->bestBid(), Price::fromDouble(99.9));
}

TEST(WindowedOrderBookTest, DeltaModifiesLevel)
{
  WindowedOrderBookFactory factory;
  auto book = static_cast<WindowedOrderBook*>(
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)}));

  EventPool<BookUpdateEvent, 3> pool;
  auto snap = pool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1)}};
  book->applyBookUpdate(*snap);

  auto delta = pool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(5)}};
  book->applyBookUpdate(*delta);

  ASSERT_EQ(book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(5));
}

TEST(WindowedOrderBookTest, DeltaIsPartialUpdate)
{
  WindowedOrderBookFactory factory;
  auto book = static_cast<WindowedOrderBook*>(
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)}));

  EventPool<BookUpdateEvent, 3> pool;
  auto snap = pool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(1)},
                {Price::fromDouble(99.9), Quantity::fromDouble(2)}};
  book->applyBookUpdate(*snap);

  auto delta = pool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{Price::fromDouble(100.0), Quantity::fromDouble(3)}};
  book->applyBookUpdate(*delta);

  ASSERT_EQ(book->bidAtPrice(Price::fromDouble(100.0)), Quantity::fromDouble(3));
  ASSERT_EQ(book->bidAtPrice(Price::fromDouble(99.9)), Quantity::fromDouble(2));
}

TEST(WindowedOrderBookTest, PriceRoundTrip)
{
  WindowedOrderBookFactory factory;
  auto book = static_cast<WindowedOrderBook*>(
      factory.create(WindowedOrderBookConfig{Price::fromDouble(0.1), Price::fromDouble(100.0)}));

  std::vector<Price> prices = {Price::fromDouble(99.9), Price::fromDouble(100.0),
                               Price::fromDouble(100.1)};
  for (Price p : prices)
  {
    size_t index = book->priceToIndex(p);
    ASSERT_EQ(book->indexToPrice(index), p);
  }
}
