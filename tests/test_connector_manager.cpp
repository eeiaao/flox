/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/common.h"
#include "flox/connector/connector_manager.h"
#include "flox/connector/exchange_connector.h"
#include "flox/engine/events/book_update_event.h"
#include "flox/engine/events/trade_event.h"
#include "flox/engine/market_data_event_pool.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace flox;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;

class MockExchangeConnector : public ExchangeConnector
{
 public:
  MOCK_METHOD(void, start, (), (override));
  MOCK_METHOD(void, stop, (), (override));
  MOCK_METHOD(std::string, exchangeId, (), (const, override));
  void setCallbacks(BookUpdateCallback book, TradeCallback trade) override
  {
    _bookCb = std::move(book);
    _tradeCb = std::move(trade);
    onCallbacksSet();
  }
  MOCK_METHOD(void, onCallbacksSet, ());

  BookUpdateCallback _bookCb;
  TradeCallback _tradeCb;

  void triggerTestData()
  {
    if (_bookCb && _tradeCb)
    {
      EventPool<BookUpdateEvent, 3> bookUpdatePool;
      EventPool<TradeEvent, 3> tradePool;

      auto bu = bookUpdatePool.acquire();
      bu->symbol = 42;

      auto tr = tradePool.acquire();
      tr->symbol = 42;
      tr->price = Price::fromDouble(3.14);

      _bookCb(bu.get());
      _tradeCb(tr.get());
    }
  }
};

TEST(ConnectorManagerTest, RegisterAndStartAll)
{
  auto connector = std::make_shared<MockExchangeConnector>();
  ConnectorManager manager;

  EXPECT_CALL(*connector, exchangeId()).WillOnce(Return("bybit"));

  EXPECT_CALL(*connector, onCallbacksSet()).Times(1);

  EXPECT_CALL(*connector, start()).WillOnce(Invoke([&]
                                                   { connector->triggerTestData(); }));

  manager.registerConnector(connector);

  bool bookUpdateCalled = false;
  bool tradeCalled = false;

  manager.startAll(
      [&](BookUpdateEvent* bu)
      {
        ASSERT_NE(bu, nullptr);
        EXPECT_EQ(bu->symbol, 42);
        bookUpdateCalled = true;
      },
      [&](TradeEvent* tr)
      {
        EXPECT_EQ(tr->symbol, 42);
        EXPECT_EQ(tr->price, Price::fromDouble(3.14));
        tradeCalled = true;
      });

  EXPECT_TRUE(bookUpdateCalled);
  EXPECT_TRUE(tradeCalled);
}
