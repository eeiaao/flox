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

// Applies a full snapshot and verifies best bid/ask are correctly set.
// This is the initial population of the order book.
TEST(WindowedOrderBookTest, ApplySnapshot) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snapshot = bookUpdatePool.acquire();
  snapshot->type = BookUpdateType::SNAPSHOT;
  snapshot->bids = {{20000, 5}, {19990, 3}};
  snapshot->asks = {{20010, 2}, {20020, 4}};
  snapshot->timestamp = std::chrono::system_clock::now();

  book->applyBookUpdate(*snapshot);

  auto bid = book->bestBid();
  auto ask = book->bestAsk();
  ASSERT_TRUE(bid.has_value());
  ASSERT_TRUE(ask.has_value());
  EXPECT_DOUBLE_EQ(*bid, 20000);
  EXPECT_DOUBLE_EQ(*ask, 20010);
}

// Applies a snapshot followed by a delta:
// - Snapshot sets bid at 1500 and ask at 1505
// - Delta removes bid at 1500 and adds new bid at 1495, modifies ask at 1505
// Checks that the best bid and ask reflect the delta correctly.
TEST(WindowedOrderBookTest, ApplyDelta) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snapshot = bookUpdatePool.acquire();

  snapshot->type = BookUpdateType::SNAPSHOT;
  snapshot->bids = {{1500, 1}};
  snapshot->asks = {{1505, 1}};
  snapshot->timestamp = std::chrono::system_clock::now();
  book->applyBookUpdate(*snapshot);

  auto delta = bookUpdatePool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{1500, 0}, {1495, 2}};
  delta->asks = {{1505, 3}};
  delta->timestamp = std::chrono::system_clock::now();
  book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(*book->bestBid(), 1495);
  EXPECT_DOUBLE_EQ(*book->bestAsk(), 1505);
}

// Applies two snapshots:
// - First sets bids at 20000 and 19990
// - Second snapshot contains only 19990
// This tests that snapshot removes levels not explicitly present —
// i.e., 20000 is wiped out, not retained.
TEST(WindowedOrderBookTest, SnapshotRemovesStaleLevels) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snap1 = bookUpdatePool.acquire();
  snap1->type = BookUpdateType::SNAPSHOT;
  snap1->bids = {{20000, 5}, {19990, 3}},
  snap1->timestamp = std::chrono::system_clock::now();
  book->applyBookUpdate(*snap1);

  EXPECT_DOUBLE_EQ(*book->bestBid(), 20000);

  auto snap2 = bookUpdatePool.acquire();
  snap2->type = BookUpdateType::SNAPSHOT;
  snap2->bids = {{19990, 7}}; // 20000 missing
  snap2->timestamp = std::chrono::system_clock::now();
  book->applyBookUpdate(*snap2);

  EXPECT_DOUBLE_EQ(*book->bestBid(), 19990);
}

// Verifies that a price converted to an index and back remains the same.
// Ensures integrity of price <-> index mapping functions.
// This is critical for avoiding off-by-one or rounding errors in price levels.
TEST(WindowedOrderBookTest, PriceIndexRoundTrip) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  auto windowedOrderBook = dynamic_cast<WindowedOrderBook *>(book);

  double price = 20000.0;
  size_t index = windowedOrderBook->priceToIndex(price);
  double back = windowedOrderBook->indexToPrice(index);

  EXPECT_DOUBLE_EQ(price, back);
}

// Applies snapshot with 1 bid and 1 ask.
// Then applies a delta that erases both (sets qty = 0).
// Ensures that both bestBid() and bestAsk() return nullopt when the book is
// empty.
TEST(WindowedOrderBookTest, BestBidAskEmptyAfterErase) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snap = bookUpdatePool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{100, 1}};
  snap->asks = {{101, 1}};
  book->applyBookUpdate(*snap);

  auto delta = bookUpdatePool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{100, 0}};
  delta->asks = {{101, 0}};
  book->applyBookUpdate(*delta);

  EXPECT_FALSE(book->bestBid().has_value());
  EXPECT_FALSE(book->bestAsk().has_value());
}

