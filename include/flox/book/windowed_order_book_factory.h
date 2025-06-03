/*
 * Flox Engine
 * Developed by Evgenii Makarov (https://github.com/eeiaao)
 *
 * Copyright (c) 2025 Evgenii Makarov
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include "flox/book/abstract_order_book_factory.h"
#include "flox/book/windowed_order_book.h"
#include "flox/common.h"
#include "flox/util/arena_resource.h"

#include <cstddef>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace flox {

struct WindowedOrderBookConfig : public IOrderBookConfig {
  Price tickSize;
  Price expectedDeviation;

  WindowedOrderBookConfig(Price tickSize, Price expectedDeviation)
      : tickSize(tickSize), expectedDeviation(expectedDeviation) {}
};

class WindowedOrderBookFactory : public IOrderBookFactory {
public:
  explicit WindowedOrderBookFactory(std::size_t arenaSize = 2'000'000)
      : _buffer(new std::byte[arenaSize]), _arena(_buffer.get(), arenaSize),
        _mem(&_arena) {}

  IOrderBook *create(const IOrderBookConfig &config) override {
    const auto &windowedConfig =
        static_cast<const WindowedOrderBookConfig &>(config);
    void *ptr =
        _mem->allocate(sizeof(WindowedOrderBook), alignof(WindowedOrderBook));
    return new (ptr) WindowedOrderBook(windowedConfig.tickSize,
                                       windowedConfig.expectedDeviation, _mem);
  }

  void reset() { _arena.reset(); }

private:
  std::unique_ptr<std::byte[]> _buffer;
  ArenaResource _arena;
  std::pmr::memory_resource *_mem;
};

} // namespace flox
