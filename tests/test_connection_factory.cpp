/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#include "flox/connector/connector_factory.h"
#include "flox/connector/exchange_connector.h"

#include <gtest/gtest.h>

using namespace flox;

class DummyConnector : public ExchangeConnector
{
 public:
  void start() override {}
  void stop() override {}
  std::string exchangeId() const override { return "dummy"; }
};

TEST(ConnectorFactoryTest, RegisterAndCreateConnector)
{
  ConnectorFactory::instance().registerConnector(
      "dummy", [](const std::string& symbol)
      { return std::make_shared<DummyConnector>(); });

  auto connector = ConnectorFactory::instance().createConnector("dummy", "BTCUSDT");
  EXPECT_TRUE(connector.has_value());
  EXPECT_EQ(connector.value()->exchangeId(), "dummy");
}

TEST(ConnectorFactoryTest, UnknownConnectorReturnsNullptr)
{
  auto connector = ConnectorFactory::instance().createConnector("unknown", "BTCUSDT");
  EXPECT_FALSE(connector.has_value());
}