// Applies snapshot with a single bid at 100.0
// Then applies delta that adds a new bid at 99.9
// Verifies:
// - bestBid remains 100.0 (unchanged)
// - quantity at 99.9 is correctly stored and retrievable
TEST(WindowedOrderBookTest, DeltaAddsNewLevel) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  auto windowedOrderBook = dynamic_cast<WindowedOrderBook *>(book);

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snap = bookUpdatePool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{100.0, 1}};
  book->applyBookUpdate(*snap);

  auto delta = bookUpdatePool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{99.9, 2}};
  book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(*book->bestBid(), 100.0);
  EXPECT_DOUBLE_EQ(windowedOrderBook->getBidQuantity(99.9), 2.0);
}

// Applies snapshot with bids at 100.0 and 99.9
// Then applies delta that removes 100.0 (qty = 0)
// Verifies:
// - top level (100.0) is removed
// - bestBid now returns 99.9
TEST(WindowedOrderBookTest, DeltaRemovesLevel) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snap = bookUpdatePool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{100.0, 1}, {99.9, 2}};
  book->applyBookUpdate(*snap);

  auto delta = bookUpdatePool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{100.0, 0}};
  book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(*book->bestBid(), 99.9);
}

// Applies snapshot with bid at 100.0 qty = 1
// Applies delta that modifies 100.0 qty to 5
// Verifies:
// - updated quantity is stored correctly
// - level isn't removed or duplicated
TEST(WindowedOrderBookTest, DeltaModifiesLevel) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  auto windowedOrderBook = dynamic_cast<WindowedOrderBook *>(book);

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snap = bookUpdatePool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{100.0, 1}};
  book->applyBookUpdate(*snap);

  auto delta = bookUpdatePool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{100.0, 5}};
  book->applyBookUpdate(*delta);

  EXPECT_DOUBLE_EQ(windowedOrderBook->getBidQuantity(100.0), 5.0);
}

// Applies snapshot with two bids: 100.0 and 99.9
// Delta only modifies 100.0
// Verifies:
// - 100.0 is updated
// - 99.9 remains unchanged
// Confirms that deltas are partial — they don't wipe untouched levels
TEST(WindowedOrderBookTest, DeltaIsPartialUpdate) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});

  EventPool<BookUpdateEvent, 3> bookUpdatePool;
  auto snap = bookUpdatePool.acquire();
  snap->type = BookUpdateType::SNAPSHOT;
  snap->bids = {{100.0, 1}, {99.9, 2}};
  book->applyBookUpdate(*snap);

  auto delta = bookUpdatePool.acquire();
  delta->type = BookUpdateType::DELTA;
  delta->bids = {{100.0, 3}};
  book->applyBookUpdate(*delta);

  auto windowedOrderBook = dynamic_cast<WindowedOrderBook *>(book);

  EXPECT_DOUBLE_EQ(windowedOrderBook->getBidQuantity(100.0), 3.0);
  EXPECT_DOUBLE_EQ(windowedOrderBook->getBidQuantity(99.9), 2.0);
}

// Verifies that multiple prices round-trip correctly through priceToIndex →
// indexToPrice This guarantees alignment of price ladder arithmetic and helps
// avoid rounding bugs that would shift levels unintentionally.
TEST(WindowedOrderBookTest, PriceRoundTrip) {
  constexpr double tickSize = 0.1;
  constexpr double expectedDeviation = 100.0;
  WindowedOrderBookFactory factory;
  auto book =
      factory.create(WindowedOrderBookConfig{tickSize, expectedDeviation});
  auto windowedOrderBook = static_cast<WindowedOrderBook *>(book);

  std::vector<double> testPrices = {99.9, 100.0, 100.1};
  for (double p : testPrices) {
    size_t i = windowedOrderBook->priceToIndex(p);
    double back = windowedOrderBook->indexToPrice(i);
    EXPECT_DOUBLE_EQ(p, back);
  }
}
