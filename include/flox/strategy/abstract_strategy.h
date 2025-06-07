/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/candle.h"
#include "flox/book/events/book_update_event.h"
#include "flox/book/events/trade_event.h"
#include "flox/engine/abstract_market_data_subscriber.h"
#include "flox/engine/events/market_data_event.h"
#include "flox/execution/abstract_executor.h"
#include "flox/position/abstract_position_manager.h"
#include "flox/risk/abstract_risk_manager.h"
#include "flox/validation/abstract_order_validator.h"

namespace flox
{

class IStrategy : public IMarketDataSubscriber
{
 public:
  virtual ~IStrategy() = default;

  // Lifecycle
  virtual void onStart() {}
  virtual void onStop() {}

  // Event hooks
  virtual void onCandle(SymbolId symbol, const Candle& candle) {}
  virtual void onTrade(const TradeEvent& trade) override {}
  virtual void onBookUpdate(const BookUpdateEvent& bookUpdate) override {}

  SubscriberId id() const override { return _subscriberId; }
  SubscriberMode mode() const override { return SubscriberMode::PUSH; }

  // Dependency injection
  void setRiskManager(IRiskManager* manager) { _riskManager = manager; }
  void setPositionManager(IPositionManager* manager) { _positionManager = manager; }
  void setOrderExecutor(IOrderExecutor* executor) { _executor = executor; }
  void setOrderValidator(IOrderValidator* validator) { _validator = validator; }

 protected:
  IRiskManager* GetRiskManager() { return _riskManager; }
  IPositionManager* GetPositionManager() { return _positionManager; }
  IOrderExecutor* GetOrderExecutor() { return _executor; }
  IOrderValidator* GetOrderValidator() { return _validator; }

 private:
  SubscriberId _subscriberId = reinterpret_cast<SubscriberId>(this);

  IRiskManager* _riskManager = nullptr;
  IPositionManager* _positionManager = nullptr;
  IOrderExecutor* _executor = nullptr;
  IOrderValidator* _validator = nullptr;
};

}  // namespace flox
