/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/engine/events/book_update_event.h"
#include "flox/engine/events/trade_event.h"

#include <functional>
#include <string>

namespace flox {

class ExchangeConnector {
public:
  virtual ~ExchangeConnector() = default;

  using BookUpdateCallback = std::function<void(BookUpdateEvent *)>;
  using TradeCallback = std::function<void(TradeEvent *)>;

  virtual void start() = 0;
  virtual void stop() = 0;

  virtual std::string exchangeId() const = 0;

  virtual void setCallbacks(BookUpdateCallback onBookUpdate,
                            TradeCallback onTrade) {
    _onBookUpdate = std::move(onBookUpdate);
    _onTrade = std::move(onTrade);
  }

protected:
  void emitBookUpdate(BookUpdateEvent *bu) {
    if (_onBookUpdate)
      _onBookUpdate(bu);
  }

  void emitTrade(TradeEvent *t) {
    if (_onTrade)
      _onTrade(t);
  }

private:
  BookUpdateCallback _onBookUpdate;
  TradeCallback _onTrade;
};

} // namespace flox
